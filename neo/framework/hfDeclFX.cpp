#include "../hflib/hfprecompiled.h"
#pragma hdrstop


/*
=================
hfDeclFX::Size
=================
*/
size_t hfDeclFX::Size() const {
	return sizeof( hfDeclFX );
}

/*
===============
hfDeclFX::Print
===============
*/
void hfDeclFX::Print() const {
	const hfDeclFX *list = this;

	common->Printf("%d events\n", list->events.Num() );
	for( int i = 0; i < list->events.Num(); i++ ) {
		switch( list->events[i].type ) {
			case FX_LIGHT:
				common->Printf("FX_LIGHT %s\n", list->events[i].data.c_str());
				break;
			case FX_PARTICLE:
				common->Printf("FX_PARTICLE %s\n", list->events[i].data.c_str());
				break;
			case FX_MODEL:
				common->Printf("FX_MODEL %s\n", list->events[i].data.c_str());
				break;
			case FX_SOUND:
				common->Printf("FX_SOUND %s\n", list->events[i].data.c_str());
				break;
			case FX_DECAL:
				common->Printf("FX_DECAL %s\n", list->events[i].data.c_str());
				break;
			case FX_SHAKE:
				common->Printf("FX_SHAKE %s\n", list->events[i].data.c_str());
				break;
			case FX_ATTACHLIGHT:
				common->Printf("FX_ATTACHLIGHT %s\n", list->events[i].data.c_str());
				break;
			case FX_ATTACHENTITY:
				common->Printf("FX_ATTACHENTITY %s\n", list->events[i].data.c_str());
				break;
			case FX_LAUNCH:
				common->Printf("FX_LAUNCH %s\n", list->events[i].data.c_str());
				break;
			case FX_SHOCKWAVE:
				common->Printf("FX_SHOCKWAVE %s\n", list->events[i].data.c_str());
				break;
		}
	}
}

/*
===============
hfDeclFX::List
===============
*/
void hfDeclFX::List() const {
	common->Printf("%s, %d stages\n", GetName(), events.Num() );
}

/*
================
hfDeclFX::ParseSingleFXAction
================
*/
void hfDeclFX::ParseSingleFXAction( hfLexer &src, hfFXSingleAction& FXAction ) {
	hfToken token;

	FXAction.type = -1;
	FXAction.sibling = -1;

	FXAction.data = "<none>";
	FXAction.name = "<none>";
	FXAction.fire = "<none>";

	FXAction.delay = 0.0f;
	FXAction.duration = 0.0f;
	FXAction.restart = 0.0f;
	FXAction.size = 0.0f;
	FXAction.fadeInTime = 0.0f;
	FXAction.fadeOutTime = 0.0f;
	FXAction.shakeTime = 0.0f;
	FXAction.shakeAmplitude = 0.0f;
	FXAction.shakeDistance = 0.0f;
	FXAction.shakeFalloff = false;
	FXAction.shakeImpulse = 0.0f;
	FXAction.shakeIgnoreMaster = false;
	FXAction.lightRadius = 0.0f;
	FXAction.rotate = 0.0f;
	FXAction.random1 = 0.0f;
	FXAction.random2 = 0.0f;

	FXAction.lightColor = vec3_origin;
	FXAction.offset = vec3_origin;
	FXAction.axis = mat3_identity;

	FXAction.bindParticles = false;
	FXAction.explicitAxis = false;
	FXAction.noshadows = false;
	FXAction.particleTrackVelocity = false;
	FXAction.trackOrigin = false;
	FXAction.soundStarted = false;

	while (1) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "shake" ) ) {
			FXAction.type = FX_SHAKE;
			FXAction.shakeTime = src.ParseFloat();
			src.ExpectTokenString(",");
			FXAction.shakeAmplitude = src.ParseFloat();
			src.ExpectTokenString(",");
			FXAction.shakeDistance = src.ParseFloat();
			src.ExpectTokenString(",");
			FXAction.shakeFalloff = src.ParseBool();
			src.ExpectTokenString(",");
			FXAction.shakeImpulse = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "noshadows" ) ) {
			FXAction.noshadows = true;
			continue;
		}

		if ( !token.Icmp( "name" ) ) {
			src.ReadToken( &token );
			FXAction.name = token;
			continue;
		}

		if ( !token.Icmp( "fire") ) {
			src.ReadToken( &token );
			FXAction.fire = token;
			continue;
		}

		if ( !token.Icmp( "random" ) ) {
			FXAction.random1 = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.random2 = src.ParseFloat();
			FXAction.delay = 0.0f;		// check random
			continue;
		}

		if ( !token.Icmp( "delay" ) ) {
			FXAction.delay = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "rotate" ) ) {
			FXAction.rotate = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "duration" ) ) {
			FXAction.duration = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "trackorigin" ) ) {
			FXAction.trackOrigin = src.ParseBool();
			continue;
		}

		if (!token.Icmp("restart")) {
			FXAction.restart = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "fadeIn" ) ) {
			FXAction.fadeInTime = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "fadeOut" ) ) {
			FXAction.fadeOutTime = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "size" ) ) {
			FXAction.size = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "offset" ) ) {
			FXAction.offset.x = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.offset.y = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.offset.z = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "axis" ) ) {
			hfVec3 v;
			v.x = src.ParseFloat();
			src.ExpectTokenString( "," );
			v.y = src.ParseFloat();
			src.ExpectTokenString( "," );
			v.z = src.ParseFloat();
			v.Normalize();
			FXAction.axis = v.ToMat3();
			FXAction.explicitAxis = true;
			continue;
		}

		if ( !token.Icmp( "angle" ) ) {
			hfAngles a;
			a[0] = src.ParseFloat();
			src.ExpectTokenString( "," );
			a[1] = src.ParseFloat();
			src.ExpectTokenString( "," );
			a[2] = src.ParseFloat();
			FXAction.axis = a.ToMat3();
			FXAction.explicitAxis = true;
			continue;
		}

		if ( !token.Icmp( "uselight" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			for( int i = 0; i < events.Num(); i++ ) {
				if ( events[i].name.Icmp( FXAction.data ) == 0 ) {
					FXAction.sibling = i;
					FXAction.lightColor = events[i].lightColor;
					FXAction.lightRadius = events[i].lightRadius;
				}
			}
			FXAction.type = FX_LIGHT;

			// precache the light material
			declManager->FindMaterial( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "attachlight" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_ATTACHLIGHT;

			// precache it
			declManager->FindMaterial( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "attachentity" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_ATTACHENTITY;

			// precache the model
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "launch" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_LAUNCH;

			// precache the entity def
			declManager->FindType( DECL_ENTITYDEF, FXAction.data );
			continue;
		}

		if ( !token.Icmp( "useModel" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			for( int i = 0; i < events.Num(); i++ ) {
				if ( events[i].name.Icmp( FXAction.data ) == 0 ) {
					FXAction.sibling = i;
				}
			}
			FXAction.type = FX_MODEL;

			// precache the model
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "light" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			src.ExpectTokenString( "," );
			FXAction.lightColor[0] = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.lightColor[1] = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.lightColor[2] = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.lightRadius = src.ParseFloat();
			FXAction.type = FX_LIGHT;

			// precache the light material
			declManager->FindMaterial( FXAction.data );
			continue;
		}
	
		if ( !token.Icmp( "model" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_MODEL;

			// precache it
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "particle" ) ) {	// FIXME: now the same as model
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_PARTICLE;

			// precache it
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "decal" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_DECAL;

			// precache it
			declManager->FindMaterial( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "particleTrackVelocity" ) ) {
			FXAction.particleTrackVelocity = true;
			continue;
		}

		if ( !token.Icmp( "sound" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_SOUND;

			// precache it
			declManager->FindSound( FXAction.data );
			continue;
		}

		if ( !token.Icmp( "ignoreMaster" ) ) {
			FXAction.shakeIgnoreMaster = true;
			continue;
		}

		if ( !token.Icmp( "shockwave" ) ) {
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_SHOCKWAVE;

			// precache the entity def
			declManager->FindType( DECL_ENTITYDEF, FXAction.data );
			continue;
		}

		src.Warning( "FX File: bad token" );
		continue;
	}
}

/*
================
hfDeclFX::Parse
================
*/
bool hfDeclFX::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	hfLexer src;
	hfToken token;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
	while( 1 ) {
		
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "bindto" ) ) {
			src.ReadToken( &token );
			joint = token;
			continue;
		}

		if ( !token.Icmp( "{" ) ) {
			hfFXSingleAction action;
			ParseSingleFXAction( src, action );
			events.Append( action );
			continue;
		}
	}

	if ( src.HadError() ) {
		src.Warning( "FX decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
hfDeclFX::DefaultDefinition
===================
*/
const char *hfDeclFX::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"{\n"
	"\t\t"		"duration\t5\n"
	"\t\t"		"model\t\t_default\n"
	"\t"	"}\n"
		"}"; 
}

/*
===================
hfDeclFX::FreeData
===================
*/
void hfDeclFX::FreeData() {
	events.Clear();
}

