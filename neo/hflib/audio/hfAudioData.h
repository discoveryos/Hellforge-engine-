#ifndef __HF_AUDIO_DATA_H__
#define __HF_AUDIO_DATA_H__

#include "../hfStr.h"

// Pure audio primitives shared by the runtime engine, the resource manager
// and the headless harness. No OS / device / miniaudio dependency: everything
// here is plain math and small containers so the offline tests exercise the
// same code the engine runs.

namespace HF_Audio {

// ---------------- sample meta ----------------
struct hfAudioSampleInfo {
	unsigned int	channels;		// 1 = mono, 2 = stereo
	unsigned int	sampleRate;		// Hz
	unsigned int	totalFrames;	// per channel
	unsigned int	totalBytes;		// interleaved f32 frames

	HF_INLINE float DurationSec() const { return sampleRate ? ( float )totalFrames / ( float )sampleRate : 0.0f; }
	HF_INLINE unsigned int FrameSize() const { return channels * sizeof( float ); }
};

// ---------------- distance attenuation ----------------
// Mirrors the classic distance models (linear / inverse clamp / inverse tanged)
// without depending on miniaudio, so the attenuation math is unit-testable.
enum hfAttenModel {
	HF_ATTEN_LINEAR = 0,
	HF_ATTEN_INVERSE_CLAMPED,
	HF_ATTEN_INVERSE_TANGLED,
};

// Returns a 0..1 gain for a point that is 'distance' from the listener.
//   linear:        1 while d <= min, eases toward 0 at max
//   inverse clamp: 1 / max(1, (d - min) / (max - min))
//   tangled/quad:  smoothstep between min and max
HF_INLINE float hfAttenuationGain( hfAttenModel model, float distance, float minDistance, float maxDistance ) {
	if ( distance <= minDistance ) { return 1.0f; }
	if ( distance >= maxDistance || maxDistance <= minDistance ) { return 0.0f; }
	if ( minDistance < 0.0f ) { minDistance = 0.0f; }
	if ( maxDistance < 0.0f ) { maxDistance = 0.0f; }
	float t = ( distance - minDistance ) / ( maxDistance - minDistance );
	if ( t > 1.0f ) { t = 1.0f; }
	switch ( model ) {
		case HF_ATTEN_LINEAR:			return 1.0f - t;
		case HF_ATTEN_INVERSE_CLAMPED:	return 1.0f / ( 1.0f + t * ( maxDistance - minDistance ) );
		case HF_ATTEN_INVERSE_TANGLED: {
			float s = t * t * ( 3.0f - 2.0f * t );	// smoothstep
			return 1.0f - s;
		}
	}
	return 1.0f - t;
}

// Simple stereo pan: -1 (left) .. 1 (right) -> { left, right } gains.
HF_INLINE void hfPanGains( float pan, float &left, float &right ) {
	left = ( pan <= 0.0f ) ? 1.0f : ( 1.0f - pan );
	right = ( pan >= 0.0f ) ? 1.0f : ( 1.0f + pan );
}

// ---------------- entity keys (generation-tagged, like the ECS) ----------------
// A key carries an index and a generation tag so a stale handle never refers
// to a reallocated slot. Used as the per-entity sound key.
class hfSoundKey {
public:
	static const unsigned int INDEX_MASK	= 0xFFFF;
	static const unsigned int GEN_MASK		= 0xFFFF0000;
	static const unsigned int GEN_SHIFT		= 16;

	HF_INLINE static unsigned int Make( unsigned int index, unsigned int gen ) {
		return ( ( gen & 0xFFFF ) << GEN_SHIFT ) | ( index & INDEX_MASK );
	}
	HF_INLINE static unsigned int IndexOf( unsigned int key ) { return key & INDEX_MASK; }
	HF_INLINE static unsigned int GenOf( unsigned int key ) { return ( key >> GEN_SHIFT ) & 0xFFFF; }
};

// Fixed-slot allocation table for sound sources (plays the role of the
// entity->ActiveSound map in the runtime engine, held trivially here).
class hfSoundBank {
public:
	struct Slot {
		unsigned int	key;			// hfSoundKey
		unsigned int	sampleId;		// index into the sample array
		unsigned char	generation;
		bool			alive;
	};
	enum { MAX_SLOTS = 512 };

	hfSoundBank() { Init(); }

	void			Init();
	unsigned int	Alloc( unsigned int sampleId );		// returns key
	void			Free( unsigned int key );
	bool			IsAlive( unsigned int key ) const;
	Slot *			Get( unsigned int key );
	const Slot *	Get( unsigned int key ) const;

	// sample registry: name -> id (lower-cased)
	unsigned int	AddSample( const char *name );		// returns id
	int				FindSample( const char *name ) const;	// -1 on miss
	int				SampleCount() const { return m_numSamples; }
	const hfStr &	SampleName( unsigned int id ) const { return id < m_numSamples ? m_sampleNames[id] : m_sampleNames[0]; }

private:
	Slot			m_slots[MAX_SLOTS];
	int				m_numSlots;
	int				m_numSamples;
	hfStr			m_sampleNames[256];
};

} // namespace HF_Audio

#endif