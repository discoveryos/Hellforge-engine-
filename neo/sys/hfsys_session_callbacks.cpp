#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "../framework/hfCommon_local.h"
#include "hfsys_session_local.h"

// The more the hfLobby class needs to call back into this class, the more likely we're doing something wrong, and there is a better way.

/*
========================
hfSessionLocalCallbacks::BecomingHost
This is called when 
========================
*/
bool hfSessionLocalCallbacks::BecomingHost( hfLobby & lobby ) {
	if ( lobby.lobbyType == hfLobby::TYPE_GAME ) {
		if ( sessionLocal->GetActivePlatformLobby() != &lobby ) {
			hfLib::Printf( "BecomingHost: Must be past the party lobby to become host of a game lobby.\n" );
			return false;
		}
		if ( sessionLocal->localState == hfSessionLocal::STATE_INGAME || sessionLocal->localState == hfSessionLocal::STATE_LOADING ) {
			// If we are in a game, go back to the lobby before becoming the new host of a game lobby
			sessionLocal->SetState( hfSessionLocal::STATE_GAME_LOBBY_PEER );

			// session mgr housekeeping that would usually be done through the standard EndMatch path
			sessionLocal->EndMatchForMigration();
		}
	}

	return true;
}

/*
========================
hfSessionLocalCallbacks::BecameHost
========================
*/
void hfSessionLocalCallbacks::BecameHost( hfLobby & lobby ) {
	// If we were in the lobby when we switched to host, then set the right state
	if ( lobby.lobbyType == hfLobby::TYPE_PARTY && sessionLocal->localState == hfSessionLocal::STATE_PARTY_LOBBY_PEER ) {
		sessionLocal->SetState( hfSessionLocal::STATE_PARTY_LOBBY_HOST );
	} else if ( lobby.lobbyType == hfLobby::TYPE_GAME && sessionLocal->localState == hfSessionLocal::STATE_GAME_LOBBY_PEER ) {
		sessionLocal->SetState( hfSessionLocal::STATE_GAME_LOBBY_HOST );
	}
}

/*
========================
hfSessionLocalCallbacks::BecomingPeer
========================
*/
bool hfSessionLocalCallbacks::BecomingPeer( hfLobby & lobby ) {
	if ( lobby.lobbyType == hfLobby::TYPE_GAME ) {
		if ( sessionLocal->localState == hfSessionLocal::STATE_INGAME || sessionLocal->localState == hfSessionLocal::STATE_LOADING ) {
			// Go to the party lobby while we try to connect to the new host
			// This isn't totally necessary but we want to end the current game now and go to some screen.
			// When the connection goes through or fails will send the session mgr to the appropriate state (game lobby or main menu)

			// What happens if we got the game migration before the party migration?
			sessionLocal->SetState( sessionLocal->GetPartyLobby().IsHost() ? hfSessionLocal::STATE_PARTY_LOBBY_HOST : hfSessionLocal::STATE_PARTY_LOBBY_PEER );
			
			// session mgr housekeeping that would usually be done through the standard EndMatch path
			sessionLocal->EndMatchForMigration();

			return true; // return true tells the session that we want him to tell us when the connects/fails
		}
	}
	return false;
}

/*
========================
hfSessionLocalCallbacks::BecamePeer
========================
*/
void hfSessionLocalCallbacks::BecamePeer( hfLobby & lobby ) {
	if ( lobby.lobbyType == hfLobby::TYPE_GAME ) {
		sessionLocal->SetState( hfSessionLocal::STATE_GAME_LOBBY_PEER );
	}
}

/*
========================
hfSessionLocalCallbacks::FailedGameMigration
========================
*/
void hfSessionLocalCallbacks::FailedGameMigration( hfLobby & lobby ) {
	// We failed to complete a game migration this could happen for a couple reasons:
	// -The network invites failed / failed to join migrated session
	// -There was nobody to invite
	lobby.ResetAllMigrationState();
	if ( lobby.lobbyType == hfLobby::TYPE_GAME ) { // this check is a  redundant since we should only get this CB from the game session

		sessionLocal->SetState( hfSessionLocal::STATE_GAME_LOBBY_HOST );

		// Make sure the sessions are joinable again
		sessionLocal->EndSessions();
	}
}

/*
========================
hfSessionLocalCallbacks::MigrationEnded
========================
*/
void hfSessionLocalCallbacks::MigrationEnded( hfLobby & lobby ) {
	if ( lobby.migrationInfo.persistUntilGameEndsData.wasMigratedGame ) {
#if 1
		if ( lobby.lobbyType == hfLobby::TYPE_GAME || ( lobby.lobbyType == hfLobby::TYPE_PARTY && session->GetState() <= hfSession::PARTY_LOBBY ) ) {
			common->Dialog().ClearDialog( GDM_MIGRATING );
			common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
			common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );

			if ( lobby.GetNumLobbyUsers() <= 1 ) {
				if ( MatchTypeHasStats( lobby.parms.matchFlags ) ) {
					common->Dialog().AddDialog( GDM_MIGRATING_FAILED_DISBANDED_STATS, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Game has disbanded
				} else {
					common->Dialog().AddDialog( GDM_MIGRATING_FAILED_DISBANDED, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Game has disbanded
				}
			} else {
				//common->Dialog().AddDialog( GDM_MIGRATING_FAILED_CONNECTION, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Lost connection to game
				if ( lobby.lobbyType == hfLobby::TYPE_GAME && MatchTypeHasStats( lobby.parms.matchFlags ) ) {
					// This means we came from a public match, so tell them they didn't lose stats
					common->Dialog().AddDialog( GDM_HOST_CONNECTION_LOST_STATS, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// The connection to the host has been lost. This game will not count towards your ranking.
				} else {
					// This means we came from a private match, just say host quit
					common->Dialog().AddDialog( GDM_HOST_CONNECTION_LOST, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );			// The connection to the host has been lost.
				}
			}

			lobby.ResetAllMigrationState();
				
			// Make sure the sessions are joinable again
			sessionLocal->EndSessions();
		}
#else
		// If we get here, we migrated from a game
		if ( lobby.GetNumLobbyUsers() <= 1 && lobby.lobbyType == hfLobby::TYPE_GAME ) {
			if ( !MatchTypeIsJoinInProgress( lobby.parms.matchFlags ) ) {
				// Handles 'soft' failed game migration where we migrated from a game and are now alone
				gameDialogMessages_t errorDlg = GDM_INVALID;
				lobby.migrationInfo.persistUntilGameEndsData.hasGameData = false; // never restart the game if we are by ourselves
				if ( lobby.migrationInfo.invites.Num() > 0 ) {
					// outstanding invites: migration failed
					errorDlg = ( MatchTypeHasStats( lobby.migrateMsgFlags ) && ( sessionLocal->GetFlushedStats() == false ) ) ? GDM_MIGRATING_FAILED_CONNECTION_STATS : GDM_MIGRATING_FAILED_CONNECTION;
				} else {
					// there was no one to invite
					errorDlg = ( MatchTypeHasStats( lobby.migrateMsgFlags ) && ( sessionLocal->GetFlushedStats() == false ) ) ? GDM_MIGRATING_FAILED_DISBANDED_STATS : GDM_MIGRATING_FAILED_DISBANDED;
				}
				if ( errorDlg != GDM_INVALID ) {
					common->Dialog().AddDialog( errorDlg, DIALOG_ACCEPT, NULL, NULL, false );
				}
				common->Dialog().ClearDialog( GDM_MIGRATING );
				common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
				common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );

				FailedGameMigration( lobby );
			}
		} else if ( lobby.lobbyType == hfLobby::TYPE_PARTY ) {
			if ( session->GetState() <= hfSession::PARTY_LOBBY ) {
				// We got dropped the party lobby, let them know what happened
				common->Dialog().ClearDialog( GDM_MIGRATING );
				common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
				common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );

				if ( lobby.GetNumLobbyUsers() <= 1 ) {
					common->Dialog().AddDialog( GDM_MIGRATING_FAILED_DISBANDED, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Game has disbanded
				} else {
					//common->Dialog().AddDialog( GDM_MIGRATING_FAILED_CONNECTION, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );	// Lost connection to game
					common->Dialog().AddDialog( GDM_HOST_CONNECTION_LOST_STATS, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );
				}

				lobby.ResetAllMigrationState();
				
				// Make sure the sessions are joinable again
				sessionLocal->EndSessions();
			}
		}
#endif
	} else if ( lobby.GetNumLobbyUsers() <= 1 && session->GetState() == hfSession::PARTY_LOBBY ) {
		// If they didn't come from a game, and are by themselves, just show the lobby disband msg
		common->Dialog().AddDialog( GDM_LOBBY_DISBANDED, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );				// The lobby you were previously in has disbanded

		// Make sure the sessions are joinable again
		sessionLocal->EndSessions();
	}
}

/*
========================
hfSessionLocalCallbacks::GoodbyeFromHost
========================
*/
void hfSessionLocalCallbacks::GoodbyeFromHost( hfLobby & lobby, int peerNum, const lobbyAddress_t & remoteAddress, int msgType ) {
	sessionLocal->GoodbyeFromHost( lobby, peerNum, remoteAddress, msgType );	
}

/*
========================
hfSessionLocalCallbacks::AnyPeerHasAddress
========================
*/
bool hfSessionLocalCallbacks::AnyPeerHasAddress( const lobbyAddress_t & remoteAddress ) const { 
	return sessionLocal->GetPartyLobby().FindAnyPeer( remoteAddress ) || sessionLocal->GetGameLobby().FindAnyPeer( remoteAddress ); 
}

/*
========================
hfSessionLocalCallbacks::RecvLeaderboardStats
========================
*/
void hfSessionLocalCallbacks::RecvLeaderboardStats( hfBitMsg & msg ) {
	// Steam and PS3 just write them as they come per player, they don't need to flush
	sessionLocal->RecvLeaderboardStatsForPlayer( msg );
}

/*
========================
hfSessionLocalCallbacks::ReceivedFullSnap
========================
*/
void hfSessionLocalCallbacks::ReceivedFullSnap() {
	// If we received a full snap, then we can transition into the INGAME state
	sessionLocal->numFullSnapsReceived++;

	if ( sessionLocal->numFullSnapsReceived < 2 ) {
		return;
	}

	if ( sessionLocal->localState != hfSessionLocal::STATE_INGAME ) {
		sessionLocal->GetActingGameStateLobby().QueueReliableMessage( sessionLocal->GetActingGameStateLobby().host, hfLobby::RELIABLE_IN_GAME );		// Let host know we are in game now
		sessionLocal->SetState( hfSessionLocal::STATE_INGAME );
	}
}

/*
========================
hfSessionLocalCallbacks::LeaveGameLobby
========================
*/
void hfSessionLocalCallbacks::LeaveGameLobby() {
	
	// Make sure we're in the game lobby
	if ( session->GetState() != hfSession::GAME_LOBBY ) {
		return;
	}

	// If we're the host of the party, only we are allowed to make this call
	if ( sessionLocal->GetPartyLobby().IsHost() ) {
		return;
	}

	sessionLocal->GetGameLobby().Shutdown();
	sessionLocal->SetState( hfSessionLocal::STATE_PARTY_LOBBY_PEER );
}

/*
========================
hfSessionLocalCallbacks::PrePickNewHost
This is called when we have determined that we need to pick a new host.
Call PickNewHostInternal to continue on with the host picking process.
========================
*/
void hfSessionLocalCallbacks::PrePickNewHost( hfLobby & lobby, bool forceMe, bool inviteOldHost ) {
	sessionLocal->PrePickNewHost( lobby, forceMe, inviteOldHost );
}

/*
========================
hfSessionLocalCallbacks::PreMigrateInvite
This is called just before we get invited to a migrated session
If we return false, the invite will be ignored
========================
*/
bool hfSessionLocalCallbacks::PreMigrateInvite( hfLobby & lobby ) {
	return sessionLocal->PreMigrateInvite( lobby );
}

/*
========================
hfSessionLocalCallbacks::ConnectAndMoveToLobby
========================
*/
void hfSessionLocalCallbacks::ConnectAndMoveToLobby( hfLobby::lobbyType_t destLobbyType, const lobbyConnectInfo_t & connectInfo, bool waitForPartyOk ) {
	
	// See if we are already in the game lobby
	hfLobby * lobby = sessionLocal->GetLobbyFromType( destLobbyType );

	if ( lobby == NULL ) {
		hfLib::Printf( "RELIABLE_CONNECT_AND_MOVE_TO_LOBBY: Invalid lobby type.\n" ); 
		return;
	}

	if ( lobby->lobbyBackend != NULL && lobby->lobbyBackend->IsOwnerOfConnectInfo( connectInfo ) ) {
		hfLib::Printf( "RELIABLE_CONNECT_AND_MOVE_TO_LOBBY: Already in lobby.\n" ); 
		return;
	}

	// See if we are in a game, or loading into a game.  If so, ignore invites from our party host
	if ( destLobbyType == hfLobby::TYPE_GAME || destLobbyType == hfLobby::TYPE_GAME_STATE ) { 
		if ( GetState() == hfSession::INGAME || GetState() == hfSession::LOADING ) {
			hfLib::Printf( "RELIABLE_CONNECT_AND_MOVE_TO_LOBBY: In a different game, ignoring.\n" ); 
			return;
		}
	}

	// End current game lobby
	lobby->Shutdown();

	// waitForPartyOk will be true if the party host wants us to wait for his ok to stay in the lobby
	lobby->waitForPartyOk = waitForPartyOk;
		
	// Connect to new game lobby
	sessionLocal->ConnectAndMoveToLobby( *lobby, connectInfo, true );		// Consider this an invite if party host told us to connect
}

/*
========================
hfSessionLocalCallbacks::HandleServerQueryRequest
========================
*/
void hfSessionLocalCallbacks::HandleServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType ) {
	sessionLocal->HandleServerQueryRequest( remoteAddr, msg, msgType );
}

/*
========================
hfSessionLocalCallbacks::HandleServerQueryAck
========================
*/
void hfSessionLocalCallbacks::HandleServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg ) {
	sessionLocal->HandleServerQueryAck( remoteAddr, msg );
}

extern hfCVar net_headlessServer;

/*
========================
hfSessionLocalCallbacks::HandlePeerMatchParamUpdate
========================
*/
void hfSessionLocalCallbacks::HandlePeerMatchParamUpdate( int peer, int msg ) {
	if ( net_headlessServer.GetBool() ) {
		sessionLocal->storedPeer = peer;
		sessionLocal->storedMsgType = msg;
	}
}

/*
========================
hfSessionLocalCallbacks::CreateLobbyBackend
========================
*/
hfLobbyBackend * hfSessionLocalCallbacks::CreateLobbyBackend( const hfMatchParameters & p, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	return sessionLocal->CreateLobbyBackend( p, skillLevel, lobbyType );
}

/*
========================
hfSessionLocalCallbacks::FindLobbyBackend
========================
*/
hfLobbyBackend * hfSessionLocalCallbacks::FindLobbyBackend( const hfMatchParameters & p, int numPartyUsers, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	return sessionLocal->FindLobbyBackend( p, numPartyUsers, skillLevel, lobbyType );
}

/*
========================
hfSessionLocalCallbacks::JoinFromConnectInfo
========================
*/
hfLobbyBackend * hfSessionLocalCallbacks::JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo , hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	return sessionLocal->JoinFromConnectInfo( connectInfo, lobbyType );
}

/*
========================
hfSessionLocalCallbacks::DestroyLobbyBackend
========================
*/
void hfSessionLocalCallbacks::DestroyLobbyBackend( hfLobbyBackend * lobbyBackend ) {
	sessionLocal->DestroyLobbyBackend( lobbyBackend );
}

