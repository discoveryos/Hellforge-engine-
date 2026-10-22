/*
===========================================================================

        Hellforge Engine - Cross-platform game engine
        Powered by Hellforge Power 1
        Copyright (C) 1993-2026

        Null sound voice implementation - portable cross-platform stub.
        No audio output, but retains the voice lifecycle logic for
        future OpenAL or similar backends.

===========================================================================
*/

#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "../hfsnd_local.h"

hfSoundVoice_Null::hfSoundVoice_Null() : hfSoundVoice_Base() {
	playing = false;
	currentAmplitude = 0.0f;
	leadinSampleDone = false;
}

hfSoundVoice_Null::~hfSoundVoice_Null() {
	Stop();
}

void hfSoundVoice_Null::Start( int offsetMS, int ssFlags ) {
	playing = true;
}

void hfSoundVoice_Null::Update() {
}

float hfSoundVoice_Null::GetAmplitude() const {
	return currentAmplitude;
}

void hfSoundVoice_Null::Start( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample ) {
	if ( leadinSample == NULL ) {
		return;
	}

	samples.SetNum( 0 );
	if ( leadinSample != NULL ) {
		samples.Append( const_cast<hfSoundSample *>(leadinSample) );
	}
	if ( loopingSample != NULL ) {
		samples.Append( const_cast<hfSoundSample *>(loopingSample) );
	}

	playing = true;
	leadinSampleDone = false;
}

void hfSoundVoice_Null::Stop() {
	playing = false;
	leadinSampleDone = false;
	samples.SetNum( 0 );
}

void hfSoundVoice_Null::Pause() {
	if ( playing ) {
		playing = false;
	}
}

void hfSoundVoice_Null::UnPause() {
	if ( !playing && samples.Num() > 0 ) {
		playing = true;
	}
}

bool hfSoundVoice_Null::IsPlaying() const {
	return playing;
}

int hfSoundVoice_Null::GetPlayedSampleCount() const {
	if ( samples.Num() == 0 ) {
		return 0;
	}
	return samples[0]->NumSamples();
}

float hfSoundVoice_Null::GetCurrentAmplitude() const {
	return currentAmplitude;
}