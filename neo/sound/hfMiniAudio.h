#ifndef __HF_MINI_AUDIO_H__
#define __HF_MINI_AUDIO_H__

#include "../hflib/audio/hfAudioData.h"
#include <map>

// Modern miniaudio-backed playback engine. Fresh implementation using the
// same architecture as the reference integration being replaced: an ma_engine
// with every active sound keyed by an engine entity id, volume/pitch/loop/
// spatialization refreshed on each update tick, plus a non-spatialized
// preview player with play/pause/seek/time queries.
//
// Keeps miniaudio out of the public header (opaque handles). All miniaudio
// types live in hfMiniAudio.cpp / hfMiniAudio() implementation TU.

namespace HF_Audio {

struct hfPlayParams {
	HF_INLINE hfPlayParams() : play( false ), loop( false ), volume( 1.0f ), pitch( 1.0f ),
		isSpatialized( true ), minDistance( 1.0f ), maxDistance( 100.0f ) {}
	bool		play;
	bool		loop;
	float		volume;
	float		pitch;
	bool		isSpatialized;
	float		minDistance;
	float		maxDistance;
};

class hfMiniAudioEngine {
public:
	hfMiniAudioEngine();
	~hfMiniAudioEngine();

	void			Init( unsigned sampleRate = 48000 );
	void			Shutdown();
	bool			IsInitialized() const { return m_engine != NULL; }

	void			SetListenerTransform( const float pos[3], const float fwd[3], const float up[3] );

	// Creates (or updates) the sound bound to `key`. Pass samples=NULL to
	// stop. Samples are interleaved float32 PCM.
	void			UpdateSound( unsigned key, const hfAudioSampleInfo &info, const float *samples,
								const hfPlayParams &params, const float pos[3] );
	void			StopSound( unsigned key );
	void			StopAllSounds();

	// ---- preview player (non-spatialized) ----
	void			PlayPreview( const hfAudioSampleInfo &info, const float *samples );
	void			PausePreview();
	void			StopPreview();
	void			SetPreviewTime( float seconds );
	bool			IsPreviewPlaying() const;
	float			GetPreviewTime() const;

private:
	struct Active {
		void *		buffer;		// ma_audio_buffer*
		void *		sound;		// ma_sound*
		bool		isInitialized;
		Active() : buffer( NULL ), sound( NULL ), isInitialized( false ) {}
	};

	void *							m_engine;			// ma_engine*
	unsigned						m_sampleRate;
	float							m_previewRate;
	std::map<unsigned, Active>		m_active;
	Active							m_preview;

	static void *EngineAlloc( unsigned int size );
	static void EngineFree( void *ptr );
};

// engine singleton + lifecycle helpers used by console startup
extern hfMiniAudioEngine *hfAudioEngine;
void hfAudioEngine_Init();
void hfAudioEngine_Shutdown();

} // namespace HF_Audio

#endif