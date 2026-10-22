#ifndef __HF_AUDIO_SPATIAL_H__
#define __HF_AUDIO_SPATIAL_H__

/*
======================================================================
	Positional audio layer.

	Fresh implementation modelled on the source/listener arrangement
	used by SynapseEngine (AudioSourceComponent / AudioListenerComponent
	fields: soundIndex, play, loop, isSpatialized, volume, pitch,
	minDistance, maxDistance) but expressed in Hellforge's idioms and
	built on the already-tested hfAttenuationGain / hfPanGains maths in
	hfAudioData.h.

	Like hfAudioData.h this stays pure headless: no OS / miniaudio /
	cvar dependencies, so the offline harness exercises the exact code
	the runtime engine drives.  Include hfprecompiled.h first (for
	hfVec3 / HF_INLINE).

	The runtime device backend (hfAudioManager) feeds a listener and the
	entity sound tables into hfSpatialEvaluate() and applies the
	returned per-channel gains to the matching ma_sound node.
======================================================================
*/

#include "hfAudioData.h"

namespace HF_Audio {

// Stereo decode of a world direction into the listener's frame.
// Returns the signed lateral component in [-1,1] (-1 = far left,
// +1 = far right); the front/back component is packed into 'frontBack'
// for optional per-doppler or rear-attenuation use.
HF_INLINE float hfSpatialDecode( const hfVec3 &relativeDirNormalized, const hfVec3 &listenerRight, const hfVec3 &listenerForward, float &frontBack ) {
	float lateral = relativeDirNormalized * listenerRight;
	frontBack = relativeDirNormalized * listenerForward;
	if ( lateral > 1.0f ) { lateral = 1.0f; }
	if ( lateral < -1.0f ) { lateral = -1.0f; }
	return lateral;
}

// Listener pose: position + orthonormal forward/right basis.
class hfSoundListener {
public:
	hfVec3			position;
	hfVec3			forward;		// unit, world space
	hfVec3			right;			// unit, world space

	HF_INLINE hfSoundListener() {
		position = hfVec3( 0.0f, 0.0f, 0.0f );
		forward = hfVec3( 0.0f, 1.0f, 0.0f );
		right = hfVec3( 1.0f, 0.0f, 0.0f );
	}

	// Build the basis from an eye position and a look-at target with
	// an arbitrary world up hint (mirrors the listener orientation in
	// Synapse's AudioListenerComponent).
	HF_INLINE void SetPositionForward( const hfVec3 &pos, const hfVec3 &fwd ) {
		position = pos;
		if ( fwd.LengthSqr() <= 0.000001f ) {
			forward = hfVec3( 0.0f, 1.0f, 0.0f );
		} else {
			forward = fwd;
			forward.Normalize();
		}
		right = forward.Cross( hfVec3( 0.0f, 0.0f, 1.0f ) );
		if ( right.LengthSqr() <= 0.000001f ) {
			right = forward.Cross( hfVec3( 1.0f, 0.0f, 0.0f ) );
		}
		right.Normalize();
	}

	HF_INLINE void SetPositionLookAt( const hfVec3 &pos, const hfVec3 &lookAt ) {
		SetPositionForward( pos, lookAt - pos );
	}
};

// Per-emitter settings, mirroring the SynapseEngine source component
// fields (position added, volume/pitch re-expressed).
class hfSpatialEmitter {
public:
	unsigned int	key;			// hfSoundKey pairing with hfSoundBank slots
	unsigned int	sampleId;		// bank sample registry id
	bool			play;
	bool			loop;
	bool			spatialized;	// isSpatialized
	float			volume;
	float			pitch;
	float			minDistance;
	float			maxDistance;
	hfAttenModel	attenuationModel;
	hfVec3			position;

	HF_INLINE hfSpatialEmitter() {
		key = 0;
		sampleId = 0;
		play = false;
		loop = false;
		spatialized = false;
		volume = 1.0f;
		pitch = 1.0f;
		minDistance = 40.0f;
		maxDistance = 800.0f;
		attenuationModel = HF_ATTEN_LINEAR;
		position = hfVec3( 0.0f, 0.0f, 0.0f );
	}
};

// Full spatial evaluation for one emitter against the listener.
// Returns the final left/right mixer gains (already volume- and
// distance-attenuated) and the raw 0..1 distance gain.
HF_INLINE float hfSpatialEvaluate( const hfSoundListener &listener, const hfSpatialEmitter &emitter, float &leftGain, float &rightGain, float &pan ) {
	leftGain = rightGain = 0.0f;	// muted until proven audible
	pan = 0.0f;
	if ( !emitter.play || emitter.volume <= 0.0f ) {
		return 0.0f;
	}

	hfVec3 relative = emitter.position - listener.position;
	float distance = relative.Length();
	if ( distance <= 0.000001f ) {
		relative = hfVec3( 0.0f, 0.0f, 0.0f );
		distance = 0.0f;
	}

	float gain = emitter.volume;
	if ( emitter.spatialized ) {
		gain *= hfAttenuationGain( emitter.attenuationModel, distance, emitter.minDistance, emitter.maxDistance );
	}

	if ( !emitter.spatialized || distance <= 0.000001f ) {
		hfPanGains( 0.0f, leftGain, rightGain );
	} else {
		hfVec3 dir = relative;
		dir.Normalize();
		float frontBack = 0.0f;
		pan = hfSpatialDecode( dir, listener.right, listener.forward, frontBack );
		hfPanGains( pan, leftGain, rightGain );
	}
	leftGain *= gain;
	rightGain *= gain;
	return gain;
}

} // namespace HF_Audio

#endif