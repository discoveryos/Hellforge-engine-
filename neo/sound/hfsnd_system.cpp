#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfsnd_local.h"

hfCVar s_noSound( "s_noSound", "0", CVAR_BOOL, "returns NULL for all sounds loaded and does not update the sound rendering" );

#ifdef HF_RETAIL
hfCVar s_useCompression( "s_useCompression", "1", CVAR_BOOL, "Use compressed sound files (mp3/xma)" );
hfCVar s_playDefaultSound( "s_playDefaultSound", "0", CVAR_BOOL, "play a beep for missing sounds" );
hfCVar s_maxSamples( "s_maxSamples", "5", CVAR_INTEGER, "max samples to load per shader" );
#else
hfCVar s_useCompression( "s_useCompression", "1", CVAR_BOOL, "Use compressed sound files (mp3/xma)" );
hfCVar s_playDefaultSound( "s_playDefaultSound", "1", CVAR_BOOL, "play a beep for missing sounds" );
hfCVar s_maxSamples( "s_maxSamples", "5", CVAR_INTEGER, "max samples to load per shader" );
#endif

hfCVar preLoad_Samples( "preLoad_Samples", "1", CVAR_SYSTEM | CVAR_BOOL, "preload samples during beginlevelload" );

hfSoundSystemLocal soundSystemLocal;
hfSoundSystem * soundSystem = &soundSystemLocal;

/*
================================================================================================

hfSoundSystemLocal

================================================================================================
*/

/*
========================
TestSound_f

This is called from the main thread.
========================
*/
void TestSound_f( const hfCmdArgs & args ) {
	if ( args.Argc() != 2 ) {
		hfLib::Printf( "Usage: testSound <file>\n" );
		return;
	}
	if ( soundSystemLocal.currentSoundWorld ) {
		soundSystemLocal.currentSoundWorld->PlayShaderDirectly( args.Argv( 1 ) );
	}
}

/*
========================
RestartSound_f
========================
*/
void RestartSound_f( const hfCmdArgs & args ) {
	soundSystemLocal.Restart();
}

/*
========================
ListSamples_f

========================
*/
void ListSamples_f( const hfCmdArgs & args ) {
	hfLib::Printf( "Sound samples\n-------------\n" );
	int totSize = 0;
	for ( int i = 0; i < soundSystemLocal.samples.Num(); i++ ) {
		hfLib::Printf( "%05dkb\t%s\n", soundSystemLocal.samples[ i ]->BufferSize() / 1024, soundSystemLocal.samples[ i ]->GetName() );
		totSize += soundSystemLocal.samples[ i ]->BufferSize();
	}
	hfLib::Printf( "--------------------------\n" );
	hfLib::Printf( "%05dkb total size\n", totSize / 1024 );
}

/*
========================
hfSoundSystemLocal::Restart
========================
*/
void hfSoundSystemLocal::Restart() {

	// Mute all channels in all worlds
	for ( int i = 0; i < soundWorlds.Num(); i++ ) {
		hfSoundWorldLocal * sw = soundWorlds[i];
		for ( int e = 0; e < sw->emitters.Num(); e++ ) {
			hfSoundEmitterLocal * emitter = sw->emitters[e];
			for ( int c = 0; c < emitter->channels.Num(); c++ ) {
				emitter->channels[c]->Mute();
			}
		}
	}
	// Shutdown sound hardware
	hardware.Shutdown();
	// Reinitialize sound hardware
	if ( !s_noSound.GetBool() ) {
		hardware.Init();
	}

	InitStreamBuffers();
}

/*
========================
hfSoundSystemLocal::Init

Initialize the SoundSystem.
========================
*/
void hfSoundSystemLocal::Init() {

	hfLib::Printf( "----- Initializing Sound System ------\n" );

	soundTime = Sys_Milliseconds();
	random.SetSeed( soundTime );

	if ( !s_noSound.GetBool() ) {
		hardware.Init();
		InitStreamBuffers();
	}

	cmdSystem->AddCommand( "testSound", TestSound_f, 0, "tests a sound", hfCmdSystem::ArgCompletion_SoundName );
	cmdSystem->AddCommand( "s_restart", RestartSound_f, 0, "restart sound system" );
	cmdSystem->AddCommand( "listSamples", ListSamples_f, 0, "lists all loaded sound samples" );

	hfLib::Printf( "sound system initialized.\n" );
	hfLib::Printf( "--------------------------------------\n" );
}

/*
========================
hfSoundSystemLocal::InitStreamBuffers
========================
*/
void hfSoundSystemLocal::InitStreamBuffers() {
	streamBufferMutex.Lock();
	const bool empty = ( bufferContexts.Num() == 0 );
	if ( empty ) {
		bufferContexts.SetNum( MAX_SOUND_BUFFERS );
		for ( int i = 0; i < MAX_SOUND_BUFFERS; i++ ) {
			freeStreamBufferContexts.Append( &( bufferContexts[ i ] ) );
		}
	} else {
		for ( int i = 0; i < activeStreamBufferContexts.Num(); i++ ) {
			freeStreamBufferContexts.Append( activeStreamBufferContexts[ i ] );
		}
		activeStreamBufferContexts.Clear();
	}
	assert( bufferContexts.Num() == MAX_SOUND_BUFFERS );
	assert( freeStreamBufferContexts.Num() == MAX_SOUND_BUFFERS );
	assert( activeStreamBufferContexts.Num() == 0 );
	streamBufferMutex.Unlock();
}

/*
========================
hfSoundSystemLocal::FreeStreamBuffers
========================
*/
void hfSoundSystemLocal::FreeStreamBuffers() {
	streamBufferMutex.Lock();
	bufferContexts.Clear();
	freeStreamBufferContexts.Clear();
	activeStreamBufferContexts.Clear();
	streamBufferMutex.Unlock();
}

/*
========================
hfSoundSystemLocal::Shutdown
========================
*/
void hfSoundSystemLocal::Shutdown() {
	hardware.Shutdown();
	FreeStreamBuffers();
	samples.DeleteContents( true );
	sampleHash.Free();
}

/*
========================
hfSoundSystemLocal::ObtainStreamBuffer

Get a stream buffer from the free pool, returns NULL if none are available
========================
*/
hfSoundSystemLocal::bufferContext_t * hfSoundSystemLocal::ObtainStreamBufferContext() {
	bufferContext_t * bufferContext = NULL;
	streamBufferMutex.Lock();
	if ( freeStreamBufferContexts.Num() != 0 ) {
		bufferContext = freeStreamBufferContexts[ freeStreamBufferContexts.Num() - 1 ];
		freeStreamBufferContexts.SetNum( freeStreamBufferContexts.Num() - 1 );
		activeStreamBufferContexts.Append( bufferContext );
	}
	streamBufferMutex.Unlock();
	return bufferContext;
}

/*
========================
hfSoundSystemLocal::ReleaseStreamBuffer

Releases a stream buffer back to the free pool
========================
*/
void hfSoundSystemLocal::ReleaseStreamBufferContext( bufferContext_t * bufferContext ) {
	streamBufferMutex.Lock();
	if ( activeStreamBufferContexts.Remove( bufferContext ) ) {
		freeStreamBufferContexts.Append( bufferContext );
	}
	streamBufferMutex.Unlock();
}

/*
========================
hfSoundSystemLocal::AllocSoundWorld
========================
*/
hfSoundWorld * hfSoundSystemLocal::AllocSoundWorld( hfRenderWorld *rw ) {
	hfSoundWorldLocal * local = new (TAG_AUDIO) hfSoundWorldLocal;
	local->renderWorld = rw;
	soundWorlds.Append( local );
	return local;
}

/*
========================
hfSoundSystemLocal::FreeSoundWorld
========================
*/
void hfSoundSystemLocal::FreeSoundWorld( hfSoundWorld *sw ) {
	hfSoundWorldLocal *local = static_cast<hfSoundWorldLocal*>( sw );
	soundWorlds.Remove( local );
	delete local;
}

/*
========================
hfSoundSystemLocal::SetPlayingSoundWorld

Specifying NULL will cause silence to be played.
========================
*/
void hfSoundSystemLocal::SetPlayingSoundWorld( hfSoundWorld *soundWorld ) {
	if ( currentSoundWorld == soundWorld ) {
		return;
	}
	hfSoundWorldLocal * oldSoundWorld = currentSoundWorld;

	currentSoundWorld = static_cast<hfSoundWorldLocal *>( soundWorld );

	if ( oldSoundWorld != NULL ) {
		oldSoundWorld->Update();
	}
}

/*
========================
hfSoundSystemLocal::GetPlayingSoundWorld
========================
*/
hfSoundWorld * hfSoundSystemLocal::GetPlayingSoundWorld() {
	return currentSoundWorld;
}

/*
========================
hfSoundSystemLocal::Render
========================
*/
void hfSoundSystemLocal::Render() {

	if ( s_noSound.GetBool() ) {
		return;
	}

	if ( needsRestart ) {
		needsRestart = false;
		Restart();
	}

	SCOPED_PROFILE_EVENT( "SoundSystem::Render" );

	if ( currentSoundWorld != NULL ) {
		currentSoundWorld->Update();
	}

	hardware.Update();

	// The sound system doesn't use game time or anything like that because the sounds are decoded in real time. 
	soundTime = Sys_Milliseconds();
}

/*
========================
hfSoundSystemLocal::OnReloadSound
========================
*/
void hfSoundSystemLocal::OnReloadSound( const hfDecl* sound ) {
	for ( int i = 0; i < soundWorlds.Num(); i++ ) {
		soundWorlds[i]->OnReloadSound( sound );
	}
}

/*
========================
hfSoundSystemLocal::StopAllSounds
========================
*/
void hfSoundSystemLocal::StopAllSounds() {
	for ( int i = 0; i < soundWorlds.Num(); i++ ) {
		hfSoundWorld *sw = soundWorlds[i];
		if ( sw ) {
			sw->StopAllSounds();
		}
	}
	hardware.Update();
}

/*
=======================
hfSoundSystemLocal::GetIXAudio2
=======================
*/
#if defined(_WIN32)
void * hfSoundSystemLocal::GetIXAudio2() const {
	return (void *)hardware.GetIXAudio2();
}
#else
void * hfSoundSystemLocal::GetIXAudio2() const {
	return NULL;
}
#endif

/*
========================
hfSoundSystemLocal::SoundTime
========================
*/
int hfSoundSystemLocal::SoundTime() const {
	return soundTime;
}

/*
========================
hfSoundSystemLocal::AllocateVoice
========================
*/
hfSoundVoice * hfSoundSystemLocal::AllocateVoice( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample ) {
	return hardware.AllocateVoice( leadinSample, loopingSample );
}

/*
========================
hfSoundSystemLocal::FreeVoice
========================
*/
void hfSoundSystemLocal::FreeVoice( hfSoundVoice * voice ) {
	hardware.FreeVoice( voice );
}

/*
========================
hfSoundSystemLocal::LoadSample
========================
*/
hfSoundSample * hfSoundSystemLocal::LoadSample( const char * name ) {
	hfStrStatic< MAX_OSPATH > canonical = name;
	canonical.ToLower();
	canonical.BackSlashesToSlashes();
	canonical.StripFileExtension();
	int hashKey = hfStr::Hash( canonical );
	for ( int i = sampleHash.First( hashKey ); i != -1; i = sampleHash.Next( i ) ) {
		if ( hfStr::Cmp( samples[i]->GetName(), canonical ) == 0 ) {
			samples[i]->SetLevelLoadReferenced();
			return samples[i];
		}
	}
	hfSoundSample * sample = new (TAG_AUDIO) hfSoundSample;
	sample->SetName( canonical );
	sampleHash.Add( hashKey, samples.Append( sample ) );
	if ( !insideLevelLoad ) {
		// Sound sample referenced before any map is loaded
		sample->SetNeverPurge();
		sample->LoadResource();
	} else {
		sample->SetLevelLoadReferenced();
	}

	if ( cvarSystem->GetCVarBool( "fs_buildgame" ) ) {
		fileSystem->AddSamplePreload( canonical );
	}

	return sample;
}

/*
========================
hfSoundSystemLocal::StopVoicesWithSample

A sample is about to be freed, make sure the hardware isn't mixing from it.
========================
*/
void hfSoundSystemLocal::StopVoicesWithSample( const hfSoundSample * const sample ) {
	for ( int w = 0; w < soundWorlds.Num(); w++ ) {
		hfSoundWorldLocal * sw = soundWorlds[w];
		if ( sw == NULL ) {
			continue;
		}
		for ( int e = 0; e < sw->emitters.Num(); e++ ) {
			hfSoundEmitterLocal * emitter = sw->emitters[e];
			if ( emitter == NULL ) {
				continue;
			}
			for ( int i = 0; i < emitter->channels.Num(); i++ ) {
				if ( emitter->channels[i]->leadinSample == sample || emitter->channels[i]->loopingSample == sample ) {
					emitter->channels[i]->Mute();
				}
			}
		}
	}
}

/*
========================
hfSoundSystemLocal::FreeVoice
========================
*/
cinData_t hfSoundSystemLocal::ImageForTime( const int milliseconds, const bool waveform ) {
	cinData_t cd;
	cd.imageY = NULL;
	cd.imageCr = NULL;
	cd.imageCb = NULL;
	cd.imageWidth = 0;
	cd.imageHeight = 0;
	cd.status = FMV_IDLE;
	return cd;
}

/*
========================
hfSoundSystemLocal::BeginLevelLoad
========================
*/
void hfSoundSystemLocal::BeginLevelLoad() {
	insideLevelLoad = true;
	for ( int i = 0; i < samples.Num(); i++ ) {
		if ( samples[i]->GetNeverPurge() ) {
			continue;
		}
		samples[i]->FreeData();
		samples[i]->ResetLevelLoadReferenced();
	}
}



/*
========================
hfSoundSystemLocal::Preload
========================
*/
void hfSoundSystemLocal::Preload( hfPreloadManifest & manifest ) {
	
	hfStrStatic< MAX_OSPATH > filename;
	
	int	start = Sys_Milliseconds();
	int numLoaded = 0;

	hfList< preloadSort_t > preloadSort;
	preloadSort.Resize( manifest.NumResources() );
	for ( int i = 0; i < manifest.NumResources(); i++ ) {
		const preloadEntry_s & p = manifest.GetPreloadByIndex( i );
		hfResourceCacheEntry rc;
		// FIXME: write these out sorted
		if ( p.resType == PRELOAD_SAMPLE ) {
			if ( p.resourceName.Find( "/vo/", false ) >= 0 ) {
				continue;
			} 
			filename  = "generated/";
			filename += p.resourceName;
			filename.SetFileExtension( "idwav" );
			if ( fileSystem->GetResourceCacheEntry( filename, rc ) ) {
				preloadSort_t ps = {};
				ps.idx = i;
				ps.ofs = rc.offset;
				preloadSort.Append( ps );
			}
		}
	}

	preloadSort.SortWithTemplate( hfSort_Preload() );

	for ( int i = 0; i < preloadSort.Num(); i++ ) {
		const preloadSort_t & ps = preloadSort[ i ];
		const preloadEntry_s & p = manifest.GetPreloadByIndex( ps.idx );
		filename = p.resourceName;
		filename.Replace( "generated/", "" );
		numLoaded++;
		hfSoundSample *sample = LoadSample( filename );
		if ( sample != NULL && !sample->IsLoaded() ) {
			sample->LoadResource();
			sample->SetLevelLoadReferenced();
		}
	}

	int	end = Sys_Milliseconds();
	common->Printf( "%05d sounds preloaded in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
	common->Printf( "----------------------------------------\n" );
}

/*
========================
hfSoundSystemLocal::EndLevelLoad
========================
*/
void hfSoundSystemLocal::EndLevelLoad() {

	insideLevelLoad = false;

	common->Printf( "----- hfSoundSystemLocal::EndLevelLoad -----\n" );
	int		start = Sys_Milliseconds();
	int		keepCount = 0;
	int		loadCount = 0;

	hfList< preloadSort_t > preloadSort;
	preloadSort.Resize( samples.Num() );

	for ( int i = 0; i < samples.Num(); i++ ) {
		common->UpdateLevelLoadPacifier();


		if ( samples[i]->GetNeverPurge() ) {
			continue;
		}
		if ( samples[i]->IsLoaded() ) {
			keepCount++;
			continue;
		}
		if ( samples[i]->GetLevelLoadReferenced() ) {
			hfStrStatic< MAX_OSPATH > filename  = "generated/";
			filename += samples[ i ]->GetName();
			filename.SetFileExtension( "idwav" );
			preloadSort_t ps = {};
			ps.idx = i;
			hfResourceCacheEntry rc;
			if ( fileSystem->GetResourceCacheEntry( filename, rc ) ) {
				ps.ofs = rc.offset;
			} else {
				ps.ofs = 0;
			}
			preloadSort.Append( ps );
			loadCount++;
		}
	}
	preloadSort.SortWithTemplate( hfSort_Preload() );
	for ( int i = 0; i < preloadSort.Num(); i++ ) {
		common->UpdateLevelLoadPacifier();


		samples[ preloadSort[ i ].idx ]->LoadResource();
	}
	int	end = Sys_Milliseconds();

	common->Printf( "%5i sounds loaded in %5.1f seconds\n", loadCount, (end-start) * 0.001 );
	common->Printf( "----------------------------------------\n" );
}



/*
========================
hfSoundSystemLocal::FreeVoice
========================
*/
void hfSoundSystemLocal::PrintMemInfo( MemInfo_t *mi ) {
}

