/*
================================================================================================

Contains the windows implementation of the network session

================================================================================================
*/

#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "../../framework/hfCommon_local.h"
#include "../hfsys_session_local.h"
#include "../hfsys_stats.h"
#include "../hfsys_savegame.h"
#include "../hfsys_lobby_backend_direct.h"
#include "../hfsys_voicechat.h"
#include "hfwin_achievements.h"
#include "hfwin_local.h"

/*
========================
Global variables
========================
*/

extern hfCVar net_port;


/*
========================
hfSessionLocalWin::hfSessionLocalWin
========================
*/
class hfSessionLocalWin : public hfSessionLocal {
friend class hfLobbyToSessionCBLocal;

public:
	hfSessionLocalWin();
	virtual ~hfSessionLocalWin();

	// hfSessionLocal interface
	virtual void		Initialize();
	virtual void		Shutdown();

	virtual void		InitializeSoundRelatedSystems();
	virtual void		ShutdownSoundRelatedSystems();

	virtual void		PlatformPump();

	virtual void		InviteFriends();
	virtual void		InviteParty();
	virtual void		ShowPartySessions();

	virtual void		ShowSystemMarketplaceUI() const;

	virtual void					ListServers( const hfCallback & callback );
	virtual void					CancelListServers();
	virtual int						NumServers() const;
	virtual const serverInfo_t *	ServerInfo( int i ) const;
	virtual void					ConnectToServer( int i );
	virtual void					ShowServerGamerCardUI( int i );

	virtual void			ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID );

	virtual void			ShowOnlineSignin() {}
	virtual void			UpdateRichPresence() {}
	virtual void			CheckVoicePrivileges() {}

	virtual bool			ProcessInputEvent( const sysEvent_t * ev );

	// System UI
	virtual bool			IsSystemUIShowing() const;
	virtual void			SetSystemUIShowing( bool show );

	// Invites
	virtual void			HandleBootableInvite( int64 lobbyId = 0 );
	virtual void			ClearBootableInvite();
	virtual void			ClearPendingInvite();
	
	virtual bool			HasPendingBootableInvite();
	virtual void			SetDiscSwapMPInvite( void * parm );
	virtual void *			GetDiscSwapMPInviteParms();

	virtual void			EnumerateDownloadableContent();

	virtual void 			HandleServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType );
	virtual void 			HandleServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg );

	// Leaderboards
	virtual void			LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t * leaderboard, const column_t * stats, const hfFile_Memory * attachment = NULL );
	virtual void			LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int startingRank, int numRows, const hfLeaderboardCallback & callback );
	virtual void			LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int64 attachmentID );

	// Scoring (currently just for TrueSkill)
	virtual void			SetLobbyUserRelativeScore( lobbyUserID_t lobbyUserID, int relativeScore, int team ) {}

	virtual void			LeaderboardFlush();

	virtual hfNetSessionPort &	GetPort( bool dedicated = false );
	virtual hfLobbyBackend *	CreateLobbyBackend( const hfMatchParameters & p, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType );
	virtual hfLobbyBackend *	FindLobbyBackend( const hfMatchParameters & p, int numPartyUsers, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType );
	virtual hfLobbyBackend *	JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo , hfLobbyBackend::lobbyBackendType_t lobbyType );
	virtual void				DestroyLobbyBackend( hfLobbyBackend * lobbyBackend );
	virtual void				PumpLobbies();
	virtual void				JoinAfterSwap( void * joinID );

	virtual bool				GetLobbyAddressFromNetAddress( const netadr_t & netAddr, lobbyAddress_t & outAddr ) const;
	virtual bool				GetNetAddressFromLobbyAddress( const lobbyAddress_t & lobbyAddress, netadr_t & outNetAddr ) const;

public:
	void	Connect_f( const hfCmdArgs &args );

private:
	void					EnsurePort();

	hfLobbyBackend *		CreateLobbyInternal( hfLobbyBackend::lobbyBackendType_t lobbyType );

	hfArray< hfLobbyBackend *, 3 > lobbyBackends;

	hfNetSessionPort		port;
	bool					canJoinLocalHost;

	hfLobbyToSessionCBLocal	* lobbyToSessionCB;
};

hfSessionLocalWin sessionLocalWin;
hfSession * session = &sessionLocalWin;

/*
========================
hfLobbyToSessionCBLocal
========================
*/
class hfLobbyToSessionCBLocal : public hfLobbyToSessionCB {
public:
	hfLobbyToSessionCBLocal( hfSessionLocalWin * sessionLocalWin_ ) : sessionLocalWin( sessionLocalWin_ ) { }

	virtual bool CanJoinLocalHost() const { sessionLocalWin->EnsurePort(); return sessionLocalWin->canJoinLocalHost; }
	virtual class hfLobbyBackend * GetLobbyBackend( hfLobbyBackend::lobbyBackendType_t type ) const { return sessionLocalWin->lobbyBackends[ type ]; }

private:
	hfSessionLocalWin *			sessionLocalWin;
};

hfLobbyToSessionCBLocal lobbyToSessionCBLocal( &sessionLocalWin );
hfLobbyToSessionCB * lobbyToSessionCB = &lobbyToSessionCBLocal;

class hfVoiceChatMgrWin : public hfVoiceChatMgr {
public:
	virtual bool	GetLocalChatDataInternal( int talkerIndex, byte * data, int & dataSize ) { return false; }
	virtual void	SubmitIncomingChatDataInternal( int talkerIndex, const byte * data, int dataSize ) { }
	virtual bool	TalkerHasData( int talkerIndex ) { return false; }
	virtual bool	RegisterTalkerInternal( int index ) { return true; }
	virtual void	UnregisterTalkerInternal( int index ) { }
};

/*
========================
hfSessionLocalWin::hfSessionLocalWin
========================
*/
hfSessionLocalWin::hfSessionLocalWin() {
	signInManager		= new (TAG_SYSTEM) hfSignInManagerWin;
	saveGameManager		= new (TAG_SAVEGAMES) hfSaveGameManager();
	voiceChat			= new (TAG_SYSTEM) hfVoiceChatMgrWin();
	lobbyToSessionCB	= new (TAG_SYSTEM) hfLobbyToSessionCBLocal( this );

	canJoinLocalHost	= false;

	lobbyBackends.Zero();
}

/*
========================
hfSessionLocalWin::hfSessionLocalWin
========================
*/
hfSessionLocalWin::~hfSessionLocalWin() {
	delete voiceChat;
	delete lobbyToSessionCB;
}

/*
========================
hfSessionLocalWin::Initialize
========================
*/
void hfSessionLocalWin::Initialize() {
	hfSessionLocal::Initialize();

	// The shipping path doesn't load title storage
	// Instead, we inject values through code which is protected through steam DRM
	titleStorageVars.Set( "MAX_PLAYERS_ALLOWED", "8" );
	titleStorageLoaded = true;

	// First-time check for downloadable content once game is launched
	EnumerateDownloadableContent();

	GetPartyLobby().Initialize( hfLobby::TYPE_PARTY, sessionCallbacks );
	GetGameLobby().Initialize( hfLobby::TYPE_GAME, sessionCallbacks );
	GetGameStateLobby().Initialize( hfLobby::TYPE_GAME_STATE, sessionCallbacks );

	achievementSystem = new (TAG_SYSTEM) hfAchievementSystemWin();
	achievementSystem->Init();
}

/*
========================
hfSessionLocalWin::Shutdown
========================
*/
void hfSessionLocalWin::Shutdown() {
	NET_VERBOSE_PRINT( "NET: Shutdown\n" );
	hfSessionLocal::Shutdown();

	MoveToMainMenu();
	
	// Wait until we fully shutdown
	while ( localState != STATE_IDLE && localState != STATE_PRESS_START ) {
		Pump();
	}

	if ( achievementSystem != NULL ) {
		achievementSystem->Shutdown();
		delete achievementSystem;
		achievementSystem = NULL;
	}
}

/*
========================
hfSessionLocalWin::InitializeSoundRelatedSystems
========================
*/
void hfSessionLocalWin::InitializeSoundRelatedSystems() {
	if ( voiceChat != NULL ) {
		voiceChat->Init( NULL );
	}
}

/*
========================
hfSessionLocalWin::ShutdownSoundRelatedSystems
========================
*/
void hfSessionLocalWin::ShutdownSoundRelatedSystems() {
	if ( voiceChat != NULL ) {
		voiceChat->Shutdown();
	}
}

/*
========================
hfSessionLocalWin::PlatformPump
========================
*/
void hfSessionLocalWin::PlatformPump() {
}

/*
========================
hfSessionLocalWin::InviteFriends
========================
*/
void hfSessionLocalWin::InviteFriends() {
}

/*
========================
hfSessionLocalWin::InviteParty
========================
*/
void hfSessionLocalWin::InviteParty() {
}

/*
========================
hfSessionLocalWin::ShowPartySessions
========================
*/
void hfSessionLocalWin::ShowPartySessions() {
}

/*
========================
hfSessionLocalWin::ShowSystemMarketplaceUI
========================
*/
void hfSessionLocalWin::ShowSystemMarketplaceUI() const {
}

/*
========================
hfSessionLocalWin::ListServers
========================
*/
void hfSessionLocalWin::ListServers( const hfCallback & callback ) { 
	ListServersCommon();
}

/*
========================
hfSessionLocalWin::CancelListServers
========================
*/
void hfSessionLocalWin::CancelListServers() {
}

/*
========================
hfSessionLocalWin::NumServers
========================
*/
int hfSessionLocalWin::NumServers() const {
	return 0;
}

/*
========================
hfSessionLocalWin::ServerInfo
========================
*/
const serverInfo_t * hfSessionLocalWin::ServerInfo( int i ) const {
	return NULL;
}

/*
========================
hfSessionLocalWin::ConnectToServer
========================
*/
void hfSessionLocalWin::ConnectToServer( int i ) {
}

/*
========================
hfSessionLocalWin::Connect_f
========================
*/
void hfSessionLocalWin::Connect_f( const hfCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		hfLib::Printf( "Usage: Connect to IP.  Use with net_port. \n");
		return;
	}

	Cancel();

	if ( signInManager->GetMasterLocalUser() == NULL ) { 
		signInManager->RegisterLocalUser( 0 );
	}

	lobbyConnectInfo_t connectInfo;

	Sys_StringToNetAdr( args.Argv(1), &connectInfo.netAddr, true );
	connectInfo.netAddr.port = net_port.GetInteger();

	ConnectAndMoveToLobby( GetPartyLobby(), connectInfo, false );
}

/*
========================
void Connect_f
========================
*/
CONSOLE_COMMAND( connect, "Connect to the specified IP", NULL ) {
	sessionLocalWin.Connect_f( args );
}

/*
========================
hfSessionLocalWin::ShowServerGamerCardUI
========================
*/
void hfSessionLocalWin::ShowServerGamerCardUI( int i ) {
}

/*
========================
hfSessionLocalWin::ShowLobbyUserGamerCardUI(
========================
*/
void hfSessionLocalWin::ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID ) {
}

/*
========================
hfSessionLocalWin::ProcessInputEvent
========================
*/
bool hfSessionLocalWin::ProcessInputEvent( const sysEvent_t * ev ) {
	if ( GetSignInManager().ProcessInputEvent( ev ) ) {
		return true;
	}
	return false;
}

/*
========================
hfSessionLocalWin::IsSystemUIShowing
========================
*/
bool hfSessionLocalWin::IsSystemUIShowing() const {
	return !win32.activeApp || isSysUIShowing; // If the user alt+tabs away, treat it the same as bringing up the steam overlay
}

/*
========================
hfSessionLocalWin::SetSystemUIShowing
========================
*/
void hfSessionLocalWin::SetSystemUIShowing( bool show ) {
	isSysUIShowing = show;
}

/*
========================
hfSessionLocalWin::HandleServerQueryRequest
========================
*/
void hfSessionLocalWin::HandleServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType ) {
	NET_VERBOSE_PRINT( "HandleServerQueryRequest from %s\n", remoteAddr.ToString() );
}

/*
========================
hfSessionLocalWin::HandleServerQueryAck
========================
*/
void hfSessionLocalWin::HandleServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg ) {
	NET_VERBOSE_PRINT( "HandleServerQueryAck from %s\n", remoteAddr.ToString() );

}

/*
========================
hfSessionLocalWin::ClearBootableInvite
========================
*/
void hfSessionLocalWin::ClearBootableInvite() {
}

/*
========================
hfSessionLocalWin::ClearPendingInvite
========================
*/
void hfSessionLocalWin::ClearPendingInvite() {
}

/*
========================
hfSessionLocalWin::HandleBootableInvite
========================
*/
void hfSessionLocalWin::HandleBootableInvite( int64 lobbyId ) {
}

/*
========================
hfSessionLocalWin::HasPendingBootableInvite
========================
*/
bool hfSessionLocalWin::HasPendingBootableInvite() {
	return false;
}

/*
========================
hfSessionLocal::SetDiscSwapMPInvite
========================
*/
void hfSessionLocalWin::SetDiscSwapMPInvite( void * parm ) {
}

/*
========================
hfSessionLocal::GetDiscSwapMPInviteParms
========================
*/
void * hfSessionLocalWin::GetDiscSwapMPInviteParms() {
	return NULL;
}

/*
========================
hfSessionLocalWin::EnumerateDownloadableContent
========================
*/
void hfSessionLocalWin::EnumerateDownloadableContent() {
}

/*
========================
hfSessionLocalWin::LeaderboardUpload
========================
*/
void hfSessionLocalWin::LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t * leaderboard, const column_t * stats, const hfFile_Memory * attachment ) {
}

/*
========================
hfSessionLocalWin::LeaderboardFlush
========================
*/
void hfSessionLocalWin::LeaderboardFlush() {
}

/*
========================
hfSessionLocalWin::LeaderboardDownload
========================
*/
void hfSessionLocalWin::LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int startingRank, int numRows, const hfLeaderboardCallback & callback ) {
}

/*
========================
hfSessionLocalWin::LeaderboardDownloadAttachment
========================
*/
void hfSessionLocalWin::LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int64 attachmentID ) {
}

/*
========================
hfSessionLocalWin::EnsurePort
========================
*/
void hfSessionLocalWin::EnsurePort() {	
	// Init the port using reqular windows sockets
	if ( port.IsOpen() ) {
		return;		// Already initialized
	}

	if ( port.InitPort( net_port.GetInteger(), false ) ) {
		canJoinLocalHost = false;
	} else {
		// Assume this is another instantiation on the same machine, and just init using any available port
		port.InitPort( PORT_ANY, false );
		canJoinLocalHost = true;
	}
}

/*
========================
hfSessionLocalWin::GetPort
========================
*/
hfNetSessionPort & hfSessionLocalWin::GetPort( bool dedicated ) { 
	EnsurePort(); 
	return port; 
}

/*
========================
hfSessionLocalWin::CreateLobbyBackend
========================
*/
hfLobbyBackend * hfSessionLocalWin::CreateLobbyBackend( const hfMatchParameters & p, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	hfLobbyBackend * lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->StartHosting( p, skillLevel, lobbyType );
	return lobbyBackend;
}

/*
========================
hfSessionLocalWin::FindLobbyBackend
========================
*/
hfLobbyBackend * hfSessionLocalWin::FindLobbyBackend( const hfMatchParameters & p, int numPartyUsers, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	hfLobbyBackend * lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->StartFinding( p, numPartyUsers, skillLevel );
	return lobbyBackend;
}

/*
========================
hfSessionLocalWin::JoinFromConnectInfo
========================
*/
hfLobbyBackend * hfSessionLocalWin::JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	hfLobbyBackend * lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->JoinFromConnectInfo( connectInfo );
	return lobbyBackend;
}

/*
========================
hfSessionLocalWin::DestroyLobbyBackend
========================
*/
void hfSessionLocalWin::DestroyLobbyBackend( hfLobbyBackend * lobbyBackend ) {
    assert( lobbyBackend != NULL );
    assert( lobbyBackends[lobbyBackend->GetLobbyType()] == lobbyBackend );

	lobbyBackends[lobbyBackend->GetLobbyType()] = NULL;

	lobbyBackend->Shutdown();
	delete lobbyBackend;
}

/*
========================
hfSessionLocalWin::PumpLobbies
========================
*/
void hfSessionLocalWin::PumpLobbies() {
	assert( lobbyBackends[hfLobbyBackend::TYPE_PARTY] == NULL || lobbyBackends[hfLobbyBackend::TYPE_PARTY]->GetLobbyType() == hfLobbyBackend::TYPE_PARTY );
	assert( lobbyBackends[hfLobbyBackend::TYPE_GAME] == NULL || lobbyBackends[hfLobbyBackend::TYPE_GAME]->GetLobbyType() == hfLobbyBackend::TYPE_GAME );
	assert( lobbyBackends[hfLobbyBackend::TYPE_GAME_STATE] == NULL || lobbyBackends[hfLobbyBackend::TYPE_GAME_STATE]->GetLobbyType() == hfLobbyBackend::TYPE_GAME_STATE );

	// Pump lobbyBackends
	for ( int i = 0; i < lobbyBackends.Num(); i++ ) {
		if ( lobbyBackends[i] != NULL ) {
			lobbyBackends[i]->Pump();
		}
	}
}

/*
========================
hfSessionLocalWin::CreateLobbyInternal
========================
*/
hfLobbyBackend * hfSessionLocalWin::CreateLobbyInternal( hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	EnsurePort();
	hfLobbyBackend * lobbyBackend = new (TAG_NETWORKING) hfLobbyBackendDirect();

	lobbyBackend->SetLobbyType( lobbyType );

	assert( lobbyBackends[lobbyType] == NULL );
	lobbyBackends[lobbyType] = lobbyBackend;

	return lobbyBackend;
}

/*
========================
hfSessionLocalWin::JoinAfterSwap
========================
*/
void hfSessionLocalWin::JoinAfterSwap( void * joinID ) {
}

/*
========================
hfSessionLocalWin::GetLobbyAddressFromNetAddress
========================
*/
bool hfSessionLocalWin::GetLobbyAddressFromNetAddress( const netadr_t & netAddr, lobbyAddress_t & outAddr ) const {
	return false;
}

/*
========================
hfSessionLocalWin::GetNetAddressFromLobbyAddress
========================
*/
bool hfSessionLocalWin::GetNetAddressFromLobbyAddress( const lobbyAddress_t & lobbyAddress, netadr_t & outNetAddr ) const {
	return false;
}

