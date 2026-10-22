#ifndef __HF_AUDIO_SERVER_H__
#define __HF_AUDIO_SERVER_H__

/*
===============================================================================

	hfAudioServer — Phase 8, threaded server split.

	The audio half of the Servers promotion: RID-handled samples and emitters,
	and a worker thread that turns the current emitter set + listener pose into
	a mixer snapshot (one resolved voice per emitter) that a device backend
	drains. It is the audio sibling of hfSimulationServer and shares its shape:
	RID registry, Kick/WaitForStep worker protocol, copy-out snapshot, and an
	inline mode that runs on the calling thread so the mix can be checked for
	determinism with and without the worker.

	It owns no device and no mixing buffer: hfAudioManager (miniaudio) is the
	front end that feeds a listener and emitter table in and applies the
	published left/right gains to the matching ma_sound nodes. Nothing here
	touches an OS, so the offline harness exercises the exact code the runtime
	runs — the same contract hfAudioData.h / hfAudioSpatial.h keep.

	hfAudioServer audio;
	hfSoundListener listener;
	audio.SetListener( listener );
	hfAudioRid voice = audio.CreateEmitter( emitter );   // RID-handled
	for ( int frame = 0; ; frame++ ) {
		audio.UpdateEmitterPosition( voice, pos );       // from the sim snapshot
		audio.Kick();                                    // worker mixes frame N
		hfAudioSnapshot mix;
		audio.CopySnapshot( &mix );                      // frame N-1
		... apply mix.voices[].leftGain / rightGain to the device ...
		audio.WaitForStep();
	}
	audio.Stop();

===============================================================================
*/

#include <string.h>

#include "../hfThread.h"
#include "../hfHeadless.h"
#include "../math/hfVector.h"
#include "hfAudioData.h"
#include "hfAudioSpatial.h"

namespace HF_Audio {

static const int HF_AUDIO_MAX_SERVERS	= 8;
static const int HF_AUDIO_MAX_SAMPLES	= 256;
static const int HF_AUDIO_MAX_EMITTERS	= 256;
static const int HF_AUDIO_MAX_VOICES	= 256;

// handle to a registered resource or server, same shape as hfRid / hfSimRid
struct hfAudioRid {
	int		index;
	HF_INLINE hfAudioRid() : index( -1 ) {}
	HF_INLINE bool Valid() const { return index >= 0; }
};

// one registered emitter: settings (front end) plus a position the front end
// refreshes from the sim snapshot before each kick.
struct hfAudioEmitterEntry {
	hfSpatialEmitter	emitter;
	hfAudioRid			sample;			// resolved sample resource
	bool				alive;
};

// one resolved voice in a published mix: no pointers, safe to copy out while
// the worker builds the next frame.
struct hfAudioVoice {
	hfAudioRid			emitter;
	unsigned int		key;			// hfSoundKey pairing
	unsigned int		sampleId;		// bank sample registry id
	hfVec3				position;
	float				leftGain;
	float				rightGain;
	float				pan;
	float				distance;
	float				gain;			// 0..1 distance+volume attenuated
	bool				audible;
};

struct hfAudioSnapshot {
	unsigned int		sequence;
	hfVec3				listenerPosition;
	int					voiceCount;
	int					audibleCount;
	hfAudioVoice		voices[ HF_AUDIO_MAX_VOICES ];

	// FNV-1a over every byte before it. Must stay the last member.
	unsigned int		checksum;

	HF_INLINE void Zero() { memset( this, 0, sizeof( *this ) ); }
	HF_INLINE unsigned int ComputeChecksum() const {
		const unsigned char *base = (const unsigned char *)this;
		unsigned int n = (unsigned int)( (const unsigned char *)&checksum - base );
		unsigned int h = 2166136261u;
		for ( unsigned int i = 0; i < n; i++ ) {
			h ^= (unsigned int)base[i];
			h *= 16777619u;
		}
		return h;
	}
};

struct hfAudioConfig {
	bool				threaded;		// false = Kick() runs inline
	HF_INLINE hfAudioConfig() : threaded( true ) {}
};

/*
The front end owns the emitters; the server owns the mix. Register a sample
and emitters, feed in a listener, then only read snapshots until Stop().
*/
class hfAudioServer : public hfSysThread {
public:
	HF_INLINE hfAudioServer( const hfAudioConfig &cfg )
		: m_cfg( cfg ), m_rid(), m_sampleCount( 0 ), m_emitterCount( 0 ),
		  m_sequence( 0 ), m_started( false ) {
		m_published.Zero();
		m_rid = Register( this );
	}

	HF_INLINE ~hfAudioServer() {
		Stop();
		Unregister();
	}

	// ---- RID registry (mirrors hfSimulationServer / hfRenderingServer) ----

	static hfAudioServer **Slots() {
		static hfAudioServer *s_slots[ HF_AUDIO_MAX_SERVERS ];
		return s_slots;
	}
	static int &CountRef() { static int s_count = 0; return s_count; }

	static hfAudioRid Register( hfAudioServer *server ) {
		hfAudioRid r;
		int &count = CountRef();
		if ( !server ) { return r; }
		int slot = -1;
		for ( int i = 0; i < count; i++ ) {
			if ( Slots()[ i ] == NULL ) { slot = i; break; }
		}
		if ( slot < 0 ) {
			if ( count >= HF_AUDIO_MAX_SERVERS ) { return r; }
			slot = count++;
		}
		r.index = slot;
		Slots()[ slot ] = server;
		return r;
	}
	HF_INLINE void Unregister() {
		if ( !m_rid.Valid() ) { return; }
		if ( m_rid.index >= 0 && m_rid.index < HF_AUDIO_MAX_SERVERS ) { Slots()[ m_rid.index ] = NULL; }
		m_rid.index = -1;
	}
	static HF_INLINE hfAudioServer *Resolve( const hfAudioRid &r ) {
		return ( r.Valid() && r.index < HF_AUDIO_MAX_SERVERS && r.index < CountRef() ) ?
				Slots()[ r.index ] : NULL;
	}
	static HF_INLINE int Count() { return CountRef(); }
	HF_INLINE hfAudioRid GetRid() const { return m_rid; }

	// ---- sample resources (RID-handled) ----

	HF_INLINE hfAudioRid CreateSample( unsigned int nameHash, const hfAudioSampleInfo &info ) {
		hfAudioRid r;
		for ( int i = 0; i < m_sampleCount; i++ ) {
			if ( m_sampleHashes[i] == nameHash ) { r.index = i; return r; }
		}
		if ( m_sampleCount >= HF_AUDIO_MAX_SAMPLES ) { return r; }
		m_sampleHashes[ m_sampleCount ] = nameHash;
		m_sampleInfos[ m_sampleCount ] = info;
		r.index = m_sampleCount++;
		return r;
	}
	HF_INLINE const hfAudioSampleInfo *GetSample( const hfAudioRid &r ) const {
		return ( r.index >= 0 && r.index < m_sampleCount ) ? &m_sampleInfos[ r.index ] : NULL;
	}
	HF_INLINE int NumSamples() const { return m_sampleCount; }

	// ---- emitter resources (RID-handled) ----

	HF_INLINE hfAudioRid CreateEmitter( const hfSpatialEmitter &e, const hfAudioRid &sample = hfAudioRid() ) {
		hfAudioRid r;
		if ( m_emitterCount >= HF_AUDIO_MAX_EMITTERS ) { return r; }
		m_emitters[ m_emitterCount ].emitter = e;
		m_emitters[ m_emitterCount ].sample = sample;
		m_emitters[ m_emitterCount ].alive = true;
		r.index = m_emitterCount++;
		return r;
	}
	HF_INLINE hfSpatialEmitter *GetEmitter( const hfAudioRid &r ) {
		return ( r.index >= 0 && r.index < m_emitterCount && m_emitters[ r.index ].alive )
				? &m_emitters[ r.index ].emitter : NULL;
	}
	HF_INLINE void SetEmitterPosition( const hfAudioRid &r, const hfVec3 &pos ) {
		hfSpatialEmitter *e = GetEmitter( r );
		if ( e ) { e->position = pos; }
	}
	HF_INLINE void SetEmitterPlaying( const hfAudioRid &r, bool play ) {
		hfSpatialEmitter *e = GetEmitter( r );
		if ( e ) { e->play = play; }
	}
	HF_INLINE void FreeEmitter( const hfAudioRid &r ) {
		if ( r.index >= 0 && r.index < m_emitterCount ) {
			m_emitters[ r.index ].alive = false;
			m_emitters[ r.index ].emitter.play = false;
		}
	}
	HF_INLINE int NumEmitters() const { return m_emitterCount; }

	// ---- listener (front end, before each kick) ----

	HF_INLINE void SetListener( const hfSoundListener &listener ) { m_listener = listener; }
	HF_INLINE void SetListenerPose( const hfVec3 &pos, const hfVec3 &forward ) {
		m_listener.SetPositionForward( pos, forward );
	}
	HF_INLINE const hfSoundListener &GetListener() const { return m_listener; }

	// ---- lifecycle ----

	HF_INLINE void Start() {
		if ( m_started || !m_cfg.threaded ) { return; }
		m_started = StartWorkerThread( "audioServer", CORE_ANY );
	}
	HF_INLINE void Stop() {
		if ( !m_started ) { return; }
		StopThread( true );
		m_started = false;
	}
	HF_INLINE bool IsStarted() const { return m_started; }
	HF_INLINE bool IsHeadless() const { return HF_Lib::IsHeadless(); }

	// ---- one fixed mix frame ----

	HF_INLINE void Kick() {
		if ( m_started ) { SignalWork(); return; }
		MixFrame();
	}
	HF_INLINE void WaitForStep() {
		if ( m_started ) { WaitForThread(); }
	}
	HF_INLINE void StepNow() { Kick(); WaitForStep(); }

	// ---- consumption ----

	HF_INLINE bool CopySnapshot( hfAudioSnapshot *out ) const {
		if ( !out ) { return false; }
		hfScopedCriticalSection lock( m_lock );
		if ( m_sequence == 0 ) { return false; }
		*out = m_published;
		return true;
	}
	HF_INLINE unsigned int GetSequence() const {
		hfScopedCriticalSection lock( m_lock );
		return m_sequence;
	}

protected:
	virtual int Run() {
		MixFrame();
		return 0;
	}

private:
	// Evaluates every live emitter against the listener and publishes the
	// result. Pure function of listener + emitters: no clocks, no randomness,
	// so the same inputs always compute the same mix on any thread.
	HF_INLINE void MixFrame() {
		hfAudioSnapshot snap;
		snap.Zero();
		snap.sequence = m_sequence + 1;
		snap.listenerPosition = m_listener.position;

		for ( int i = 0; i < m_emitterCount && snap.voiceCount < HF_AUDIO_MAX_VOICES; i++ ) {
			if ( !m_emitters[i].alive ) { continue; }
			const hfSpatialEmitter &e = m_emitters[i].emitter;

			hfAudioVoice &v = snap.voices[ snap.voiceCount ];
			v.emitter	= hfAudioRid();
			v.emitter.index = i;
			v.key		= e.key;
			v.sampleId	= e.sampleId;
			v.position	= e.position;
			float left = 0.0f, right = 0.0f, pan = 0.0f;
			float gain = hfSpatialEvaluate( m_listener, e, left, right, pan );
			v.leftGain	= left;
			v.rightGain	= right;
			v.pan		= pan;
			v.distance	= ( e.position - m_listener.position ).Length();
			v.gain		= gain;
			v.audible	= ( left > 0.0f || right > 0.0f );
			if ( v.audible ) { snap.audibleCount++; }
			snap.voiceCount++;
		}

		snap.checksum = snap.ComputeChecksum();

		hfScopedCriticalSection lock( m_lock );
		m_published = snap;
		m_sequence = snap.sequence;
	}

	hfAudioConfig			m_cfg;
	hfAudioRid				m_rid;
	hfSoundListener			m_listener;

	unsigned int			m_sampleHashes[ HF_AUDIO_MAX_SAMPLES ];
	hfAudioSampleInfo		m_sampleInfos[ HF_AUDIO_MAX_SAMPLES ];
	int						m_sampleCount;

	hfAudioEmitterEntry		m_emitters[ HF_AUDIO_MAX_EMITTERS ];
	int						m_emitterCount;

	hfAudioSnapshot			m_published;
	unsigned int			m_sequence;
	bool					m_started;
	mutable hfSysMutex		m_lock;
};

} // namespace HF_Audio

#endif