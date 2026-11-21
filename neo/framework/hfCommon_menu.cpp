#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfCommon_local.h"
#include "../hflib/ui/hfUiFrontend.h"
#include "../renderer/hfUiDrawPass.h"

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
	if ( action == HF_Ui::hfUiFrontend::HF_UI_ACTION_QUIT ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
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
	uiFrontend->SetActionHandler( HF_FrontendActionHandler, NULL );
	hfUiDrawPass::SetSource( uiFrontend->RegistryPtr(), uiFrontend->SystemPtr() );
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
		return;
	}

	if ( game && game->Shell_IsActive() ) {
		return;
	}

	if ( game ) {
		game->Shell_Show( true );
		game->Shell_SyncWithSession();
	}

	console->Close();
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
			// In-game: only let frontend handle pause/back keys.
			if ( event->evType == SE_KEY && event->evValue2 == 1
				&& ( event->evValue == K_ESCAPE || event->evValue == K_JOY9 ) ) {
				if ( uiFrontend->HandleSysEvent( *event ) ) {
					return true;
				}
			}
		}
	}

	if ( game && game->Shell_IsActive() ) {
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
	if ( game ) {
		game->Shell_SyncWithSession();
	}
	FrontendFrame();
}

