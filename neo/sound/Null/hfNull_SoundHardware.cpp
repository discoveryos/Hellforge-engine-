/*
===========================================================================

        Hellforge Engine - Cross-platform game engine
        Powered by Hellforge Power 1
        Copyright (C) 1993-2026

        Null sound hardware implementation - portable stub.
        No audio output device is opened; this provides the interface
        contract that the rest of the sound system expects.

===========================================================================
*/

#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "../hfsnd_local.h"

extern hfCVar s_volume_dB;

/*
========================
hfSoundHardware_Null::hfSoundHardware_Null
========================
*/
hfSoundHardware_Null::hfSoundHardware_Null() {
	zombieVoices.SetNum( 0 );
	freeVoices.SetNum( 0 );
}

/*
========================
hfSoundHardware_Null::~hfSoundHardware_Null
========================
*/
hfSoundHardware_Null::~hfSoundHardware_Null() {
	Shutdown();
}

void listDevices_f( const hfCmdArgs & args );

/*
========================
hfSoundHardware_Null::Init
========================
*/
void hfSoundHardware_Null::Init() {
	cmdSystem->AddCommand( "listDevices", listDevices_f, 0, "Lists the connected sound devices", NULL );

	hfLib::Printf( "Null audio backend: no output device opened.\n" );

	hfSoundVoice::InitSurround( 2, 3 ); // stereo output
}

void listDevices_f( const hfCmdArgs & args ) {
	hfLib::Printf( "Null audio backend: no output devices are available.\n" );
}

/*
========================
hfSoundHardware_Null::Shutdown
========================
*/
void hfSoundHardware_Null::Shutdown() {
	for ( int i = 0; i < zombieVoices.Num(); i++ ) {
		delete zombieVoices[i];
	}
	zombieVoices.Clear();

	for ( int i = 0; i < freeVoices.Num(); i++ ) {
		delete freeVoices[i];
	}
	freeVoices.Clear();
}

/*
========================
hfSoundHardware_Null::AllocateVoice
========================
*/
hfSoundVoice * hfSoundHardware_Null::AllocateVoice( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample ) {
	if ( leadinSample == NULL ) {
		return NULL;
	}

	hfSoundVoice_Null * voice = new hfSoundVoice_Null();

	// start the voice with the samples
	voice->Start( leadinSample, loopingSample );

	return (hfSoundVoice *)voice;
}

/*
========================
hfSoundHardware_Null::FreeVoice
========================
*/
void hfSoundHardware_Null::FreeVoice( hfSoundVoice * voice ) {
	voice->Stop();
	zombieVoices.Append( (hfSoundVoice_Null *)voice );
}

/*
========================
hfSoundHardware_Null::Update
========================
*/
void hfSoundHardware_Null::Update() {
	// move any zombie voices to the free list
	for ( int i = 0; i < zombieVoices.Num(); i++ ) {
		if ( !zombieVoices[i]->IsPlaying() ) {
			freeVoices.Append( zombieVoices[i] );
			zombieVoices.RemoveIndexFast( i );
			i--;
		}
	}
}