#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfMiniAudio.h"
#include "hfAudioManager.h"
#include "../hflib/audio/hfAudioData.h"
#include "hfCVarSystem.h"
#include "hfCmdSystem.h"
#include "../framework/hfCommon.h"

// Console-facing audio module. Registers the hfComInitAudio-adjacent commands
// so the modern miniaudio engine can be driven from the console without any
// gameplay/gamecode dependencies: init/shutdown the backend, play/stop named
// sounds, and drive the preview player.

namespace HF_Audio {

hfCVar s_audioRate( "s_audioRate", "48000", CVAR_RENDERER | CVAR_INTEGER, "miniaudio output sample rate" );
hfCVar s_audioList( "s_audioList", "1", CVAR_RENDERER | CVAR_BOOL, "print every started sound" );

hfMiniAudioEngine *hfAudioEngine = NULL;
hfAudioManager *hfAudioManagerInstance = NULL;

static void EngineInit() {
	if ( hfAudioEngine ) { return; }
	unsigned int rate = ( unsigned int )s_audioRate.GetInteger();
	hfMiniAudioEngine *engine = new hfMiniAudioEngine();
	engine->Init( rate );
	if ( !engine->IsInitialized() ) {
		hfLib::Printf( "hfAudioEngine: miniaudio init failed (no audio device?)\n" );
		delete engine;
		return;
	}
	hfAudioEngine = engine;
	hfLib::Printf( "hfAudioEngine: miniaudio backend ready (%u Hz)\n", rate );
}

static void EngineShutdown() {
	if ( hfAudioEngine ) {
		hfAudioEngine->Shutdown();
		delete hfAudioEngine;
		hfAudioEngine = NULL;
	}
}

// AudioManager singleton with preview dirty hooks wired to the preview player.
static void ManagerEnsure() {
	if ( hfAudioManagerInstance ) { return; }
	hfAudioManagerInstance = new hfAudioManager( NULL, NULL );
}

// ---- console commands ----

static void hfAudioInit_f( const hfCmdArgs & args ) {
	EngineInit();
	ManagerEnsure();
}

static void hfAudioShutdown_f( const hfCmdArgs & args ) {
	EngineShutdown();
}

static void hfAudioStopAll_f( const hfCmdArgs & args ) {
	if ( hfAudioEngine ) { hfAudioEngine->StopAllSounds(); }
}

static void hfAudioPlay_f( const hfCmdArgs & args ) {
	if ( args.Argc() < 2 ) {
		hfLib::Printf( "usage: audioPlay <file> [entityKey]\n" );
		return;
	}
	EngineInit();
	ManagerEnsure();
	if ( !hfAudioEngine ) { return; }
	unsigned int id = hfAudioManagerInstance->LoadSync( args.Argv( 1 ) );
	const hfAudioManager::Entry *e = hfAudioManagerInstance->GetEntry( id );
	if ( !e || e->state != HF_SOUND_READY || !e->samples ) {
		hfLib::Printf( "audioPlay: '%s' not decodable\n", args.Argv( 1 ) );
		return;
	}
	unsigned int key = ( args.Argc() >= 3 ) ? ( unsigned int )atoi( args.Argv( 2 ) ) : 1;
	hfPlayParams p;
	p.play = true;
	p.loop = ( args.Argc() >= 4 ) ? ( atoi( args.Argv( 3 ) ) != 0 ) : false;
	float pos[3] = { 0.0f, 0.0f, 0.0f };
	hfAudioEngine->UpdateSound( key, e->info, e->samples, p, pos );
	if ( s_audioList.GetBool() ) {
		hfLib::Printf( "audio: playing '%s' key=%u (%.2fs, %u Hz, %u ch)\n",
			e->name.c_str(), key, e->info.DurationSec(), e->info.sampleRate, e->info.channels );
	}
}

static void hfAudioStop_f( const hfCmdArgs & args ) {
	if ( args.Argc() < 2 ) {
		hfLib::Printf( "usage: audioStop <entityKey>\n" );
		return;
	}
	if ( hfAudioEngine ) {
		hfAudioEngine->StopSound( ( unsigned int )atoi( args.Argv( 1 ) ) );
	}
}

static void hfAudioPreview_f( const hfCmdArgs & args ) {
	if ( args.Argc() < 2 ) {
		hfLib::Printf( "usage: audioPreview <file>\n" );
		return;
	}
	EngineInit();
	ManagerEnsure();
	if ( !hfAudioEngine ) { return; }
	unsigned int id = hfAudioManagerInstance->LoadSync( args.Argv( 1 ) );
	const hfAudioManager::Entry *e = hfAudioManagerInstance->GetEntry( id );
	if ( !e || e->state != HF_SOUND_READY || !e->samples ) {
		hfLib::Printf( "audioPreview: '%s' not decodable\n", args.Argv( 1 ) );
		return;
	}
	hfAudioEngine->PlayPreview( e->info, e->samples );
}

static void hfAudioPreviewSeek_f( const hfCmdArgs & args ) {
	if ( hfAudioEngine && args.Argc() >= 2 ) {
		hfAudioEngine->SetPreviewTime( ( float )atof( args.Argv( 1 ) ) );
	}
}

static void hfAudioPreviewStop_f( const hfCmdArgs & args ) {
	if ( hfAudioEngine ) { hfAudioEngine->StopPreview(); }
}

static void hfAudioPreviewStatus_f( const hfCmdArgs & args ) {
	if ( !hfAudioEngine ) { hfLib::Printf( "audio: engine not initialized\n" ); return; }
	hfLib::Printf( "audio: preview playing=%d time=%.2fs\n",
		hfAudioEngine->IsPreviewPlaying() ? 1 : 0, hfAudioEngine->GetPreviewTime() );
}

static float s_toneBuf[48000];	// 1s of mono f32 at 48 kHz

static void hfAudioTone_f( const hfCmdArgs & args ) {
	float freq = ( args.Argc() >= 2 ) ? ( float )atof( args.Argv( 1 ) ) : 440.0f;
	bool loop = ( args.Argc() >= 3 ) ? ( atoi( args.Argv( 2 ) ) != 0 ) : true;
	if ( freq < 20.0f ) { freq = 20.0f; }
	EngineInit();
	if ( !hfAudioEngine ) { return; }
	unsigned rate = ( unsigned int )s_audioRate.GetInteger();
	if ( rate > 48000 ) { rate = 48000; }
	if ( rate < 1 ) { rate = 1; }
	for ( unsigned i = 0; i < rate; i++ ) {
		s_toneBuf[i] = 0.4f * hfMath::Sin( hfMath::TWO_PI * freq * ( float )i / ( float )rate );
	}
	hfAudioSampleInfo info;
	info.channels = 1;
	info.sampleRate = rate;
	info.totalFrames = rate;
	info.totalBytes = rate * sizeof( float );
	hfPlayParams p;
	p.play = true;
	p.loop = loop;
	float pos[3] = { 0.0f, 0.0f, 0.0f };
	hfAudioEngine->UpdateSound( 99, info, s_toneBuf, p, pos );
	hfLib::Printf( "audio: tone %.0f Hz (%u Hz, loop=%d) on key 99\n", freq, rate, loop ? 1 : 0 );
}

void HF_AudioRegisterCommands() {
	static bool sDone = false;
	if ( sDone ) { return; }
	sDone = true;
	if ( cmdSystem ) {
		cmdSystem->AddCommand( "audioInit", hfAudioInit_f, CMD_FL_SOUND, "initialize the miniaudio backend" );
		cmdSystem->AddCommand( "audioShutdown", hfAudioShutdown_f, CMD_FL_SOUND, "shut down the miniaudio backend" );
		cmdSystem->AddCommand( "audioPlay", hfAudioPlay_f, CMD_FL_SOUND, "audioPlay <file> [key] [loop]" );
		cmdSystem->AddCommand( "audioStop", hfAudioStop_f, CMD_FL_SOUND, "audioStop <key>" );
		cmdSystem->AddCommand( "audioStopAll", hfAudioStopAll_f, CMD_FL_SOUND, "stop every active sound" );
		cmdSystem->AddCommand( "audioPreview", hfAudioPreview_f, CMD_FL_SOUND, "audioPreview <file> - non-spatialized player" );
		cmdSystem->AddCommand( "audioPreviewSeek", hfAudioPreviewSeek_f, CMD_FL_SOUND, "audioPreviewSeek <seconds>" );
		cmdSystem->AddCommand( "audioPreviewStop", hfAudioPreviewStop_f, CMD_FL_SOUND, "stop + rewind preview" );
		cmdSystem->AddCommand( "audioPreviewStatus", hfAudioPreviewStatus_f, CMD_FL_SOUND, "preview playing/time" );
		cmdSystem->AddCommand( "audioManagerStats", hfAudioPreviewStatus_f, CMD_FL_SOUND, "preview status (manager hook)" );
		cmdSystem->AddCommand( "audioTone", hfAudioTone_f, CMD_FL_SOUND, "audioTone <freqHz> [loop] - synthesised test tone" );
	}
}

} // namespace HF_Audio