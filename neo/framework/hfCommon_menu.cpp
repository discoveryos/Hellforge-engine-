#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfCommon_local.h"
#include "../hflib/ui/hfUiFrontend.h"
#include "../hflib/ui/hfUiSettings.h"
#include "../hflib/game/hfArenaSave.h"
#include "../renderer/hfUiDrawPass.h"

// A real map is selected by the host, not baked into the UI.  When empty,
// START falls back to the deterministic arena that ships with the frontend.
static hfCVar ui_startMap( "ui_startMap", "", CVAR_ARCHIVE, "map to load when START is pressed" );
static hfCVar ui_startDevMap( "ui_startDevMap", "0", CVAR_BOOL | CVAR_ARCHIVE, "load ui_startMap in developer mode" );

// Engine-owned arena: the save/load policy has a concrete snapshot to write
// when no real game module is installed (the harness and map-less checkouts).
static HfArena						s_frontendArena;

// Durable mirror of the options screen.  Loaded from the signed-in user's
// console slot at boot; edits are written back and applied to live cvars.
static HF_Ui::hfUiSettings	s_frontendSettings;
static float				s_frontendMusic = -1.0f;
static float				s_frontendSfx = -1.0f;
static bool					s_frontendFullscreen = false;

/*
==============
hfCommonLocal::InitializeMPMapsModes
==============
*/
void hfCommonLocal::InitializeMPMapsModes() {

	const char ** gameModes = NULL;
	const char ** gameModesDisplay = NULL;
	int numModes = game->GetMPGameModes( &gameModes, &gameModesDisplay );
	mpGameModes.SetNum( numModes );
	for ( int i = 0; i < numModes; i++ ) {
		mpGameModes[i] = gameModes[i];
	}
	mpDisplayGameModes.SetNum( numModes );
	for ( int i = 0; i < numModes; i++ ) {
		mpDisplayGameModes[i] = gameModesDisplay[i];
	}
	int numMaps = declManager->GetNumDecls( DECL_MAPDEF );
	mpGameMaps.Clear();
	for ( int i = 0; i < numMaps; i++ ) {
		const hfDeclEntityDef * mapDef = static_cast<const hfDeclEntityDef *>( declManager->DeclByIndex( DECL_MAPDEF, i ) );
		uint32 supportedModes = 0;
		for ( int j = 0; j < numModes; j++ ) {
			if ( mapDef->dict.GetBool( gameModes[j], false ) ) {
				supportedModes |= BIT(j);
			}
		}
		if ( supportedModes != 0 ) {
			mpMap_t & mpMap = mpGameMaps.Alloc();
			mpMap.mapFile = mapDef->GetName();
			mpMap.mapName = mapDef->dict.GetString( "name", mpMap.mapFile );
			mpMap.supportedModes = supportedModes;
		}
	}
}

/*
==============
hfCommonLocal::OnStartHosting
==============
*/
void hfCommonLocal::OnStartHosting( hfMatchParameters & parms ) {
	if ( ( parms.matchFlags & MATCH_REQUIRE_PARTY_LOBBY ) == 0 ) {
		return; // This is the party lobby or a SP match
	}

	// If we were searching for a random match but didn't find one, we'll need to select parameters now
	if ( parms.gameMap < 0 ) {
		if ( parms.gameMode < 0 ) {
			// Random mode means any map will do
			parms.gameMap = hfLib::frameNumber % mpGameMaps.Num();
		} else {
			// Select a map which supports the chosen mode
			hfList<int> supportedMaps;
			uint32 supportedMode = BIT( parms.gameMode );
			for ( int i = 0; i < mpGameMaps.Num(); i++ ) {
				if ( mpGameMaps[i].supportedModes & supportedMode ) {
					supportedMaps.Append( i );
				}
			}
			if ( supportedMaps.Num() == 0 ) {
				// We don't have any maps which support the chosen mode...
				parms.gameMap = hfLib::frameNumber % mpGameMaps.Num();
				parms.gameMode = -1;
			} else {
				parms.gameMap = supportedMaps[ hfLib::frameNumber % supportedMaps.Num() ];
			}
		}
	}
	if ( parms.gameMode < 0 ) {
		uint32 supportedModes = mpGameMaps[parms.gameMap].supportedModes;
		int8 supportedModeList[32] = {};
		int numSupportedModes = 0;
		for ( int i = 0; i < 32; i++ ) {
			if ( supportedModes & BIT(i) ) {
				supportedModeList[numSupportedModes] = i;
				numSupportedModes++;
			}
		}
		parms.gameMode = supportedModeList[ ( hfLib::frameNumber / mpGameMaps.Num() ) % numSupportedModes ];
	}
	parms.mapName = mpGameMaps[parms.gameMap].mapFile;
	parms.numSlots = session->GetTitleStorageInt("MAX_PLAYERS_ALLOWED", 4 );
}

static void HF_FrontendActionHandler( int action, void * /*user*/ ) {
	using namespace HF_Ui;
	// The handler is a free function, so reach the installed frontend through
	// the engine singleton rather than the hfCommonLocal member name.
	HF_Ui::hfUiFrontend *uiFrontend = commonLocal.GetFrontend();
	if ( uiFrontend == NULL ) {
		return;
	}

	switch ( action ) {
		case hfUiFrontend::HF_UI_ACTION_QUIT:
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
			break;

		case hfUiFrontend::HF_UI_ACTION_START: {
			// Buffer the real map command instead of calling StartNewGame()
			// from inside a UI update.  The session's LOADING state drives
			// ExecuteMapChange(), which owns the LOADING -> PLAYING switch.
			const char *mapName = ui_startMap.GetString();
			if ( mapName != NULL && mapName[ 0 ] != '\0' ) {
				char command[ 256 ];
				snprintf( command, sizeof( command ), "%s %s\n",
						  ui_startDevMap.GetBool() ? "devmap" : "map", mapName );
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, command );
				uiFrontend->SetState( hfUiFrontend::HF_UI_FRONTEND_LOADING );
				uiFrontend->MarkHostHandled();
			}
			break;
		}

		case hfUiFrontend::HF_UI_ACTION_SAVE_SLOT_WRITE: {
			hfArenaSave snapshot;
			const int slot = uiFrontend->GetLastSaveSlot();
			HfArena *arena = uiFrontend->GetArena();
			hfconsole::hfConsoleServices *services = hfconsole::HfConsole_GetServices();
			if ( arena == NULL || slot < 0 || services == NULL ) { break; }
			snapshot.Capture( *arena );
			if ( snapshot.SaveToSlot( services, services->GetUserId( 0 ),
									  hfUiSaveSlots::SlotName( slot ) ) ) {
				uiFrontend->SaveSlots().Refresh( *uiFrontend->RegistryPtr(), uiFrontend->System() );
			}
			break;
		}

		case hfUiFrontend::HF_UI_ACTION_SAVE_SLOT_LOAD: {
			hfArenaSave snapshot;
			const int slot = uiFrontend->GetLastSaveSlot();
			HfArena *arena = uiFrontend->GetArena();
			hfconsole::hfConsoleServices *services = hfconsole::HfConsole_GetServices();
			if ( arena == NULL || slot < 0 || services == NULL ) { break; }
			if ( snapshot.LoadFromSlot( services, services->GetUserId( 0 ),
										hfUiSaveSlots::SlotName( slot ) )
				&& snapshot.Restore( *arena ) ) {
				uiFrontend->SetState( arena->gameOver
									  ? hfUiFrontend::HF_UI_FRONTEND_RESULTS
									  : hfUiFrontend::HF_UI_FRONTEND_PLAYING );
			}
			break;
		}

		default:
			break;
	}
}

/*
==============
hfCommonLocal::InitFrontend
==============
*/
void hfCommonLocal::InitFrontend() {
	if ( uiFrontend != NULL ) {
		return;
	}
	uiEventBus = new HF_Ecs::EventBus();
	uiFrontend = new HF_Ui::hfUiFrontend();

	float w = 1280.0f;
	float h = 720.0f;
	if ( renderSystem != NULL ) {
		w = ( float )renderSystem->GetWidth();
		h = ( float )renderSystem->GetHeight();
	}
	uiFrontend->Init( *uiEventBus, w, h );
	uiFrontend->SetArena( &s_frontendArena );
	s_frontendArena.Reset( 1 );
	uiFrontend->SetActionHandler( HF_FrontendActionHandler, NULL );
	hfUiDrawPass::SetSource( uiFrontend->RegistryPtr(), uiFrontend->SystemPtr() );
	hfUiDrawPass::SetGameplayOverlay( uiFrontend->GetArena() );

	hfconsole::hfConsoleServices *services = hfconsole::HfConsole_GetServices();
	const hfconsole::hfConsoleUserId user = ( services != NULL ) ? services->GetUserId( 0 )
															 : hfconsole::hfConsoleUserId( 0 );
	if ( services != NULL ) {
		s_frontendSettings.LoadFromSlot( services, user );
	}
	s_frontendSettings.WriteTo( *uiFrontend );
	s_frontendSettings.ApplyToCVars();
	s_frontendMusic = s_frontendSettings.musicVolume;
	s_frontendSfx = s_frontendSettings.sfxVolume;
	s_frontendFullscreen = s_frontendSettings.fullscreen;
}

/*
==============
hfCommonLocal::ShutdownFrontend
==============
*/
void hfCommonLocal::ShutdownFrontend() {
	if ( uiFrontend != NULL ) {
		uiFrontend->Shutdown();
		delete uiFrontend;
		uiFrontend = NULL;
	}
	if ( uiEventBus != NULL ) {
		delete uiEventBus;
		uiEventBus = NULL;
	}
	hfUiDrawPass::SetSource( NULL, NULL );
}

/*
==============
hfCommonLocal::FrontendFrame
==============
*/
void hfCommonLocal::FrontendFrame() {
	if ( uiFrontend == NULL ) {
		return;
	}
	// Keep viewport in sync with the renderer.
	if ( renderSystem != NULL ) {
		const float w = ( float )renderSystem->GetWidth();
		const float h = ( float )renderSystem->GetHeight();
		if ( w > 0.0f && h > 0.0f ) {
			uiFrontend->System().SetViewport( w, h );
		}
	}
	// Advance the frontend (fixed-step sim happens inside when PLAYING).
	uiFrontend->Update( 1.0f / 60.0f );

	// The renderer may draw the arena layer only while PLAYING.  Deriving the
	// gate from the frontend state here keeps ownership in one place.
	hfUiDrawPass::SetGameplayOverlayEnabled(
		uiFrontend->GetState() == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_PLAYING );

	// Options edits are durable and take effect live.  Compare against the
	// last applied mirror so a drag writes once per changed value, not every
	// frame while the mouse is idle.
	{
		const float music = uiFrontend->GetMusicVolume();
		const float sfx = uiFrontend->GetSfxVolume();
		const bool fullscreen = uiFrontend->GetFullscreen();
		if ( music != s_frontendMusic || sfx != s_frontendSfx || fullscreen != s_frontendFullscreen ) {
			s_frontendMusic = music;
			s_frontendSfx = sfx;
			s_frontendFullscreen = fullscreen;
			s_frontendSettings.ReadFrom( *uiFrontend );
			s_frontendSettings.ApplyToCVars();
			hfconsole::hfConsoleServices *services = hfconsole::HfConsole_GetServices();
			if ( services != NULL ) {
				s_frontendSettings.SaveToSlot( services, services->GetUserId( 0 ) );
			}
		}
	}

	if ( uiFrontend->QuitRequested() ) {
		uiFrontend->ClearQuitRequest();
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
==============
hfCommonLocal::StartMainMenu
==============
*/
void hfCommonLocal::StartMenu( bool playIntro ) {
	if ( readDemo ) {
		// if we're playing a demo, esc kills it
		UnloadMap();
	}

	InitFrontend();
	if ( uiFrontend != NULL ) {
		hfUiDrawPass::SetSource( uiFrontend->RegistryPtr(), uiFrontend->SystemPtr() );
		if ( !mapSpawned ) {
			uiFrontend->SetState( HF_Ui::hfUiFrontend::HF_UI_FRONTEND_TITLE );
		} else if ( uiFrontend->GetState() != HF_Ui::hfUiFrontend::HF_UI_FRONTEND_PAUSED ) {
			uiFrontend->SetState( HF_Ui::hfUiFrontend::HF_UI_FRONTEND_PAUSED );
		}
		console->Close();
	}
}

/*
===============
hfCommonLocal::ExitMenu
===============
*/
void hfCommonLocal::ExitMenu() {
	if ( uiFrontend != NULL ) {
		const HF_Ui::hfUiFrontend::hfUiFrontendState s = uiFrontend->GetState();
		if ( s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_PAUSED ) {
			uiFrontend->SetState( HF_Ui::hfUiFrontend::HF_UI_FRONTEND_PLAYING );
			return;
		}
		if ( s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_MENU ) {
			uiFrontend->SetState( HF_Ui::hfUiFrontend::HF_UI_FRONTEND_TITLE );
			return;
		}
	}
	if ( game ) {
		game->Shell_Show( false );
	}
}

/*
==============
hfCommonLocal::MenuEvent

Executes any commands returned by the gui
==============
*/
bool hfCommonLocal::MenuEvent( const sysEvent_t * event ) {

	if ( session->GetSignInManager().ProcessInputEvent( event ) ) {
		return true;
	}

	if ( uiFrontend != NULL ) {
		const HF_Ui::hfUiFrontend::hfUiFrontendState s = uiFrontend->GetState();
		const bool frontendActive = ( s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_TITLE
			|| s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_MENU
			|| s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_LOADING
			|| s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_PAUSED
			|| s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_RESULTS );
		if ( frontendActive ) {
			if ( uiFrontend->HandleSysEvent( *event ) ) {
				return true;
			}
		} else if ( s == HF_Ui::hfUiFrontend::HF_UI_FRONTEND_PLAYING ) {
			// In-game: sample gameplay intent first.  HandleGameInput leaves
			// console/pause keys alone, so those still reach the host below.
			if ( uiFrontend->HandleGameInput( *event ) ) {
				return true;
			}
			if ( event->evType == SE_KEY && event->evValue2 == 1
				&& ( event->evValue == K_ESCAPE || event->evValue == K_JOY9 ) ) {
				if ( uiFrontend->HandleSysEvent( *event ) ) {
					return true;
				}
			}
		}
	}

	// The new frontend owns menu input.  Only fall back to the legacy shell
	// when no frontend is installed (e.g. a host that opts out at boot).
	if ( uiFrontend == NULL && game && game->Shell_IsActive() ) {
		return game->Shell_HandleGuiEvent( event );
	}

	if ( game ) {
		return game->HandlePlayerGuiEvent( event );
	}

	return false;
}

/*
=================
hfCommonLocal::GuiFrameEvents
=================
*/
void hfCommonLocal::GuiFrameEvents() {
	if ( uiFrontend == NULL && game ) {
		game->Shell_SyncWithSession();
	}
	FrontendFrame();
}

