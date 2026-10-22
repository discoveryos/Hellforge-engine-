#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfCommon_local.h"

/*
================
FindUnusedFileName
================
*/
static hfStr FindUnusedFileName( const char *format ) {
	hfStr filename;

	for ( int i = 0 ; i < 999 ; i++ ) {
		filename.Format( format, i );
		int len = fileSystem->ReadFile( filename, NULL, NULL );
		if ( len <= 0 ) {
			return filename;	// file doesn't exist
		}
	}

	return filename;
}

/*
================
hfCommonLocal::StartRecordingRenderDemo
================
*/
void hfCommonLocal::StartRecordingRenderDemo( const char *demoName ) {
	if ( writeDemo ) {
		// allow it to act like a toggle
		StopRecordingRenderDemo();
		return;
	}

	if ( !demoName[0] ) {
		common->Printf( "hfCommonLocal::StartRecordingRenderDemo: no name specified\n" );
		return;
	}

	console->Close();

	writeDemo = new (TAG_SYSTEM) hfDemoFile;
	if ( !writeDemo->OpenForWriting( demoName ) ) {
		common->Printf( "error opening %s\n", demoName );
		delete writeDemo;
		writeDemo = NULL;
		return;
	}

	common->Printf( "recording to %s\n", writeDemo->GetName() );

	writeDemo->WriteInt( DS_VERSION );
	writeDemo->WriteInt( RENDERDEMO_VERSION );

	// if we are in a map already, dump the current state
	soundWorld->StartWritingDemo( writeDemo );
	renderWorld->StartWritingDemo( writeDemo );
}

/*
================
hfCommonLocal::StopRecordingRenderDemo
================
*/
void hfCommonLocal::StopRecordingRenderDemo() {
	if ( !writeDemo ) {
		common->Printf( "hfCommonLocal::StopRecordingRenderDemo: not recording\n" );
		return;
	}
	soundWorld->StopWritingDemo();
	renderWorld->StopWritingDemo();

	writeDemo->Close();
	common->Printf( "stopped recording %s.\n", writeDemo->GetName() );
	delete writeDemo;
	writeDemo = NULL;
}

/*
================
hfCommonLocal::StopPlayingRenderDemo

Reports timeDemo numbers and finishes any avi recording
================
*/
void hfCommonLocal::StopPlayingRenderDemo() {
	if ( !readDemo ) {
		timeDemo = TD_NO;
		return;
	}

	// Record the stop time before doing anything that could be time consuming 
	int timeDemoStopTime = Sys_Milliseconds();

	EndAVICapture();

	readDemo->Close();

	soundWorld->StopAllSounds();
	soundSystem->SetPlayingSoundWorld( menuSoundWorld );

	common->Printf( "stopped playing %s.\n", readDemo->GetName() );
	delete readDemo;
	readDemo = NULL;

	if ( timeDemo ) {
		// report the stats
		float	demoSeconds = ( timeDemoStopTime - timeDemoStartTime ) * 0.001f;
		float	demoFPS = numDemoFrames / demoSeconds;
		hfStr	message = va( "%i frames rendered in %3.1f seconds = %3.1f fps\n", numDemoFrames, demoSeconds, demoFPS );

		common->Printf( message );
		if ( timeDemo == TD_YES_THEN_QUIT ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
		}
		timeDemo = TD_NO;
	}
}

/*
================
hfCommonLocal::DemoShot

A demoShot is a single frame demo
================
*/
void hfCommonLocal::DemoShot( const char *demoName ) {
	StartRecordingRenderDemo( demoName );

	// force draw one frame
	const bool captureToImage = false;
	UpdateScreen( captureToImage );

	StopRecordingRenderDemo();
}

/*
================
hfCommonLocal::StartPlayingRenderDemo
================
*/
void hfCommonLocal::StartPlayingRenderDemo( hfStr demoName ) {
	if ( !demoName[0] ) {
		common->Printf( "hfCommonLocal::StartPlayingRenderDemo: no name specified\n" );
		return;
	}

	// make sure localSound / GUI intro music shuts up
	soundWorld->StopAllSounds();
	soundWorld->PlayShaderDirectly( "", 0 );	
	menuSoundWorld->StopAllSounds();
	menuSoundWorld->PlayShaderDirectly( "", 0 );

	// exit any current game
	Stop();

	// automatically put the console away
	console->Close();

	readDemo = new (TAG_SYSTEM) hfDemoFile;
	demoName.DefaultFileExtension( ".demo" );
	if ( !readDemo->OpenForReading( demoName ) ) {
		common->Printf( "couldn't open %s\n", demoName.c_str() );
		delete readDemo;
		readDemo = NULL;
		Stop();
		StartMenu();
		return;
	}

	const bool captureToImage = false;
	UpdateScreen( captureToImage );

	AdvanceRenderDemo( true );

	numDemoFrames = 1;

	timeDemoStartTime = Sys_Milliseconds();
}

/*
================
hfCommonLocal::TimeRenderDemo
================
*/
void hfCommonLocal::TimeRenderDemo( const char *demoName, bool twice, bool quit ) {
	hfStr demo = demoName;

	StartPlayingRenderDemo( demo );
	
	if ( twice && readDemo ) {
		while ( readDemo ) {
			const bool captureToImage = false;
			UpdateScreen( captureToImage );
			AdvanceRenderDemo( true );
		}

		StartPlayingRenderDemo( demo );
	}
	

	if ( !readDemo ) {
		return;
	}

	if ( quit ) {
		// this allows hardware vendors to automate some testing
		timeDemo = TD_YES_THEN_QUIT;
	} else {
		timeDemo = TD_YES;
	}
}


/*
================
hfCommonLocal::BeginAVICapture
================
*/
void hfCommonLocal::BeginAVICapture( const char *demoName ) {
	hfStr name = demoName;
	name.ExtractFileBase( aviDemoShortName );
	aviCaptureMode = true;
	aviDemoFrameCount = 0;
	soundWorld->AVIOpen( va( "demos/%s/", aviDemoShortName.c_str() ), aviDemoShortName.c_str() );
}

/*
================
hfCommonLocal::EndAVICapture
================
*/
void hfCommonLocal::EndAVICapture() {
	if ( !aviCaptureMode ) {
		return;
	}

	soundWorld->AVIClose();

	// write a .roqParam file so the demo can be converted to a roq file
	hfFile *f = fileSystem->OpenFileWrite( va( "demos/%s/%s.roqParam", 
		aviDemoShortName.c_str(), aviDemoShortName.c_str() ) );
	f->Printf( "INPUT_DIR demos/%s\n", aviDemoShortName.c_str() );
	f->Printf( "FILENAME demos/%s/%s.RoQ\n", aviDemoShortName.c_str(), aviDemoShortName.c_str() );
	f->Printf( "\nINPUT\n" );
	f->Printf( "%s_*.tga [00000-%05i]\n", aviDemoShortName.c_str(), (int)( aviDemoFrameCount-1 ) );
	f->Printf( "END_INPUT\n" );
	delete f;

	common->Printf( "captured %i frames for %s.\n", ( int )aviDemoFrameCount, aviDemoShortName.c_str() );

	aviCaptureMode = false;
}


/*
================
hfCommonLocal::AVIRenderDemo
================
*/
void hfCommonLocal::AVIRenderDemo( const char *_demoName ) {
	hfStr	demoName = _demoName;	// copy off from va() buffer

	StartPlayingRenderDemo( demoName );
	if ( !readDemo ) {
		return;
	}

	BeginAVICapture( demoName.c_str() ) ;

	// I don't understand why I need to do this twice, something
	// strange with the nvidia swapbuffers?
	const bool captureToImage = false;
	UpdateScreen( captureToImage );
}

/*
================
hfCommonLocal::AVIGame

Start AVI recording the current game session
================
*/
void hfCommonLocal::AVIGame( const char *demoName ) {
	if ( aviCaptureMode ) {
		EndAVICapture();
		return;
	}

	if ( !mapSpawned ) {
		common->Printf( "No map spawned.\n" );
	}

	if ( !demoName || !demoName[0] ) {
		hfStr filename = FindUnusedFileName( "demos/game%03i.game" );
		demoName = filename.c_str();

		// write a one byte stub .game file just so the FindUnusedFileName works,
		fileSystem->WriteFile( demoName, demoName, 1 );
	}

	BeginAVICapture( demoName ) ;
}

/*
================
hfCommonLocal::CompressDemoFile
================
*/
void hfCommonLocal::CompressDemoFile( const char *scheme, const char *demoName ) {
	hfStr	fullDemoName = "demos/";
	fullDemoName += demoName;
	fullDemoName.DefaultFileExtension( ".demo" );
	hfStr compressedName = fullDemoName;
	compressedName.StripFileExtension();
	compressedName.Append( "_compressed.demo" );

	int savedCompression = cvarSystem->GetCVarInteger("com_compressDemos");
	bool savedPreload = cvarSystem->GetCVarBool("com_preloadDemos");
	cvarSystem->SetCVarBool( "com_preloadDemos", false );
	cvarSystem->SetCVarInteger("com_compressDemos", atoi(scheme) );

	hfDemoFile demoread, demowrite;
	if ( !demoread.OpenForReading( fullDemoName ) ) {
		common->Printf( "Could not open %s for reading\n", fullDemoName.c_str() );
		return;
	}
	if ( !demowrite.OpenForWriting( compressedName ) ) {
		common->Printf( "Could not open %s for writing\n", compressedName.c_str() );
		demoread.Close();
		cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
		cvarSystem->SetCVarInteger("com_compressDemos", savedCompression);
		return;
	}
	common->SetRefreshOnPrint( true );
	common->Printf( "Compressing %s to %s...\n", fullDemoName.c_str(), compressedName.c_str() );

	static const int bufferSize = 65535;
	char buffer[bufferSize];
	int bytesRead;
	while ( 0 != (bytesRead = demoread.Read( buffer, bufferSize ) ) ) {
		demowrite.Write( buffer, bytesRead );
		common->Printf( "." );
	}

	demoread.Close();
	demowrite.Close();

	cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
	cvarSystem->SetCVarInteger("com_compressDemos", savedCompression);

	common->Printf( "Done\n" );
	common->SetRefreshOnPrint( false );

}

/*
===============
hfCommonLocal::AdvanceRenderDemo
===============
*/
void hfCommonLocal::AdvanceRenderDemo( bool singleFrameOnly ) {
	int	ds = DS_FINISHED;
	readDemo->ReadInt( ds );

	switch ( ds ) {
	case DS_FINISHED:
		if ( numDemoFrames != 1 ) {
			// if the demo has a single frame (a demoShot), continuously replay
			// the renderView that has already been read
			Stop();
			StartMenu();
		}
		return;
	case DS_RENDER:
		if ( renderWorld->ProcessDemoCommand( readDemo, &currentDemoRenderView, &demoTimeOffset ) ) {
			// a view is ready to render
			numDemoFrames++;
		}
		break;
	case DS_SOUND:
		soundWorld->ProcessDemoCommand( readDemo );
		break;
	default:
		common->Error( "Bad render demo token" );
	}
}

/*
================
Common_DemoShot_f
================
*/
CONSOLE_COMMAND( demoShot, "writes a screenshot as a demo", NULL ) {
	if ( args.Argc() != 2 ) {
		hfStr filename = FindUnusedFileName( "demos/shot%03i.demo" );
		commonLocal.DemoShot( filename );
	} else {
		commonLocal.DemoShot( va( "demos/shot_%s.demo", args.Argv(1) ) );
	}
}

/*
================
Common_RecordDemo_f
================
*/
CONSOLE_COMMAND( recordDemo, "records a demo", NULL ) {
	if ( args.Argc() != 2 ) {
		hfStr filename = FindUnusedFileName( "demos/demo%03i.demo" );
		commonLocal.StartRecordingRenderDemo( filename );
	} else {
		commonLocal.StartRecordingRenderDemo( va( "demos/%s.demo", args.Argv(1) ) );
	}
}

/*
================
Common_CompressDemo_f
================
*/
CONSOLE_COMMAND( compressDemo, "compresses a demo file", hfCmdSystem::ArgCompletion_DemoName ) {
	if ( args.Argc() == 2 ) {
		commonLocal.CompressDemoFile( "2", args.Argv(1) );
	} else if ( args.Argc() == 3 ) {
		commonLocal.CompressDemoFile( args.Argv(2), args.Argv(1) );
	} else {
		common->Printf("use: CompressDemo <file> [scheme]\nscheme is the same as com_compressDemo, defaults to 2" );
	}
}

/*
================
Common_StopRecordingDemo_f
================
*/
CONSOLE_COMMAND( stopRecording, "stops demo recording", NULL ) {
	commonLocal.StopRecordingRenderDemo();
}

/*
================
Common_PlayDemo_f
================
*/
CONSOLE_COMMAND( playDemo, "plays back a demo", hfCmdSystem::ArgCompletion_DemoName ) {
	if ( args.Argc() >= 2 ) {
		commonLocal.StartPlayingRenderDemo( va( "demos/%s", args.Argv(1) ) );
	}
}

/*
================
Common_TimeDemo_f
================
*/
CONSOLE_COMMAND( timeDemo, "times a demo", hfCmdSystem::ArgCompletion_DemoName ) {
	if ( args.Argc() >= 2 ) {
		commonLocal.TimeRenderDemo( va( "demos/%s", args.Argv(1) ), ( args.Argc() > 2 ), false );
	}
}

/*
================
Common_TimeDemoQuit_f
================
*/
CONSOLE_COMMAND( timeDemoQuit, "times a demo and quits", hfCmdSystem::ArgCompletion_DemoName ) {
	commonLocal.TimeRenderDemo( va( "demos/%s", args.Argv(1) ), true );
}

/*
================
Common_AVIDemo_f
================
*/
CONSOLE_COMMAND( aviDemo, "writes AVIs for a demo", hfCmdSystem::ArgCompletion_DemoName ) {
	commonLocal.AVIRenderDemo( va( "demos/%s", args.Argv(1) ) );
}

/*
================
Common_AVIGame_f
================
*/
CONSOLE_COMMAND( aviGame, "writes AVIs for the current game", NULL ) {
	commonLocal.AVIGame( args.Argv(1) );
}

