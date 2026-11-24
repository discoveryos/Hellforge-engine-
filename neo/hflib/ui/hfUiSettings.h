#ifndef __HF_UI_SETTINGS_H__
#define __HF_UI_SETTINGS_H__

/*
===============================================================================

	Phase 17 — hfUiSettings.

	The durable half of the options screen: a plain struct with a text format,
	read and written through the Phase 13 per-account console storage.  That
	means settings follow the signed-in user on a console instead of living in
	a shared desktop config file, which is exactly what the platform storage
	seam exists for.

	Text format (unknown keys are ignored so an older/newer file still loads):

		music_volume      = 70
		sfx_volume        = 85
		mouse_sensitivity = 1.0
		width             = 1280
		height            = 720
		quality           = 2
		fullscreen        = 1
		vsync             = 1

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../framework/hfCmdSystem.h"
#include "../framework/hfCVarSystem.h"
#include "../console/hfConsoleServices.h"
#include "hfUiFrontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace HF_Ui {

using hfconsole::hfConsoleServices;
using hfconsole::hfConsoleUserId;

struct hfUiSettings {
	enum {
		HF_UI_SETTINGS_TEXT_MAX = 1024,
		HF_UI_SETTINGS_SLOT = 0
	};

	float	musicVolume;
	float	sfxVolume;
	float	mouseSensitivity;
	int		width;
	int		height;
	int		quality;
	bool	fullscreen;
	bool	vsync;

	hfUiSettings() { SetDefaults(); }

	void SetDefaults() {
		musicVolume = 70.0f;
		sfxVolume = 85.0f;
		mouseSensitivity = 1.0f;
		width = 1280;
		height = 720;
		quality = 2;
		fullscreen = false;
		vsync = true;
	}

	int Serialize( char *out, int maxSize ) const {
		if ( out == NULL || maxSize <= 0 ) { return 0; }
		snprintf( out, ( size_t )maxSize,
				  "music_volume = %g\n"
				  "sfx_volume = %g\n"
				  "mouse_sensitivity = %g\n"
				  "width = %d\n"
				  "height = %d\n"
				  "quality = %d\n"
				  "fullscreen = %d\n"
				  "vsync = %d\n",
				  musicVolume, sfxVolume, mouseSensitivity, width, height, quality,
				  fullscreen ? 1 : 0, vsync ? 1 : 0 );
		return ( int )strlen( out );
	}

	bool Parse( const char *text ) {
		if ( text == NULL ) { return false; }
		int recognized = 0;
		const char *p = text;
		while ( *p != '\0' ) {
			const char *lineEnd = p;
			while ( *lineEnd != '\0' && *lineEnd != '\n' ) { lineEnd++; }

			char line[ 256 ];
			int len = ( int )( lineEnd - p );
			if ( len > ( int )sizeof( line ) - 1 ) { len = ( int )sizeof( line ) - 1; }
			memcpy( line, p, len );
			line[ len ] = '\0';
			p = ( *lineEnd == '\n' ) ? lineEnd + 1 : lineEnd;

			char *comment = strchr( line, '#' );
			if ( comment != NULL ) { *comment = '\0'; }

			char *eq = strchr( line, '=' );
			if ( eq == NULL ) { continue; }
			*eq = '\0';
			char *key = Trim( line );
			char *value = Trim( eq + 1 );
			if ( *key == '\0' || *value == '\0' ) { continue; }

			if ( strcmp( key, "music_volume" ) == 0 ) { musicVolume = ( float )atof( value ); recognized++; }
			else if ( strcmp( key, "sfx_volume" ) == 0 ) { sfxVolume = ( float )atof( value ); recognized++; }
			else if ( strcmp( key, "mouse_sensitivity" ) == 0 ) { mouseSensitivity = ( float )atof( value ); recognized++; }
			else if ( strcmp( key, "width" ) == 0 ) { width = atoi( value ); recognized++; }
			else if ( strcmp( key, "height" ) == 0 ) { height = atoi( value ); recognized++; }
			else if ( strcmp( key, "quality" ) == 0 ) { quality = atoi( value ); recognized++; }
			else if ( strcmp( key, "fullscreen" ) == 0 ) { fullscreen = atoi( value ) != 0; recognized++; }
			else if ( strcmp( key, "vsync" ) == 0 ) { vsync = atoi( value ) != 0; recognized++; }
		}

		Clamp();
		return recognized > 0;
	}

	void Clamp() {
		if ( musicVolume < 0.0f ) { musicVolume = 0.0f; }
		if ( musicVolume > 100.0f ) { musicVolume = 100.0f; }
		if ( sfxVolume < 0.0f ) { sfxVolume = 0.0f; }
		if ( sfxVolume > 100.0f ) { sfxVolume = 100.0f; }
		if ( mouseSensitivity < 0.1f ) { mouseSensitivity = 0.1f; }
		if ( mouseSensitivity > 10.0f ) { mouseSensitivity = 10.0f; }
		if ( width < 320 ) { width = 320; }
		if ( height < 200 ) { height = 200; }
		if ( quality < 0 ) { quality = 0; }
		if ( quality > 3 ) { quality = 3; }
	}

	// Mirror into / out of the options screen.
	void WriteTo( hfUiFrontend &frontend ) const {
		frontend.SetMusicVolume( musicVolume );
		frontend.SetSfxVolume( sfxVolume );
		frontend.SetFullscreen( fullscreen );
	}

	void ReadFrom( const hfUiFrontend &frontend ) {
		musicVolume = frontend.GetMusicVolume();
		sfxVolume = frontend.GetSfxVolume();
		fullscreen = frontend.GetFullscreen();
		Clamp();
	}

	// --- engine application ----------------------------------------------

	struct hfUiCvarValues {
		float	volumeDb;
		float	sensitivity;
		int		fullscreen;
		int		width, height, vidMode;
		int		quality;
		int		swapInterval;
	};

	// Pure mapping so it can be checked in a headless test with no cvar
	// system installed.  The single engine master-volume cvar is driven by
	// the average of the two UI sliders, in dB (100% == 0 dB).
	void ComputeCvarValues( hfUiCvarValues &out ) const {
		const float master = ClampF( ( musicVolume + sfxVolume ) * 0.5f, 0.0f, 100.0f );
		out.volumeDb = master <= 0.0f ? -60.0f : 20.0f * log10f( master / 100.0f );
		out.volumeDb = ClampF( out.volumeDb, -60.0f, 0.0f );
		out.sensitivity = ClampF( mouseSensitivity, 0.1f, 10.0f );

		out.fullscreen = fullscreen ? 1 : 0;
		out.width = ( int )ClampF( ( float )width, 320.0f, 4096.0f );
		out.height = ( int )ClampF( ( float )height, 200.0f, 4096.0f );
		out.vidMode = -1;	// -1 selects r_customWidth / r_customHeight
		out.quality = ( int )ClampF( ( float )quality, 0.0f, 3.0f );
		out.swapInterval = vsync ? 1 : 0;
	}

	bool ApplyToCVars( hfCVarSystem *cvars ) const {
		if ( cvars == NULL ) { return false; }
		hfUiCvarValues v;
		ComputeCvarValues( v );
		cvars->SetCVarFloat( "s_volume_dB", v.volumeDb );
		cvars->SetCVarFloat( "sensitivity", v.sensitivity );
		cvars->SetCVarInteger( "r_fullscreen", v.fullscreen );
		cvars->SetCVarInteger( "r_customWidth", v.width );
		cvars->SetCVarInteger( "r_customHeight", v.height );
		cvars->SetCVarInteger( "r_vidMode", v.vidMode );
		cvars->SetCVarInteger( "opt_applyPreset", v.quality );
		cvars->SetCVarInteger( "r_swapInterval", v.swapInterval );
		return true;
	}

	bool ApplyToCVars() const { return ApplyToCVars( hfLib::cvarSystem ); }

	// --- per-account storage ---------------------------------------------

	bool SaveToSlot( hfConsoleServices *services, hfConsoleUserId user,
					 const char *slot = "settings" ) const {
		if ( services == NULL ) { return false; }
		char text[ HF_UI_SETTINGS_TEXT_MAX ];
		const int length = Serialize( text, sizeof( text ) );
		return services->WriteSlot( user, slot, ( const byte * )text, length );
	}

	bool LoadFromSlot( hfConsoleServices *services, hfConsoleUserId user,
					   const char *slot = "settings" ) {
		if ( services == NULL ) { return false; }
		const int size = services->SlotSize( user, slot );
		if ( size <= 0 ) { return false; }
		int readSize = size;
		if ( readSize > HF_UI_SETTINGS_TEXT_MAX - 1 ) { readSize = HF_UI_SETTINGS_TEXT_MAX - 1; }
		char text[ HF_UI_SETTINGS_TEXT_MAX ];
		const int got = services->ReadSlot( user, slot, ( byte * )text, readSize );
		if ( got <= 0 ) { return false; }
		text[ got ] = '\0';
		return Parse( text );
	}

private:
	static float ClampF( float v, float lo, float hi ) {
		return v < lo ? lo : ( v > hi ? hi : v );
	}

	static char *Trim( char *s ) {
		while ( *s == ' ' || *s == '\t' ) { s++; }
		int len = ( int )strlen( s );
		while ( len > 0 && ( s[ len - 1 ] == ' ' || s[ len - 1 ] == '\t' || s[ len - 1 ] == '\r' ) ) {
			s[ --len ] = '\0';
		}
		return s;
	}
};

} // namespace HF_Ui

#endif
