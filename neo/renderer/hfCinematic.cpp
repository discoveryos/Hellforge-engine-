#pragma hdrstop
#include "../hflib/hfprecompiled.h"


extern hfCVar s_noSound;

#include "hftr_local.h"

//===========================================

/*
==============
hfCinematic::InitCinematic
==============
*/
void hfCinematic::InitCinematic( void ) {
}

/*
==============
hfCinematic::ShutdownCinematic
==============
*/
void hfCinematic::ShutdownCinematic( void ) {
}

/*
==============
hfCinematic::Alloc
==============
*/
hfCinematic * hfCinematic::Alloc() {
	return new hfCinematic;
}

/*
==============
hfCinematic::~hfCinematic
==============
*/
hfCinematic::~hfCinematic( ) {
	Close();
}

/*
==============
hfCinematic::InitFromFile
==============
*/
bool hfCinematic::InitFromFile( const char *qpath, bool looping ) {
	return false;
}

/*
==============
hfCinematic::AnimationLength
==============
*/
int hfCinematic::AnimationLength() {
	return 0;
}

/*
==============
hfCinematic::GetStartTime
==============
*/
int hfCinematic::GetStartTime() {
	return -1;
}

/*
==============
hfCinematic::ResetTime
==============
*/
void hfCinematic::ResetTime(int milliseconds) {
}

/*
==============
hfCinematic::ImageForTime
==============
*/
cinData_t hfCinematic::ImageForTime( int milliseconds ) {
	cinData_t c;
	memset( &c, 0, sizeof( c ) );
	return c;
}

/*
==============
hfCinematic::ExportToTGA
==============
*/
void hfCinematic::ExportToTGA( bool skipExisting ) {
}

/*
==============
hfCinematic::GetFrameRate
==============
*/
float hfCinematic::GetFrameRate() const {
	return 30.0f;
}

/*
==============
hfCinematic::Close
==============
*/
void hfCinematic::Close() {
}

/*
==============
hfSndWindow::InitFromFile
==============
*/
bool hfSndWindow::InitFromFile( const char *qpath, bool looping ) {
	hfStr fname = qpath;

	fname.ToLower();
	if ( !fname.Icmp( "waveform" ) ) {
		showWaveform = true;
	} else {
		showWaveform = false;
	}
	return true;
}

/*
==============
hfSndWindow::ImageForTime
==============
*/
cinData_t hfSndWindow::ImageForTime( int milliseconds ) {
	return soundSystem->ImageForTime( milliseconds, showWaveform );
}

/*
==============
hfSndWindow::AnimationLength
==============
*/
int hfSndWindow::AnimationLength() {
	return -1;
}

