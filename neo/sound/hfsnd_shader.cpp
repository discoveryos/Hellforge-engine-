#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfsnd_local.h"

extern hfCVar s_maxSamples;

typedef enum {
	SPEAKER_LEFT = 0,
	SPEAKER_RIGHT,
	SPEAKER_CENTER,
	SPEAKER_LFE,
	SPEAKER_BACKLEFT,
	SPEAKER_BACKRIGHT
} speakerLabel;

/*
===============
hfSoundShader::Init
===============
*/
void hfSoundShader::Init() {
	leadin = false;
	leadinVolume = 0;
	altSound = NULL;
}

/*
===============
hfSoundShader::hfSoundShader
===============
*/
hfSoundShader::hfSoundShader() {
	Init();
}

/*
===============
hfSoundShader::~hfSoundShader
===============
*/
hfSoundShader::~hfSoundShader() {
}

/*
=================
hfSoundShader::Size
=================
*/
size_t hfSoundShader::Size() const {
	return sizeof( hfSoundShader );
}

/*
===============
hfSoundShader::hfSoundShader::FreeData
===============
*/
void hfSoundShader::FreeData() {
}

/*
===================
hfSoundShader::SetDefaultText
===================
*/
bool hfSoundShader::SetDefaultText() {
	hfStr wavname;

	wavname = GetName();
	wavname.DefaultFileExtension( ".wav" );		// if the name has .ogg in it, that will stay

	// if there exists a wav file with the same name
	if ( 1 ) { //fileSystem->ReadFile( wavname, NULL ) != -1 ) {
		char generated[2048];
		hfStr::snPrintf( generated, sizeof( generated ), 
						"sound %s // IMPLICITLY GENERATED\n"
						"{\n"
						"%s\n"
						"}\n", GetName(), wavname.c_str() );
		SetText( generated );
		return true;
	} else {
		return false;
	}
}

/*
===================
DefaultDefinition
===================
*/
const char *hfSoundShader::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"_default.wav\n"
		"}";
}

/*
===============
hfSoundShader::Parse

  this is called by the declManager
===============
*/
bool hfSoundShader::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	hfLexer	src;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	if ( !ParseShader( src ) ) {
		MakeDefault();
		return false;
	}
	return true;
}

/*
===============
hfSoundShader::ParseShader
===============
*/
bool hfSoundShader::ParseShader( hfLexer &src ) {
	hfToken		token;

	parms.minDistance = 1;
	parms.maxDistance = 10;
	parms.volume = 1;
	parms.shakes = 0;
	parms.soundShaderFlags = 0;
	parms.soundClass = 0;

	speakerMask = 0;
	altSound = NULL;

	entries.Clear();

	while ( 1 ) {
		if ( !src.ExpectAnyToken( &token ) ) {
			return false;
		}
		// end of definition
		else if ( token == "}" ) {
			break;
		}
		// minimum number of sounds
		else if ( !token.Icmp( "minSamples" ) ) {
			src.ParseInt();
		}
		// description
		else if ( !token.Icmp( "description" ) ) {
			src.ReadTokenOnLine( &token );
		}
		// mindistance
		else if ( !token.Icmp( "mindistance" ) ) {
			parms.minDistance = src.ParseFloat();
		}
		// maxdistance
		else if ( !token.Icmp( "maxdistance" ) ) {
			parms.maxDistance = src.ParseFloat();
		}
		// shakes screen
		else if ( !token.Icmp( "shakes" ) ) {
			src.ExpectAnyToken( &token );
			if ( token.type == TT_NUMBER ) {
				parms.shakes = token.GetFloatValue();
			} else {
				src.UnreadToken( &token );
				parms.shakes = 1.0f;
			}
		}
		// reverb
		else if ( !token.Icmp( "reverb" ) ) {
			src.ParseFloat();
			if ( !src.ExpectTokenString( "," ) ) {
				src.FreeSource();
				return false;
			}
			src.ParseFloat();
			// no longer supported
		}
		// volume
		else if ( !token.Icmp( "volume" ) ) {
			parms.volume = src.ParseFloat();
		}
		// leadinVolume is used to allow light breaking leadin sounds to be much louder than the broken loop
		else if ( !token.Icmp( "leadinVolume" ) ) {
			leadinVolume = src.ParseFloat();
			leadin = true;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_center" ) ) {
			speakerMask |= 1<<SPEAKER_CENTER;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_left" ) ) {
			speakerMask |= 1<<SPEAKER_LEFT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_right" ) ) {
			speakerMask |= 1<<SPEAKER_RIGHT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_backright" ) ) {
			speakerMask |= 1<<SPEAKER_BACKRIGHT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_backleft" ) ) {
			speakerMask |= 1<<SPEAKER_BACKLEFT;
		}
		// speaker mask
		else if ( !token.Icmp( "mask_lfe" ) ) {
			speakerMask |= 1<<SPEAKER_LFE;
		}
		// soundClass
		else if ( !token.Icmp( "soundClass" ) ) {
			parms.soundClass = src.ParseInt();
			if ( parms.soundClass < 0 || parms.soundClass >= SOUND_MAX_CLASSES ) {
				src.Warning( "SoundClass out of range" );
				return false;
			}
		}
		// altSound
		else if ( !token.Icmp( "altSound" ) ) {
			if ( !src.ExpectAnyToken( &token ) ) {
				return false;
			}
			altSound = declManager->FindSound( token.c_str() );
		}
		// ordered
		else if ( !token.Icmp( "ordered" ) ) {
			// no longer supported
		}
		// no_dups
		else if ( !token.Icmp( "no_dups" ) ) {
			parms.soundShaderFlags |= SSF_NO_DUPS;
		}
		// no_flicker
		else if ( !token.Icmp( "no_flicker" ) ) {
			parms.soundShaderFlags |= SSF_NO_FLICKER;
		}
		// plain
		else if ( !token.Icmp( "plain" ) ) {	
			// no longer supported
		}
		// looping
		else if ( !token.Icmp( "looping" ) ) {
			parms.soundShaderFlags |= SSF_LOOPING;
		}
		// no occlusion
		else if ( !token.Icmp( "no_occlusion" ) ) {
			parms.soundShaderFlags |= SSF_NO_OCCLUSION;
		}
		// private
		else if ( !token.Icmp( "private" ) ) {
			parms.soundShaderFlags |= SSF_PRIVATE_SOUND;
		}
		// antiPrivate
		else if ( !token.Icmp( "antiPrivate" ) ) {
			parms.soundShaderFlags |= SSF_ANTI_PRIVATE_SOUND;
		}
		// once
		else if ( !token.Icmp( "playonce" ) ) {
			parms.soundShaderFlags |= SSF_PLAY_ONCE;
		}
		// global
		else if ( !token.Icmp( "global" ) ) {
			parms.soundShaderFlags |= SSF_GLOBAL;
		}
		// unclamped
		else if ( !token.Icmp( "unclamped" ) ) {
			parms.soundShaderFlags |= SSF_UNCLAMPED;
		}
		// omnidirectional
		else if ( !token.Icmp( "omnidirectional" ) ) {
			parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
		}
		else if ( !token.Icmp( "onDemand" ) ) {
			// no longer loading sounds on demand
		}
		// the wave files
		else if ( !token.Icmp( "leadin" ) ) {
			leadin = true;
		} else if ( token.Find( ".wav", false ) != -1 || token.Find( ".ogg", false ) != -1 ) {
			if ( token.IcmpPrefixPath( "sound/vo/" ) == 0 || token.IcmpPrefixPath( "sound/guis/" ) == 0 ) {
				parms.soundShaderFlags |= SSF_VO;
			}
			if ( token.IcmpPrefixPath( "sound/musical/" ) == 0 ) {
				parms.soundShaderFlags |= SSF_MUSIC;
			}
			// add to the wav list
			if ( s_maxSamples.GetInteger() == 0 || ( s_maxSamples.GetInteger() > 0 && entries.Num() < s_maxSamples.GetInteger() ) ) {
				entries.Append( soundSystemLocal.LoadSample( token.c_str() ) );
			}
		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
===============
hfSoundShader::List
===============
*/
void hfSoundShader::List() const {
	hfStrList	shaders;

	common->Printf( "%4i: %s\n", Index(), GetName() );
	for( int k = 0; k < entries.Num(); k++ ) {
		const hfSoundSample *objectp = entries[k];
		if ( objectp ) {
			common->Printf( "      %5dms %4dKb %s\n", objectp->LengthInMsec(), (objectp->BufferSize()/1024), objectp->GetName() );
		}
	}
}

/*
===============
hfSoundShader::GetAltSound
===============
*/
const hfSoundShader *hfSoundShader::GetAltSound() const {
	return altSound;
}

/*
===============
hfSoundShader::GetMinDistance
===============
*/
float hfSoundShader::GetMinDistance() const {
	return parms.minDistance;
}

/*
===============
hfSoundShader::GetMaxDistance
===============
*/
float hfSoundShader::GetMaxDistance() const {
	return parms.maxDistance;
}

/*
===============
hfSoundShader::HasDefaultSound
===============
*/
bool hfSoundShader::HasDefaultSound() const {
	for ( int i = 0; i < entries.Num(); i++ ) {
		if ( entries[i] && entries[i]->IsDefault() ) {
			return true;
		}
	}
	return false;
}

/*
===============
hfSoundShader::GetParms
===============
*/
const soundShaderParms_t *hfSoundShader::GetParms() const {
	return &parms;
}

/*
===============
hfSoundShader::GetNumSounds
===============
*/
int hfSoundShader::GetNumSounds() const {
	return entries.Num();
}

/*
===============
hfSoundShader::GetSound
===============
*/
const char *hfSoundShader::GetSound( int index ) const {
	if ( index >= 0 && index < entries.Num() ) {
		return entries[index]->GetName();
	}
	return "";
}

