#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfCommon_local.h"

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

/*
==============
hfCommonLocal::StartMainMenu
==============
*/
void hfCommonLocal::StartMenu( bool playIntro ) {
	if ( game && game->Shell_IsActive() ) {
		return;
	}

	if ( readDemo ) {
		// if we're playing a demo, esc kills it
		UnloadMap();
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
}

