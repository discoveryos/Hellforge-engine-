#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "../renderer/hfImage.h"

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

hfCVar swf_loadBinary( "swf_loadBinary", "1", CVAR_INTEGER, "used to set whether to load binary swf from generated" );

int hfSWF::mouseX = -1;
int hfSWF::mouseY = -1;
bool hfSWF::isMouseInClientArea = false;

extern hfCVar in_useJoystick;

/*
===================
hfSWF::hfSWF
===================
*/
hfSWF::hfSWF( const char * filename_, hfSoundWorld * soundWorld_ ) {

	atlasMaterial = NULL;

	swfScale = 1.0f;
	scaleToVirtual.Set( 1.0f, 1.0f );

	random.SetSeed( Sys_Milliseconds() );

	guiSolid = declManager->FindMaterial( "guiSolid" );
	guiCursor_arrow = declManager->FindMaterial( "ui/assets/guicursor_arrow" );
	guiCursor_hand = declManager->FindMaterial( "ui/assets/guicursor_hand" );
	white = declManager->FindMaterial( "_white" );

	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY1>", "guis/assets/hud/controller/xb360/a", "guis/assets/hud/controller/ps3/cross", 37, 37, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY2>", "guis/assets/hud/controller/xb360/b", "guis/assets/hud/controller/ps3/circle", 37, 37, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY3>", "guis/assets/hud/controller/xb360/x", "guis/assets/hud/controller/ps3/square", 37, 37, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY4>", "guis/assets/hud/controller/xb360/y", "guis/assets/hud/controller/ps3/triangle", 37, 37, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY_TRIGGER2>", "guis/assets/hud/controller/xb360/rt", "guis/assets/hud/controller/ps3/r2", 64, 52, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY_TRIGGER1>", "guis/assets/hud/controller/xb360/lt", "guis/assets/hud/controller/ps3/l2", 64, 52, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY5>", "guis/assets/hud/controller/xb360/lb", "guis/assets/hud/controller/ps3/l1", 52, 32, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<JOY6>", "guis/assets/hud/controller/xb360/rb", "guis/assets/hud/controller/ps3/r1", 52, 32, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<MOUSE1>", "guis/assets/hud/controller/mouse1", "", 64, 52, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<MOUSE2>", "guis/assets/hud/controller/mouse2", "", 64, 52, 0 ) );
	 tooltipButtonImage.Append( keyButtonImages_t( "<MOUSE3>", "guis/assets/hud/controller/mouse3", "", 64, 52, 0 ) );
	 	
	for ( int index = 0; index < tooltipButtonImage.Num(); index++ ) {
		if ( ( tooltipButtonImage[index].xbImage != NULL ) && ( tooltipButtonImage[index].xbImage[0] != '\0' ) ) {
			declManager->FindMaterial( tooltipButtonImage[index].xbImage );
		}
		if ( ( tooltipButtonImage[index].psImage != NULL ) && ( tooltipButtonImage[index].psImage[0] != '\0' ) ) {
			declManager->FindMaterial( tooltipButtonImage[index].psImage );
		}
	}

	frameWidth = 0;
	frameHeight = 0;
	frameRate = 0;
	lastRenderTime = 0;

	isActive = false;
	inhibitControl = false;
	useInhibtControl = true;

	crop = false;
	blackbars = false;
	paused = false;
	hasHitObject = false;

	useMouse = true;
	mouseEnabled = false;
	renderBorder = 0;
	mouseObject = NULL;
	hoverObject = NULL;
	soundWorld = NULL;
	forceNonPCPlatform = false;

	if ( hfStr::Cmpn( filename_, "swf/", 4 ) != 0 ) {
		// if it doesn't already have swf/ in front of it, add it
		filename = "swf/";
		filename += filename_;
	} else {
		filename = filename_;
	}
	filename.ToLower();
	filename.BackSlashesToSlashes();
	filename.SetFileExtension( ".swf" );

	timestamp = fileSystem->GetTimestamp( filename );

	mainsprite = new (TAG_SWF) hfSWFSprite( this );
	mainspriteInstance = NULL;

	hfStr binaryFileName = "generated/";
	binaryFileName += filename;
	binaryFileName.SetFileExtension( ".bswf" );

	if ( swf_loadBinary.GetBool() ) {
		HF_TIME_T sourceTime = fileSystem->GetTimestamp( filename );
		if ( !LoadBinary( binaryFileName, sourceTime ) ) {
			if ( LoadSWF( filename ) ) {
				WriteBinary( binaryFileName );
			}
		}
	} else {
		LoadSWF( filename );
	}
	hfStr atlasFileName = binaryFileName;
	atlasFileName.SetFileExtension( ".tga" );
	atlasMaterial = declManager->FindMaterial( atlasFileName );

	globals = hfSWFScriptObject::Alloc();
	globals->Set( "_global", globals );

	globals->Set( "Object", &scriptFunction_Object );

	mainspriteInstance = spriteInstanceAllocator.Alloc();
	mainspriteInstance->Init( mainsprite, NULL, 0 );

	shortcutKeys = hfSWFScriptObject::Alloc();
	scriptFunction_shortcutKeys_clear.Bind( this );
	scriptFunction_shortcutKeys_clear.Call( shortcutKeys, hfSWFParmList() );
	globals->Set( "shortcutKeys", shortcutKeys );

	globals->Set( "deactivate", scriptFunction_deactivate.Bind( this ) );
	globals->Set( "inhibitControl", scriptFunction_inhibitControl.Bind( this ) );
	globals->Set( "useInhibit", scriptFunction_useInhibit.Bind( this ) );
	globals->Set( "precacheSound", scriptFunction_precacheSound.Bind( this ) );
	globals->Set( "playSound", scriptFunction_playSound.Bind( this ) );
	globals->Set( "stopSounds",scriptFunction_stopSounds.Bind( this ) );
	globals->Set( "getPlatform", scriptFunction_getPlatform.Bind( this ) );
	globals->Set( "getTruePlatform", scriptFunction_getTruePlatform.Bind( this ) );
	globals->Set( "getLocalString", scriptFunction_getLocalString.Bind( this ) );
	globals->Set( "swapPS3Buttons", scriptFunction_swapPS3Buttons.Bind( this ) );
	globals->Set( "_root", mainspriteInstance->scriptObject );
	globals->Set( "strReplace", scriptFunction_strReplace.Bind( this ) );
	globals->Set( "getCVarInteger", scriptFunction_getCVarInteger.Bind( this ) );
	globals->Set( "setCVarInteger", scriptFunction_setCVarInteger.Bind( this ) );

	globals->Set( "acos", scriptFunction_acos.Bind( this ) );
	globals->Set( "cos", scriptFunction_cos.Bind( this ) );
	globals->Set( "sin", scriptFunction_sin.Bind( this ) );
	globals->Set( "round", scriptFunction_round.Bind( this ) );
	globals->Set( "pow", scriptFunction_pow.Bind( this ) );
	globals->Set( "sqrt", scriptFunction_sqrt.Bind( this ) );
	globals->Set( "abs", scriptFunction_abs.Bind( this ) );
	globals->Set( "rand", scriptFunction_rand.Bind( this ) );
	globals->Set( "floor", scriptFunction_floor.Bind( this ) );
	globals->Set( "ceil", scriptFunction_ceil.Bind( this ) );
	globals->Set( "toUpper", scriptFunction_toUpper.Bind( this ) );

	globals->SetNative( "platform", swfScriptVar_platform.Bind( &scriptFunction_getPlatform ) );
	globals->SetNative( "blackbars", swfScriptVar_blackbars.Bind( this ) );
	globals->SetNative( "cropToHeight", swfScriptVar_crop.Bind( this ) );
	globals->SetNative( "cropToFit", swfScriptVar_crop.Bind( this ) );
	globals->SetNative( "crop", swfScriptVar_crop.Bind( this ) );

	// Do this to touch any external references (like sounds)
	// But disable script warnings because many globals won't have been created yet
	extern hfCVar swf_debug;
	int debug = swf_debug.GetInteger();
	swf_debug.SetInteger( 0 );

	mainspriteInstance->Run();
	mainspriteInstance->RunActions();
	mainspriteInstance->RunTo( 0 );

	swf_debug.SetInteger( debug );

	if ( mouseX == -1 ) {
		mouseX = ( frameWidth / 2 );
	}

	if ( mouseY == -1 ) {
		mouseY = ( frameHeight / 2 );
	}

	soundWorld = soundWorld_;
}

/*
===================
hfSWF::~hfSWF
===================
*/
hfSWF::~hfSWF() {
	spriteInstanceAllocator.Free( mainspriteInstance );
	delete mainsprite;

	for ( int i = 0 ; i < dictionary.Num() ; i++ ) {
		if ( dictionary[i].sprite ) {
			delete dictionary[i].sprite;
			dictionary[i].sprite = NULL;
		}
		if ( dictionary[i].shape ) {
			delete dictionary[i].shape;
			dictionary[i].shape = NULL;
		}
		if ( dictionary[i].font ) {
			delete dictionary[i].font;
			dictionary[i].font = NULL;
		}
		if ( dictionary[i].text ) {
			delete dictionary[i].text;
			dictionary[i].text = NULL;
		}
		if ( dictionary[i].edittext ) {
			delete dictionary[i].edittext;
			dictionary[i].edittext = NULL;
		}
	}
	
	globals->Clear();
	tooltipButtonImage.Clear();
	globals->Release();

	shortcutKeys->Clear();
	shortcutKeys->Release();
}

/*
===================
hfSWF::Activate
when a SWF is deactivated, it rewinds the timeline back to the start
===================
*/
void hfSWF::Activate( bool b ) {
	if ( !isActive && b ) {
		inhibitControl = false;
		lastRenderTime = Sys_Milliseconds();

		mainspriteInstance->FreeDisplayList();
		mainspriteInstance->Play();
		mainspriteInstance->Run();
		mainspriteInstance->RunActions();
	}
	isActive = b;
}

/*
===================
hfSWF::InhibitControl
===================
*/
bool hfSWF::InhibitControl() {
	if ( !IsLoaded() || !IsActive() ) {
		return false;
	}
	return ( inhibitControl && useInhibtControl );
}

/*
===================
hfSWF::PlaySound
===================
*/
int hfSWF::PlaySound( const char * sound, int channel, bool blocking ) {
	if ( !IsActive() ) {
		return -1;
	}
	if ( soundWorld != NULL ) {
		return soundWorld->PlayShaderDirectly( sound, channel );
	} else {
		hfLib::Warning( "No playing sound world on soundSystem in swf play sound!" );
		return -1;
	}
}

/*
===================
hfSWF::PlaySound
===================
*/
void hfSWF::StopSound( int channel ) {
	if ( soundWorld != NULL ) {
		soundWorld->PlayShaderDirectly( NULL, channel );
	} else {
		hfLib::Warning( "No playing sound world on soundSystem in swf play sound!" );
	}
}

/*
===================
hfSWF::hfSWFScriptFunction_inhibitControl::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_inhibitControl::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	pThis->inhibitControl = parms[0].ToBool();
	return hfSWFScriptVar();
}

/*
===================
hfSWF::hfSWFScriptFunction_inhibitControl::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_useInhibit::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	pThis->useInhibtControl = parms[0].ToBool();
	return hfSWFScriptVar();
}

/*
===================
hfSWF::hfSWFScriptFunction_deactivate::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_deactivate::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	pThis->Activate( false );
	return hfSWFScriptVar();
}

/*
===================
hfSWF::hfSWFScriptFunction_precacheSound::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_precacheSound::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	const hfSoundShader * soundShader = declManager->FindSound( parms[0].ToString(), true );
	return soundShader->GetName();
}

/*
===================
hfSWF::hfSWFScriptFunction_playSound::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_playSound::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	int channel = SCHANNEL_ANY;
	// specific channel passed in
	if ( parms.Num() > 1 ) {
		channel = parms[1].ToInteger();
	}

	pThis->PlaySound( parms[0].ToString(), channel );

	return hfSWFScriptVar();
}

/*
===================
hfSWF::hfSWFScriptFunction_stopSounds::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_stopSounds::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {

	int channel = SCHANNEL_ANY;
	if ( parms.Num() == 1 ) {
		channel = parms[0].ToInteger();
	}

	pThis->StopSound( channel );

	return hfSWFScriptVar();
}

/*
========================
hfSWFScriptFunction_GetPlatform::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_getPlatform::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	return pThis->GetPlatform();
}

/*
========================
hfSWFScriptFunction_GetPlatform::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_getTruePlatform::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {

	return 2;
}


/*
========================
hfSWFScriptFunction_GetPlatform::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_strReplace::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {

	if ( parms.Num() != 3 ) {
		return "";
	}

	hfStr str = parms[0].ToString();
	hfStr repString = parms[1].ToString();
	hfStr val = parms[2].ToString();
	str.Replace( repString, val );

	return str;
}

/*
========================
hfSWFScriptFunction_GetPlatform::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_getLocalString::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {

	if ( parms.Num() == 0 ) {
		return hfSWFScriptVar();
	}

	hfStr val = hfLocalization::GetString( parms[0].ToString() );
	return val; 
}

/*
========================
hfSWF::UseCircleForAccept
========================
*/
bool hfSWF::UseCircleForAccept() {
	return false;
}

/*
========================
hfSWF::GetPlatform
========================
*/
int	hfSWF::GetPlatform() {


	if ( in_useJoystick.GetBool() || forceNonPCPlatform ) {
		forceNonPCPlatform = false;
		return 0;
	}

	return 2;
}

/*
========================
hfSWFScriptFunction_swapPS3Buttons::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_swapPS3Buttons::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	return pThis->UseCircleForAccept();
}

/*
========================
hfSWFScriptFunction_getCVarInteger::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_getCVarInteger::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	return cvarSystem->GetCVarInteger( parms[0].ToString() );
}

/*
========================
hfSWFScriptFunction_setCVarInteger::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_setCVarInteger::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	cvarSystem->SetCVarInteger( parms[0].ToString(), parms[1].ToInteger() );
	return hfSWFScriptVar();
}

/*
===================
hfSWF::hfSWFScriptFunction_acos::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_acos::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 ) {
		return hfSWFScriptVar();
	}
	return hfMath::ACos( parms[0].ToFloat() );
}

/*
===================
hfSWF::hfSWFScriptFunction_cos::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_cos::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 ) {
		return hfSWFScriptVar();
	}
	return hfMath::Cos( parms[0].ToFloat() );
}

/*
===================
hfSWF::hfSWFScriptFunction_sin::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_sin::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 ) {
		return hfSWFScriptVar();
	}
	return ( hfMath::Sin( parms[0].ToFloat() ) );
}

/*
===================
hfSWF::hfSWFScriptFunction_round::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_round::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 ) {
		return hfSWFScriptVar();
	}
	int value = hfMath::Ftoi( parms[0].ToFloat() + 0.5f );
	return value;
}

/*
===================
hfSWF::hfSWFScriptFunction_pow::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_pow::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 2 ) {
		return hfSWFScriptVar();
	}	

	float value = parms[0].ToFloat();
	float power = parms[1].ToFloat();
	return ( hfMath::Pow( value, power ) );
}

/*
===================
hfSWF::hfSWFScriptFunction_pow::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_sqrt::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 ) {
		return hfSWFScriptVar();
	}	

	float value = parms[0].ToFloat();
	return ( hfMath::Sqrt( value ) );
}

/*
===================
hfSWF::hfSWFScriptFunction_abs::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_abs::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 ) {
		return hfSWFScriptVar();
	}	

	float value = hfMath::Fabs( parms[0].ToFloat() );
	return value;
}

/*
===================
hfSWF::hfSWFScriptFunction_rand::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_rand::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	float min = 0.0f;
	float max = 1.0f;
	switch ( parms.Num() ) {
	case 0:
		break;
	case 1:
		max = parms[0].ToFloat();
		break;
	default:
		min = parms[0].ToFloat();
		max = parms[1].ToFloat();
		break;
	}
	return min + pThis->GetRandom().RandomFloat() * ( max - min );
}

/*
========================
hfSWFScriptFunction_floor::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_floor::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 || !parms[0].IsNumeric() ) {
		hfLib::Warning( "Invalid parameters specified for floor" );
		return hfSWFScriptVar();
	}

	float num = parms[0].ToFloat();

	return hfSWFScriptVar( hfMath::Floor( num ) );
}

/*
========================
hfSWFScriptFunction_ceil::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_ceil::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 || !parms[0].IsNumeric() ) {
		hfLib::Warning( "Invalid parameters specified for ceil" );
		return hfSWFScriptVar();
	}

	float num = parms[0].ToFloat();

	return hfSWFScriptVar( hfMath::Ceil( num ) );
}

/*
========================
hfSWFScriptFunction_toUpper::Call
========================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_toUpper::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	if ( parms.Num() != 1 || !parms[0].IsString() ) {
		hfLib::Warning( "Invalid parameters specified for toUpper" );
		return hfSWFScriptVar();
	}

	hfStr val = hfLocalization::GetString( parms[0].ToString() );
	val.ToUpper();
	return val;
}

/*
===================
hfSWF::hfSWFScriptFunction_shortcutKeys_clear::Call
===================
*/
hfSWFScriptVar hfSWF::hfSWFScriptFunction_shortcutKeys_clear::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
	hfSWFScriptObject * object = pThis->shortcutKeys;
	object->Clear();
	object->Set( "clear", this );
	object->Set( "JOY1", "ENTER" );
	object->Set( "JOY2", "BACKSPACE" );
	object->Set( "JOY3", "START" );
	object->Set( "JOY5", "LB" );
	object->Set( "JOY6", "RB" );
	object->Set( "JOY9", "START" );
	object->Set( "JOY10", "BACKSPACE" );
	object->Set( "JOY_DPAD_UP", "UP" );
	object->Set( "JOY_DPAD_DOWN", "DOWN" );
	object->Set( "JOY_DPAD_LEFT", "LEFT" );
	object->Set( "JOY_DPAD_RIGHT", "RIGHT" );
	object->Set( "JOY_STICK1_UP", "STICK1_UP" );
	object->Set( "JOY_STICK1_DOWN", "STICK1_DOWN" );
	object->Set( "JOY_STICK1_LEFT", "STICK1_LEFT" );
	object->Set( "JOY_STICK1_RIGHT", "STICK1_RIGHT" );
	object->Set( "JOY_STICK2_UP", "STICK2_UP" );
	object->Set( "JOY_STICK2_DOWN", "STICK2_DOWN" );
	object->Set( "JOY_STICK2_LEFT", "STICK2_LEFT" );
	object->Set( "JOY_STICK2_RIGHT", "STICK2_RIGHT" );
	object->Set( "KP_ENTER", "ENTER" );
	object->Set( "MWHEELDOWN", "MWHEEL_DOWN" );
	object->Set( "MWHEELUP", "MWHEEL_UP" );
	object->Set( "K_TAB", "TAB" );


	// FIXME: I'm an RTARD and didn't realize the keys all have "ARROW" after them
	object->Set( "LEFTARROW", "LEFT" );
	object->Set( "RIGHTARROW", "RIGHT" );
	object->Set( "UPARROW", "UP" );
	object->Set( "DOWNARROW", "DOWN" );


	return hfSWFScriptVar();
}

hfSWFScriptVar hfSWF::hfSWFScriptNativeVar_blackbars::Get( hfSWFScriptObject * object ) {
	return pThis->blackbars;
}

void hfSWF::hfSWFScriptNativeVar_blackbars::Set( hfSWFScriptObject * object, const hfSWFScriptVar & value ) {
	pThis->blackbars = value.ToBool();
}

hfSWFScriptVar hfSWF::hfSWFScriptNativeVar_crop::Get( hfSWFScriptObject * object ) {
	return pThis->crop;
}

void hfSWF::hfSWFScriptNativeVar_crop::Set( hfSWFScriptObject * object, const hfSWFScriptVar & value ) {
	pThis->crop = value.ToBool();
}

