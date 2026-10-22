#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "Session_local.h"
#include "../sys/hfsys_session_savegames.h"
#include "../sys/hfsys_voicechat.h"


/*
========================
SteamAPIDebugTextHook
========================
*/
extern "C" void __cdecl SteamAPIDebugTextHook( int nSeverity, const char * pchDebugText ) {
	// if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
	// if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
	hfLib::Printf( "%s", pchDebugText );

	if ( nSeverity >= 1 ) {
		// place to set a breakpoint for catching API errors
		int x = 3;
		x;
	}
}

/*
===============================================================================

SESSION LOCAL
  
===============================================================================
*/

/*
===============
hfSessionLocal::hfSessionLocal
===============
*/
hfSessionLocal::hfSessionLocal() {

	localState = STATE_PRESS_START;
	
	InitBaseState();
}

/*
===============
hfSessionLocal::~hfSessionLocal
===============
*/
hfSessionLocal::~hfSessionLocal() {
	if ( sessionCallbacks != NULL ) {
		delete sessionCallbacks;
	}
}



/*
========================
hfSessionLocal::InitBaseState
========================
*/
void hfSessionLocal::InitBaseState() {
	

	localState						= STATE_PRESS_START;
	sessionOptions					= 0;
	currentID						= 0;

	sessionCallbacks				= new hfSessionLocalCallbacks( this );

	connectType						= CONNECT_NONE;

	isSysUIShowing					= false;

	pendingInviteDevice				= 0;
	pendingInviteMode				= PENDING_INVITE_NONE;


	flushedStats					= false;

	enumerationHandle				= 0;
}

/*
===============
hfSessionLocal::Shutdown
===============
*/
void hfSessionLocal::Shutdown() {

	delete signInManager;

	if ( achievementSystem != NULL ) {
		achievementSystem->Shutdown();
		delete achievementSystem;
	}

	DestroySteamObjects();
}

/*
===============
hfSessionLocal::Init

Called in an orderly fashion at system startup,
so commands, cvars, files, etc are all available
===============
*/
void hfSessionLocal::Init() {

	common->Printf( "-------- Initializing Session --------\n" );

	InitSteam();
	ConstructSteamObjects();

	signInManager = new hfSignInManagerWin();
	achievementSystem = new hfAchievementSystemWin();
	achievementSystem->Init();
	Initialize();

	common->Printf( "session initialized\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
========================
hfSessionLocal::InitSteam
========================
*/
void hfSessionLocal::InitSteam() {
	if ( steamInitialized || steamFailed ) {
		if ( steamFailed ) {
			net_usePlatformBackend.SetBool( false );		
		}
		return;
	}

	steamInitialized = SteamAPI_Init();
	steamFailed = !steamInitialized;

	if ( steamFailed ) {
		if ( net_usePlatformBackend.GetBool() ) {
			hfLib::Warning( "Steam failed to initialize.  Usually this happens because the Steam client isn't running." );
			// Turn off the usage of steam if it fails to initialize
			// FIXME: We'll want to bail (nicely) in the shipping product most likely
			net_usePlatformBackend.SetBool( false );		
		}
		return;
	}

	// from now on, all Steam API functions should return non-null interface pointers

	assert( SteamUtils() );
	SteamUtils()->SetWarningMessageHook( &SteamAPIDebugTextHook );

	ConstructSteamObjects();	
}

/*
========================
hfSessionLocal::ConstructSteamObjects
========================
*/
void hfSessionLocal::ConstructSteamObjects() {
}

/*
========================
hfSessionLocal::DestroySteamObjects
========================
*/
void hfSessionLocal::DestroySteamObjects() {
}

/*
========================
hfSessionLocal::MoveToPressStart
========================
*/
void hfSessionLocal::MoveToPressStart( gameDialogMessages_t msg ) {	
	if ( localState != STATE_PRESS_START ) {
		MoveToPressStart();

		common->Dialog().ClearDialogs();
		common->Dialog().AddDialog( msg, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );
	}
}

/*
========================
hfSessionLocal::MoveToPressStart
========================
*/
void hfSessionLocal::MoveToPressStart() {	
	if ( localState != STATE_PRESS_START ) {
		GetSignInManager().RemoveAllLocalUsers();
		SetState( STATE_PRESS_START );
	}
}

/*
========================
hfSessionLocal::SetState
========================
*/
void hfSessionLocal::SetState( hfSessionLocal::state_t newState ) {	
	assert( newState < NUM_STATES );
	assert( localState < NUM_STATES );

	if ( newState == localState ) {
		return;
	}

	localState = newState;
}

/*
========================
hfSessionLocal::GetState
========================
*/
hfSessionLocal::sessionState_t hfSessionLocal::GetState() const {
	// Convert our internal state to one of the external states
	switch ( localState ) {
		case STATE_PRESS_START:							return PRESS_START;
		case STATE_IDLE:								return IDLE;
		case STATE_PARTY_LOBBY_HOST:					return PARTY_LOBBY;
		case STATE_PARTY_LOBBY_PEER:					return PARTY_LOBBY;
		case STATE_GAME_LOBBY_HOST:						return GAME_LOBBY;
		case STATE_GAME_LOBBY_PEER:						return GAME_LOBBY;
		//case STATE_GAME_STATE_LOBBY_HOST:				return GAME_LOBBY;
		//case STATE_GAME_STATE_LOBBY_PEER:				return GAME_LOBBY;
		case STATE_LOADING:								return LOADING;
		case STATE_INGAME:								return INGAME;
		case STATE_CREATE_AND_MOVE_TO_PARTY_LOBBY:		return CONNECTING;
		case STATE_CREATE_AND_MOVE_TO_GAME_LOBBY:		return CONNECTING;
		//case STATE_CREATE_AND_MOVE_TO_GAME_STATE_LOBBY:	return CONNECTING;
		case STATE_FIND_OR_CREATE_MATCH:				return SEARCHING;
		case STATE_CONNECT_AND_MOVE_TO_PARTY:			return CONNECTING;
		case STATE_CONNECT_AND_MOVE_TO_GAME:			return CONNECTING;
		//case STATE_CONNECT_AND_MOVE_TO_GAME_STATE:		return CONNECTING;
		case STATE_BUSY:								return BUSY;
		default: {
			hfLib::Error( "GetState: Unknown state in hfSessionLocal" );
			return IDLE;
		}	
	};
}

/*
========================
hfSessionLocal::Pump
========================
*/
void hfSessionLocal::Pump() {

	GetSignInManager().Pump();

	hfLocalUser * masterUser = GetSignInManager().GetMasterLocalUser();

	if ( masterUser != NULL && localState == STATE_PRESS_START ) {
		// If we have a master user, and we are at press start, move to the menu area
		SetState( STATE_IDLE );


	}

	GetAchievementSystem().Pump();
}

/*
========================
hfSessionLocal::OnMasterLocalUserSignin
========================
*/
void hfSessionLocal::OnMasterLocalUserSignin() {
	enumerationHandle = EnumerateSaveGames( 0 );
}

/*
========================
hfSessionLocal::LoadGame
========================
*/
saveGameHandle_t hfSessionLocal::LoadGame( const char * name, const hfList< hfSaveFileEntry > & files ) {
	if ( processorLoadFiles.InitLoadFiles( name, files ) ) {
		return saveGameManager.ExecuteProcessor( &processorLoadFiles );
	} else {
		return 0;
	}
}

/*
========================
hfSessionLocal::SaveGame
========================
*/
saveGameHandle_t hfSessionLocal::SaveGame( const char * name, const hfList< hfSaveFileEntry > & files, const hfSaveGameDetails & description, uint64 skipErrorMask ) {
	saveGameHandle_t ret = 0;
	
	// serialize the description file behind their back...
	hfList< hfSaveFileEntry > filesWithDetails( files );
	hfFile_Memory * gameDetailsFile = new hfFile_Memory( SAVEGAME_DETAILS_FILENAME );
	//gameDetailsFile->MakeWritable();
	description.descriptors.WriteToIniFile( gameDetailsFile );
	filesWithDetails.Append( hfSaveFileEntry( gameDetailsFile, SAVEGAMEFILE_TEXT | SAVEGAMEFILE_AUTO_DELETE, SAVEGAME_DETAILS_FILENAME ) );

	if ( processorSave.InitSave( name, filesWithDetails, description ) ) {
		processorSave.SetSkipSystemErrorDialogMask( skipErrorMask );
		ret = GetSaveGameManager().ExecuteProcessor( &processorSave );
	}
	return ret;
}

/*
========================
hfSessionLocal::EnumerateSaveGames
========================
*/
saveGameHandle_t hfSessionLocal::EnumerateSaveGames( uint64 skipErrorMask ) {
	saveGameHandle_t ret = 0;
	
	// flush the old enumerated list
	GetSaveGameManager().GetEnumeratedSavegamesNonConst().Clear();

	if ( processorEnumerate.Init() ) {
		processorEnumerate.SetSkipSystemErrorDialogMask( skipErrorMask );
		ret = GetSaveGameManager().ExecuteProcessor( &processorEnumerate );
	}
	return ret;
}

/*
========================
hfSessionLocal::DeleteSaveGame
========================
*/
saveGameHandle_t hfSessionLocal::DeleteSaveGame( const char * name, uint64 skipErrorMask ) {
	saveGameHandle_t ret = 0;
	if ( processorDelete.InitDelete( name ) ) {
		processorDelete.SetSkipSystemErrorDialogMask( skipErrorMask );
		ret = GetSaveGameManager().ExecuteProcessor( &
			processorDelete );
	}
	return ret;
}

/*
========================
hfSessionLocal::IsEnumerating
========================
*/
bool hfSessionLocal::IsEnumerating() const {
	return !session->IsSaveGameCompletedFromHandle( processorEnumerate.GetHandle() );
}

/*
========================
hfSessionLocal::GetEnumerationHandle
========================
*/
saveGameHandle_t hfSessionLocal::GetEnumerationHandle() const {
	return processorEnumerate.GetHandle();
}

/*
========================
hfSessionLocal::CancelSaveGameWithHandle
========================
*/
void hfSessionLocal::CancelSaveGameWithHandle( const saveGameHandle_t & handle ) {
	GetSaveGameManager().CancelWithHandle( handle );
}


// FIXME: Move to sys_stats.cpp
leaderboardDefinition_t *	registeredLeaderboards[MAX_LEADERBOARDS];
int							numRegisteredLeaderboards = 0;

/*
========================
Sys_FindLeaderboardDef
========================
*/
const leaderboardDefinition_t * Sys_FindLeaderboardDef( int id ) {
	for ( int i = 0; i < numRegisteredLeaderboards; i++ ) {
		if ( registeredLeaderboards[i]->id == id ) {
			return registeredLeaderboards[i];
		}
	}
	
	return NULL;
}

/*
========================
hfSessionLocal::GetActiveLobby
========================
*/
hfLobby *	hfSessionLocal::GetActiveLobby() {
	sessionState_t state = GetState();

	if ( ( state == GAME_LOBBY ) || ( state == BUSY ) || ( state == INGAME ) || ( state == LOADING ) ) {
		return &GetGameLobby();
	} else if ( state == PARTY_LOBBY ) {
		return &GetPartyLobby();
	}
	
	return NULL;
}

/*
========================
hfSessionLocal::GetActiveLobby
========================
*/
const hfLobby * hfSessionLocal::GetActiveLobby() const {
	sessionState_t state = GetState();

	if ( ( state == GAME_LOBBY ) || ( state == BUSY ) || ( state == INGAME ) || ( state == LOADING ) ) {
		return &GetGameLobby();
	} else if ( state == PARTY_LOBBY ) {
		return &GetPartyLobby();
	}
	
	return NULL;
}

/*
========================
hfSessionLocal::GetActiveLobbyBase
This returns the base version for the hfSession version
========================
*/
hfLobbyBase & hfSessionLocal::GetActiveLobbyBase() {
	hfLobby * activeLobby = GetActiveLobby();

	if ( activeLobby != NULL ) {
		return *activeLobby;
	}

	return stubLobby;		// So we can return at least something
}

/*
========================
hfSessionLocal::PrePickNewHost
This is called when we have determined that we need to pick a new host.
Call PickNewHostInternal to continue on with the host picking process.
========================
*/
void hfSessionLocal::PrePickNewHost( hfLobby & lobby, bool forceMe, bool inviteOldHost ) {
	NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: (%s)\n", lobby.GetLobbyName() );

	if ( GetActiveLobby() == NULL ) {
		NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: GetActiveLobby() == NULL (%s)\n", lobby.GetLobbyName() );
		return;
	}

	// Check to see if we can migrate AT ALL
	// This is checking for coop, we should make this a specific option (MATCH_ALLOW_MIGRATION)
	if ( GetPartyLobby().parms.GetSessionMatchFlags() & MATCH_PARTY_INVITE_PLACEHOLDER ) {
		NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: MATCH_PARTY_INVITE_PLACEHOLDER (%s)\n", lobby.GetLobbyName() );

		// Can't migrate, shut both lobbies down, and create a new match using the original parms
		GetGameLobby().Shutdown();
		GetPartyLobby().Shutdown();
		
		// Throw up the appropriate dialog message so the player knows what happeend
		if ( localState >= STATE_LOADING ) {
			NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: localState >= hfSessionLocal::STATE_LOADING (%s)\n", lobby.GetLobbyName() );
			common->Dialog().AddDialog( GDM_BECAME_HOST_GAME_STATS_DROPPED, DIALOG_ACCEPT, NULL, NULL, false, __FUNCTION__, __LINE__, true );
		} else {
			NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: localState < hfSessionLocal::STATE_LOADING (%s)\n", lobby.GetLobbyName() );
			common->Dialog().AddDialog( GDM_LOBBY_BECAME_HOST_GAME, DIALOG_ACCEPT, NULL, NULL, false, __FUNCTION__, __LINE__, true  );
		}

		CreateMatch( GetActiveLobby()->parms );

		return;
	}

	// Check to see if the match is searchable
	if ( GetState() >= hfSession::GAME_LOBBY && MatchTypeIsSearchable( GetGameLobby().parms.GetSessionMatchFlags() ) ) {
		NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: MatchTypeIsSearchable (%s)\n", lobby.GetLobbyName() );
		// Searchable games migrate lobbies independently, and don't need to stay in sync
		lobby.PickNewHostInternal( forceMe, inviteOldHost );	
		return;
	}

	//
	// Beyond this point, game lobbies must be sync'd with party lobbies as far as host status
	// So to enforce that, we pull you out of the game lobby if you are in one when migration occurs
	//

	// Check to see if we should go back to a party lobby
	if ( GetBackState() >= hfSessionLocal::PARTY_LOBBY || GetState() == hfSession::PARTY_LOBBY ) {
		NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: GetBackState() >= hfSessionLocal::PARTY_LOBBY || GetState() == hfSession::PARTY_LOBBY (%s)\n", lobby.GetLobbyName() );
		// Force the party lobby to start picking a new host if we lost the game lobby host
		GetPartyLobby().PickNewHostInternal( forceMe, inviteOldHost );

		// End the game lobby, and go back to party lobby
		GetGameLobby().Shutdown();
		SetState( GetPartyLobby().IsHost() ? STATE_PARTY_LOBBY_HOST : STATE_PARTY_LOBBY_PEER );
	} else {
		NET_VERBOSE_PRINT("hfSessionLocal::PrePickNewHost: GetBackState() < hfSessionLocal::PARTY_LOBBY && GetState() != hfSession::PARTY_LOBBY (%s)\n", lobby.GetLobbyName() );
		if ( localState >= STATE_LOADING ) {
			common->Dialog().AddDialog( GDM_HOST_QUIT, DIALOG_ACCEPT, NULL, NULL, false, __FUNCTION__, __LINE__, true  );		// The host has quit the session. Returning to the main menu.
		}

		// Go back to main menu
		GetGameLobby().Shutdown();
		GetPartyLobby().Shutdown();
		SetState( STATE_IDLE );
	}
}
/*
========================
hfSessionLocal::PreMigrateInvite
This is called just before we get invited to a migrated session
If we return false, the invite will be ignored
========================
*/
bool hfSessionLocal::PreMigrateInvite( hfLobby & lobby )
{
	if ( GetActiveLobby() == NULL ) {
		return false;
	}

	// Check to see if we can migrate AT ALL
	// This is checking for coop, we should make this a specific option (MATCH_ALLOW_MIGRATION)
	if ( !verify( ( GetPartyLobby().parms.GetSessionMatchFlags() & MATCH_PARTY_INVITE_PLACEHOLDER ) == 0 ) ) {
		return false;	// Shouldn't get invites for coop (we should make this a specific option (MATCH_ALLOW_MIGRATION))
	}

	// Check to see if the match is searchable
	if ( MatchTypeIsSearchable( GetGameLobby().parms.GetSessionMatchFlags() ) ) {
		// Searchable games migrate lobbies independently, and don't need to stay in sync
		return true;	
	}

	//
	// Beyond this point, game lobbies must be sync'd with party lobbies as far as host status
	// So to enforce that, we pull you out of the game lobby if you are in one when migration occurs
	//

	if ( lobby.lobbyType != hfLobby::TYPE_PARTY ) {
		return false;		// We shouldn't be getting invites from non party lobbies when in a non searchable game
	}

	// Non placeholder Party lobbies can always migrate
	if ( GetBackState() >= hfSessionLocal::PARTY_LOBBY ) {
		// Non searchable games go back to the party lobby
		GetGameLobby().Shutdown();
		SetState( GetPartyLobby().IsHost() ? STATE_PARTY_LOBBY_HOST : STATE_PARTY_LOBBY_PEER );
	}

	return true;	// Non placeholder Party lobby invites joinable
}

/*
========================
hfSessionLocal::HandleDedicatedServerQueryRequest
========================
*/
void hfSessionLocal::HandleDedicatedServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType ) {
	NET_VERBOSE_PRINT( "HandleDedicatedServerQueryRequest from %s\n", remoteAddr.ToString() );
	
	bool canJoin = true;
	
	const unsigned long localChecksum = 0;
	const unsigned long remoteChecksum = msg.ReadLong();

	if ( remoteChecksum != localChecksum ) {
		NET_VERBOSE_PRINT( "HandleServerQueryRequest: Invalid version from %s\n", remoteAddr.ToString() );
		canJoin = false;
	}

	// Make sure we are the host of this party session
	if ( !GetPartyLobby().IsHost() ) {
		NET_VERBOSE_PRINT( "HandleServerQueryRequest: Not host of party\n" );
		canJoin = false;
	}

	// Make sure there is a session active
	if ( GetActiveLobby() == NULL ) {
		canJoin = false;
	}

	// Make sure we have enough free slots
	if ( GetPartyLobby().NumFreeSlots() == 0 || GetGameLobby().NumFreeSlots() == 0 ) {
		NET_VERBOSE_PRINT( "No free slots\n" );
		canJoin = false;
	}
	
	if ( MatchTypeInviteOnly( GetPartyLobby().parms.GetSessionMatchFlags() ) ) {
		canJoin = false;
	} 

	// Buffer to hold reply msg
	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE - 2 ];
	hfBitMsg retmsg;
	retmsg.InitWrite( buffer, sizeof( buffer ) );
	
	hfLocalUser * masterUser = GetSignInManager().GetMasterLocalUser();

	if ( masterUser == NULL ) {
		canJoin = false;
	}
	
	// Send the info about this game session to the caller
	retmsg.WriteBool( canJoin );
	
	if ( canJoin ) {
		retmsg.WriteBool( session->GetState() >= hfSession::LOADING );

		retmsg.WriteString( masterUser->GetGamerTag() );
		retmsg.WriteLong( GetActiveLobby()->parms.GetGameType() );			// We need to write out the game type whether we are in a game or not
		
		if ( GetGameLobby().IsSessionActive() ) {
			retmsg.WriteLong( GetGameLobby().parms.GetGameMap() );
			retmsg.WriteLong( GetGameLobby().parms.GetGameMode() );
		} else {
			retmsg.WriteLong( -1 );
			retmsg.WriteLong( -1 );
		}

		retmsg.WriteLong( GetActiveLobby()->GetNumLobbyUsers() );
		retmsg.WriteLong( GetActiveLobby()->parms.GetNumSlots() );
		for ( int i = 0; i < GetActiveLobby()->GetNumLobbyUsers(); i++ ) {
			retmsg.WriteString( GetActiveLobby()->GetLobbyUserName( i ) );
		}
	}

	// Send it
	GetPartyLobby().SendConnectionLess( remoteAddr, hfLobby::OOB_MATCH_QUERY_ACK, retmsg.GetWriteData(), retmsg.GetSize() );		
}

/*
========================
hfSessionLocal::HandleDedicatedServerQueryAck
========================
*/
void hfSessionLocal::HandleDedicatedServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg ) {
	NET_VERBOSE_PRINT( "HandleDedicatedServerQueryAck from %s\n", remoteAddr.ToString() );
}

/*
========================
hfSessionLocal::StartSessions
========================
*/
void hfSessionLocal::StartSessions() {
	if ( GetPartyLobby().lobbyBackend != NULL ) {
		GetPartyLobby().lobbyBackend->StartSession();
	}

	if ( GetGameLobby().lobbyBackend != NULL ) {
		GetGameLobby().lobbyBackend->StartSession();
	}
	
	SetLobbiesAreJoinable( false );
}

/*
========================
hfSessionLocal::EndSessions
========================
*/
void hfSessionLocal::EndSessions() {
	if ( GetPartyLobby().lobbyBackend != NULL ) {
		GetPartyLobby().lobbyBackend->EndSession();
	}

	if ( GetGameLobby().lobbyBackend != NULL ) {
		GetGameLobby().lobbyBackend->EndSession();
	}

	SetLobbiesAreJoinable( true );
}

/*
========================
hfSessionLocal::SetLobbiesAreJoinable
========================
*/
void hfSessionLocal::SetLobbiesAreJoinable( bool joinable ) {
	// NOTE - We don't manipulate the joinable state when we are supporting join in progress
	// Lobbies will naturally be non searchable when there are no free slots
	if ( GetPartyLobby().lobbyBackend != NULL && !MatchTypeIsJoinInProgress( GetPartyLobby().parms.GetSessionMatchFlags() ) ) {
		NET_VERBOSE_PRINT( "Party lobbyBackend SetIsJoinable: %d\n", joinable );
		GetPartyLobby().lobbyBackend->SetIsJoinable( joinable );
	}
	
	if ( GetGameLobby().lobbyBackend != NULL && !MatchTypeIsJoinInProgress( GetGameLobby().parms.GetSessionMatchFlags() ) ) {
		GetGameLobby().lobbyBackend->SetIsJoinable( joinable );
		NET_VERBOSE_PRINT( "Game lobbyBackend SetIsJoinable: %d\n", joinable );

	}
}

/*
========================
hfSessionLocal::EndMatchForMigration
========================
*/
void hfSessionLocal::EndMatchForMigration() {
	ClearVoiceGroups();	
}

/*
========================
hfSessionLocal::ClearVoiceGroups
========================
*/
void hfSessionLocal::ClearVoiceGroups() {
	/*
	for ( int i = 0; i < GetGameLobby().GetNumLobbyUsers(); ++i ) {
		SetGameSessionUserChatGroup( i, 0 );
	}
	SetActiveChatGroup( 0 );
	*/
}

/*
========================
hfSessionLocal::GoodbyeFromHost
========================
*/
void hfSessionLocal::GoodbyeFromHost( hfLobby & lobby, int peerNum, const lobbyAddress_t & remoteAddress, int msgType ) {
	if ( !verify( localState > STATE_IDLE ) ) {
		hfLib::Printf( "NET: Got disconnected from host %s on session %s when we were not in a lobby or game.\n", remoteAddress.ToString(), lobby.GetLobbyName() );
		MoveToMainMenu();
		return;		// Ignore if we are not past the main menu
	}

	// Goodbye from host.  See if we were connecting vs connected
	if ( ( localState == STATE_CONNECT_AND_MOVE_TO_PARTY || localState == STATE_CONNECT_AND_MOVE_TO_GAME ) && lobby.peers[peerNum].GetConnectionState() == hfLobby::CONNECTION_CONNECTING ) {
		// We were denied a connection attempt
		hfLib::Printf( "NET: Denied connection attempt from host %s on session %s. MsgType %i.\n", remoteAddress.ToString(), lobby.GetLobbyName(), msgType );
		// This will try to move to the next connection if one exists, otherwise will create a match
		HandleConnectionFailed( lobby, msgType == hfLobby::OOB_GOODBYE_FULL );
	} else {
		// We were disconnected from a server we were previously connected to
		hfLib::Printf( "NET: Disconnected from host %s on session %s. MsgType %i.\n", remoteAddress.ToString(), lobby.GetLobbyName(), msgType );

		const bool leaveGameWithParty = ( msgType == hfLobby::OOB_GOODBYE_W_PARTY );

		if ( leaveGameWithParty && lobby.lobbyType == hfLobby::TYPE_GAME && lobby.IsPeer() && GetState() == hfSession::GAME_LOBBY && GetPartyLobby().host >= 0 &&
				lobby.peers[peerNum].address.Compare( GetPartyLobby().peers[GetPartyLobby().host].address, true ) ) {
			// If a host is telling us goodbye from a game lobby, and the game host is the same as our party host, 
			// and we aren't in a game, and the host wants us to leave with him, then do so now
			GetGameLobby().Shutdown();
			SetState( STATE_PARTY_LOBBY_PEER );
		} else {
			// Host left, so pick a new host (possibly even us) for this lobby
			lobby.PickNewHost();
		}
	}
}

/*
========================
hfSessionLocal::HandlePackets
========================
*/
bool hfSessionLocal::HandlePackets() {
	byte				packetBuffer[ hfPacketProcessor::MAX_FINAL_PACKET_SIZE ];
	lobbyAddress_t	remoteAddress;
	int					recvSize = 0;

	while ( ReadRawPacket( remoteAddress, packetBuffer, recvSize, sizeof( packetBuffer ) ) && recvSize > 0 ) {
		// fragMsg will hold the raw packet
		hfBitMsg fragMsg;
		fragMsg.InitRead( packetBuffer, recvSize );

		// Peek at the session ID
		hfPacketProcessor::sessionId_t sessionID = hfPacketProcessor::GetSessionID( fragMsg );

		// hfLib::Printf( "NET: HandlePackets - session %d, size %d \n", sessionID, recvSize );

		// Make sure it's valid
		if ( sessionID == hfPacketProcessor::SESSION_ID_INVALID ) {
			hfLib::Printf( "NET: Invalid sessionID %s.\n", remoteAddress.ToString() );
			continue;
		}

		// Distribute the packet to the proper lobby
		if ( sessionID & 1 ) {
			GetGameLobby().HandlePacket( remoteAddress, fragMsg, sessionID );
		} else {
			GetPartyLobby().HandlePacket( remoteAddress, fragMsg, sessionID );
		}
	}

	return false;
}

hfCVar net_connectTimeoutInSeconds( "net_connectTimeoutInSeconds", "15", CVAR_INTEGER, "timeout (in seconds) while connecting" );
hfCVar net_testPartyMemberConnectFail( "net_testPartyMemberConnectFail", "-1", CVAR_INTEGER, "Force this party member index to fail to connect to games." );

/*
========================
hfSessionLocal::HandleConnectAndMoveToLobby
Called from State_Connect_And_Move_To_Party/State_Connect_And_Move_To_Game
========================
*/
bool hfSessionLocal::HandleConnectAndMoveToLobby( hfLobby & lobby ) {	
	assert( localState == STATE_CONNECT_AND_MOVE_TO_PARTY || localState == STATE_CONNECT_AND_MOVE_TO_GAME );
	assert( connectType == CONNECT_FIND_OR_CREATE || connectType == CONNECT_DIRECT );

	if ( lobby.GetState() == hfLobby::STATE_FAILED ) {
		// If we get here, we were trying to connect to a lobby (from state State_Connect_And_Move_To_Party/State_Connect_And_Move_To_Game)
		HandleConnectionFailed( lobby, false );
		return true;
	}

	if ( lobby.GetState() != hfLobby::STATE_IDLE ) {
		return HandlePackets();	// Valid but busy
	}

	assert( !GetPartyLobby().waitForPartyOk );

	//
	// Past this point, we've connected to the lobby
	//

	// If we are connecting to a game lobby, see if we need to keep waiting as either a host or peer while we're confirming all party members made it
	if ( lobby.lobbyType == hfLobby::TYPE_GAME ) {
		if ( GetPartyLobby().IsHost() ) {
			// As a host, wait until all party members make it
			assert( !GetGameLobby().waitForPartyOk );

			const int timeoutMs = net_connectTimeoutInSeconds.GetInteger() * 1000;

			if ( timeoutMs != 0 && Sys_Milliseconds() - lobby.helloStartTime > timeoutMs ) {
				// Took too long, move to next result, or create a game instead
				HandleConnectionFailed( lobby, false );
				return true;
			}

			int numUsersIn = 0;

			for ( int i = 0; i < GetPartyLobby().GetNumLobbyUsers(); i++ ) {

				if ( net_testPartyMemberConnectFail.GetInteger() == i ) {
					continue;
				}

				bool foundUser = false;

				lobbyUser_t * partyUser = GetPartyLobby().GetLobbyUser( i );

				for ( int j = 0; j < GetGameLobby().GetNumLobbyUsers(); j++ ) {
					lobbyUser_t * gameUser = GetGameLobby().GetLobbyUser( j );

					if ( GetGameLobby().IsSessionUserLocal( gameUser ) || gameUser->address.Compare( partyUser->address, true ) ) {
						numUsersIn++;
						foundUser = true;
						break;
					}
				}
	
				assert( !GetPartyLobby().IsSessionUserIndexLocal( i ) || foundUser );
			}

			if ( numUsersIn != GetPartyLobby().GetNumLobbyUsers() ) {
				return HandlePackets();		// All users not in, keep waiting until all user make it, or we time out
			} 
		
			NET_VERBOSE_PRINT( "NET: All party members made it into the game lobby.\n" );
			
			// Let all the party members know everyone made it, and it's ok to stay at this server
			for ( int i = 0; i < GetPartyLobby().peers.Num(); i++ ) {
				if ( GetPartyLobby().peers[ i ].IsConnected() ) {
					GetPartyLobby().QueueReliableMessage( i, hfLobby::RELIABLE_PARTY_CONNECT_OK );
				}
			}
		} else {
			if ( !verify ( lobby.host != -1 ) ) {
				MoveToMainMenu();
				connectType = CONNECT_NONE;
				return false;
			}

			// As a peer, wait for server to tell us everyone made it
			if ( GetGameLobby().waitForPartyOk ) {
				const int timeoutMs =  net_connectTimeoutInSeconds.GetInteger()  * 1000;

				if ( timeoutMs != 0 && Sys_Milliseconds() - lobby.helloStartTime > timeoutMs ) {
					GetGameLobby().waitForPartyOk = false;		// Just connect to this game lobby if we haven't heard from the party host for the entire timeout duration
				}
			}

			if ( GetGameLobby().waitForPartyOk ) {
				return HandlePackets();			// Waiting on party host to tell us everyone made it
			}
		}
	}

	// Success
	SetState( lobby.lobbyType == hfLobby::TYPE_PARTY ? STATE_PARTY_LOBBY_PEER : STATE_GAME_LOBBY_PEER );
	connectType = CONNECT_NONE;

	return false;
}

/*
========================
hfSessionLocal::MoveToMainMenu
========================
*/
void hfSessionLocal::MoveToMainMenu() {
	GetPartyLobby().Shutdown();
	GetGameLobby().Shutdown();
	SetState( STATE_IDLE );
}


/*
========================
hfSessionLocal::HandleConnectionFailed
Called anytime a connection fails, and does the right thing.
========================
*/
void hfSessionLocal::HandleConnectionFailed( hfLobby & lobby, bool wasFull ) {
	assert( localState == STATE_CONNECT_AND_MOVE_TO_PARTY || localState == STATE_CONNECT_AND_MOVE_TO_GAME );
	assert( connectType == CONNECT_FIND_OR_CREATE || connectType == CONNECT_DIRECT );
	bool canPlayOnline = true;
	
	// Check for online status (this is only a problem on the PS3 at the moment. The 360 LIVE system handles this for us
	if ( GetSignInManager().GetMasterLocalUser() != NULL ) {
		canPlayOnline = GetSignInManager().GetMasterLocalUser()->CanPlayOnline();
	}
	
	if ( connectType == CONNECT_FIND_OR_CREATE ) {
		// Clear the "Lobby was Full" dialog in case it's up
		// We only want to see this msg when doing a direct connect (CONNECT_DIRECT)
		common->Dialog().ClearDialog( GDM_LOBBY_FULL );

		assert( localState == STATE_CONNECT_AND_MOVE_TO_GAME );
		assert( lobby.lobbyType == hfLobby::TYPE_GAME );
		if ( !lobby.ConnectToNextSearchResult() ) {
			CreateMatch( GetGameLobby().parms );		// Assume any time we are connecting to a game lobby, it is from a FindOrCreateMatch call, so create a match
		}
	} else if ( connectType == CONNECT_DIRECT ) {
		if ( localState == STATE_CONNECT_AND_MOVE_TO_GAME && GetPartyLobby().IsPeer() ) {

			int flags = GetPartyLobby().parms.GetSessionMatchFlags();

			if ( MatchTypeIsOnline( flags ) && ( flags & MATCH_REQUIRE_PARTY_LOBBY ) && ( ( flags & MATCH_PARTY_INVITE_PLACEHOLDER ) == 0 ) ) {
				// We get here when our party host told us to connect to a game, but the game didn't exist.  
				// Just drop back to the party lobby and wait for further orders.
				SetState( STATE_PARTY_LOBBY_PEER );
				return;
			}
		}

		if ( wasFull ) {
			common->Dialog().AddDialog( GDM_LOBBY_FULL, DIALOG_ACCEPT, NULL, NULL, false );
		} else if ( !canPlayOnline ) {
			common->Dialog().AddDialog( GDM_PLAY_ONLINE_NO_PROFILE, DIALOG_ACCEPT, NULL, NULL, false );
		} else {
			// TEMP HACK: We detect the steam lobby is full in hfLobbyBackendWin, and then STATE_FAILED, which brings us here. Need to find a way to notify
			// session local that the game was full so we don't do this check here
			// eeubanks: Pollard, how do you think we should handle this?
			if ( !common->Dialog().HasDialogMsg( GDM_LOBBY_FULL, NULL ) ) {
			common->Dialog().AddDialog( GDM_INVALID_INVITE, DIALOG_ACCEPT, NULL, NULL, false );
		}
		}
		MoveToMainMenu();
	} else {
		// Shouldn't be possible, but just in case
		MoveToMainMenu();
	}
}

/*
========================
hfSessionLocal::SendRawPacket
========================
*/
void hfSessionLocal::SendRawPacket( const lobbyAddress_t & to, const void * data, int size ) {
	GetPort().SendRawPacket( to, data, size );
}

/*
========================
hfSessionLocal::ReadRawPacket
========================
*/
bool hfSessionLocal::ReadRawPacket( lobbyAddress_t & from, void * data, int & size, int maxSize ) {
	return GetPort().ReadRawPacket( from, data, size, maxSize );
}

/*
========================
hfSessionLocal::ConnectAndMoveToLobby
========================
*/
void hfSessionLocal::ConnectAndMoveToLobby( hfLobby & lobby, const lobbyConnectInfo_t & connectInfo, bool fromInvite ) {

	// Since we are connecting directly to a lobby, make sure no search results are left over from previous FindOrCreateMatch results
	// If we don't do this, we might think we should attempt to connect to an old search result, and we don't want to in this case
	lobby.searchResults.Clear();

	// Attempt to connect to the lobby
	lobby.ConnectTo( connectInfo, fromInvite );

	connectType = CONNECT_DIRECT;

	// Wait for connection
	SetState( lobby.lobbyType == hfLobby::TYPE_PARTY ? STATE_CONNECT_AND_MOVE_TO_PARTY : STATE_CONNECT_AND_MOVE_TO_GAME );
}

/*
========================
hfSessionLocal::WriteLeaderboardToMsg
========================
*/
void hfSessionLocal::WriteLeaderboardToMsg( hfBitMsg & msg, const leaderboardDefinition_t * leaderboard, const column_t * stats ) {
	assert( Sys_FindLeaderboardDef( leaderboard->id ) == leaderboard );
	
	msg.WriteLong( leaderboard->id );

	for ( int i = 0; i < leaderboard->numColumns; ++i ) {
		uint64 value = stats[i].value;

		//hfLib::Printf( "value = %i\n", (int32)value );

		for ( int j = 0; j < leaderboard->columnDefs[i].bits; j++ ) {
			msg.WriteBits( value & 1, 1 );
			value >>= 1;
		}
		//msg.WriteData( &stats[i].value, sizeof( stats[i].value ) );
	}
}

/*
========================
hfSessionLocal::ReadLeaderboardFromMsg
========================
*/
const leaderboardDefinition_t * hfSessionLocal::ReadLeaderboardFromMsg( hfBitMsg & msg, column_t * stats ) {
	int id = msg.ReadLong();
	
	const leaderboardDefinition_t * leaderboard = Sys_FindLeaderboardDef( id );
	
	if ( leaderboard == NULL ) {
		hfLib::Printf( "NET: Invalid leaderboard id: %i\n", id );
		return NULL;
	}
	
	for ( int i = 0; i < leaderboard->numColumns; ++i ) {
		uint64 value = 0;

		for ( int j = 0; j < leaderboard->columnDefs[i].bits; j++ ) {
			value |= (uint64)( msg.ReadBits( 1 ) & 1 ) << j;
		}

		stats[i].value = value;

		//hfLib::Printf( "value = %i\n", (int32)value );
		//msg.ReadData( &stats[i].value, sizeof( stats[i].value ) );
	}
	
	return leaderboard;
}

/*
========================
hfSessionLocal::EndMatchInternal
========================
*/
void hfSessionLocal::EndMatchInternal( bool premature/*=false*/ ) {
	ClearVoiceGroups();


	for ( int p = 0; p < GetGameLobby().peers.Num(); p++ ) {
		// If we are the host, increment the session ID.  The client will use a rolling check to catch it
		if ( GetGameLobby().IsHost() ) {
			if ( GetGameLobby().peers[p].IsConnected() ) {
				if ( GetGameLobby().peers[p].packetProc != NULL ) {
					GetGameLobby().peers[p].packetProc->VerifyEmptyReliableQueue( hfLobby::RELIABLE_GAME_DATA, hfLobby::RELIABLE_DUMMY_MSG );
				}
				GetGameLobby().peers[p].sessionID = GetGameLobby().IncrementSessionID( GetGameLobby().peers[p].sessionID );
			}
		}
		GetGameLobby().peers[p].ResetMatchData();
	}

	
	GetGameLobby().loaded = false;
	//gameLobbyWasCoalesced	= false;		// Reset this back to false.  We use this so the lobby code doesn't randomly choose a map when we coalesce
	
	ClearMigrationState();

	if ( common->IsMultiplayer() ) {
		if ( GetGameLobby().IsSessionActive() ) {
			// All peers need to remove disconnected users to stay in sync
			GetGameLobby().CompactDisconnectedUsers();

			// Go back to the game lobby
			if ( GetGameLobby().IsHost() ) {
				SetState( STATE_GAME_LOBBY_HOST );
			} else {
				SetState( STATE_GAME_LOBBY_PEER );
			}
		} else {
			// Oops, no game lobby?
			assert( false ); // how is this possible?
			MoveToMainMenu();
		}
	} else {
		SetState( STATE_IDLE );
	}

	if ( GetGameLobby().IsHost() ) {
		// Send a reliable msg to all peers to also "EndMatch"
		for ( int p = 0; p < GetGameLobby().peers.Num(); p++ ) {
			GetGameLobby().QueueReliableMessage( p, premature ? hfLobby::RELIABLE_ENDMATCH_PREMATURE : hfLobby::RELIABLE_ENDMATCH );
		}
	} else if( premature ) {
		// Notify client that host left early and thats why we are back in the lobby
		bool stats = MatchTypeHasStats( GetGameLobby().GetMatchParms().GetSessionMatchFlags() ) && ( GetFlushedStats() == false );
		common->Dialog().AddDialog( stats ? GDM_HOST_RETURNED_TO_LOBBY_STATS_DROPPED : GDM_HOST_RETURNED_TO_LOBBY, DIALOG_ACCEPT, NULL, NULL, false, __FUNCTION__, __LINE__, true );
	}
}

/*
========================
hfSessionLocal::HandleOobVoiceAudio
========================
*/
void hfSessionLocal::HandleOobVoiceAudio( const lobbyAddress_t & from, const hfBitMsg & msg ) {

	hfLobby * activeLobby = GetActiveLobby();
	
	if ( activeLobby == NULL ) {
		return;
	}

	voiceChat->SetActiveLobby( activeLobby->lobbyType );

	voiceChat->SubmitIncomingChatData( msg.GetReadData() + msg.GetReadCount(), msg.GetSize() - msg.GetReadCount() );
}

/*
========================
hfSessionLocal::ClearMigrationState
========================
*/
void hfSessionLocal::ClearMigrationState() {
	// We are ending the match without migration, so clear that state
	GetPartyLobby().ResetAllMigrationState();
	GetGameLobby().ResetAllMigrationState();
}


/*
========================
hfSessionLocal::SendLeaderboardStatsToPlayer
========================
*/
void hfSessionLocal::SendLeaderboardStatsToPlayer( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, const column_t * stats ) {

	if ( GetGameLobby().IsLobbyUserDisconnected( sessionUserIndex ) ) {
		hfLib::Warning( "Tried to tell disconnected user to report stats" );
		return;
	}
	
	const int peerIndex = GetGameLobby().PeerIndexFromLobbyUserIndex( sessionUserIndex );
	
	if ( peerIndex == -1 ) {
		hfLib::Warning( "Tried to tell invalid peer index to report stats" );
		return;
	}

	if ( !verify( GetGameLobby().IsHost() ) || 
		!verify( peerIndex < GetGameLobby().peers.Num() ) || 
		!verify( GetGameLobby().peers[ peerIndex ].IsConnected() ) ) {
		hfLib::Warning( "Tried to tell invalid peer to report stats" );
		return;
	}

	NET_VERBOSE_PRINT( "Telling sessionUserIndex %i (peer %i) to report stats\n", sessionUserIndex, peerIndex );

	lobbyUser_t * gameUser = GetGameLobby().GetLobbyUser( sessionUserIndex );

	if ( !verify( gameUser != NULL ) ) {
		return;
	}

	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE ];
	hfBitMsg msg;
	msg.InitWrite( buffer, sizeof( buffer ) );

	// Use the user ID
	msg.WriteLong( gameUser->userID );

	WriteLeaderboardToMsg( msg, leaderboard, stats );
	
	GetGameLobby().QueueReliableMessage( peerIndex, hfLobby::RELIABLE_POST_STATS, msg.GetWriteData(), msg.GetSize() );
}

/*
========================
hfSessionLocal::RecvLeaderboardStatsForPlayer
========================
*/
void hfSessionLocal::RecvLeaderboardStatsForPlayer( hfBitMsg & msg ) {
	column_t stats[ MAX_LEADERBOARD_COLUMNS ];

	int userID = msg.ReadLong();

	int sessionUserIndex = GetGameLobby().FindSessionUserByUserId( userID );

	const leaderboardDefinition_t * leaderboard = ReadLeaderboardFromMsg( msg, stats );
	
	if ( leaderboard == NULL ) {
		hfLib::Printf( "RecvLeaderboardStatsForPlayer: Invalid lb.\n" );
		return;
	}

	LeaderboardUpload( sessionUserIndex, leaderboard, stats );
}
