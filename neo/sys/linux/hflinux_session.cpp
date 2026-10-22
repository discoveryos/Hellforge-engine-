#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "../../framework/hfCommon_local.h"
#include "../hfsys_session_local.h"
#include "../hfsys_stats.h"
#include "../hfsys_savegame.h"
#include "../hfsys_lobby_backend_direct.h"
#include "../hfsys_voicechat.h"
#include "../hfsys_achievements.h"

class hfLobbyToSessionCBLocal;

extern hfCVar net_port;

/*
========================
hfLocalUserLinux
========================
*/
class hfLocalUserLinux : public hfLocalUser {
public:
	hfLocalUserLinux() : inputDevice( 0 ) {}

	void					Init( int inputDevice_, const char * gamertag_, int numLocalUsers );

	const char * GetGamerTag() const { return gamertag.c_str(); }

	// hfLocalUser interface
	virtual void			PumpPlatform() {}
	virtual bool			IsProfileReady() const { return true; }
	virtual bool			IsOnline() const { return true; }
	virtual uint32			GetOnlineCaps() const { return ( IsPersistent() && IsOnline() ) ? ( CAP_IS_ONLINE | CAP_CAN_PLAY_ONLINE ) : 0; }
	virtual int				GetInputDevice() const { return inputDevice; }
	virtual bool			IsInParty() const { return false; }
	virtual int				GetPartyCount() const { return 1; }

	void					SetInputDevice( int inputDevice_ ) { inputDevice = inputDevice_; }
	void					SetGamerTag( const char * gamerTag_ ) { gamertag = gamerTag_; }

private:
	hfStrStatic< 64 >		gamertag;
	int						inputDevice;
};

/*
========================
hfLocalUserLinux::Init
========================
*/
void hfLocalUserLinux::Init( int inputDevice_, const char * gamertag_, int numLocalUsers ) {
	if ( numLocalUsers == 1 ) {  // Check for 1, since this is now incremented before we get in here
		// This is the master user
		gamertag = gamertag_;
	} else {
		// Generate a name based off the master user for split-screen users.
		// We use the number of users on the system to generate the name rather than the device
		// number so that it is always consistently "username (2)" for the second player.
		gamertag.Format( "%s (%i)", gamertag_, numLocalUsers );
	}

	inputDevice = inputDevice_;
}

/*
================================================
hfSignInManagerLinux
================================================
*/
class hfSignInManagerLinux : public hfSignInManagerBase {
public:
	virtual ~hfSignInManagerLinux() {}

	// hfSignInManagerBase interface
	virtual void					Pump();
	virtual int						GetNumLocalUsers() const { return localUsers.Num(); }
	virtual hfLocalUser *			GetLocalUserByIndex( int index ) { return &localUsers[index]; }
	virtual const hfLocalUser *		GetLocalUserByIndex( int index ) const { return &localUsers[index]; }
	virtual void					RemoveLocalUserByIndex( int index );
	virtual void					RegisterLocalUser( int inputDevice );		// Register a local user to the passed in controller

private:
	hfStaticList< hfLocalUserLinux, MAX_INPUT_DEVICES >	localUsers;
};

/*
========================
hfSignInManagerLinux::Pump
========================
*/
void hfSignInManagerLinux::Pump() {
	// If we have more users than we need, then set to the lower amount
	// (don't remove the master user though)
	if ( localUsers.Num() > 1 && localUsers.Num() > maxDesiredLocalUsers ) {
		localUsers.SetNum( maxDesiredLocalUsers );
	}

	// If we don't have enough, then make sure we do
	while ( localUsers.Num() < minDesiredLocalUsers ) {
		RegisterLocalUser( localUsers.Num() );
	}

	// See if we need to save settings on any of the profiles
	for ( int i = 0; i < localUsers.Num(); i++ ) {
		localUsers[i].Pump();
	}
}

/*
========================
hfSignInManagerLinux::RemoveLocalUserByIndex
========================
*/
void hfSignInManagerLinux::RemoveLocalUserByIndex( int index ) {
	session->OnLocalUserSignout( &localUsers[index] );
	localUsers.RemoveIndex( index );
}

/*
========================
hfSignInManagerLinux::RegisterLocalUser
========================
*/
void hfSignInManagerLinux::RegisterLocalUser( int inputDevice ) {
	if ( GetLocalUserByInputDevice( inputDevice ) != NULL ) {
		return;
	}

	hfLocalUserLinux & localUser = *localUsers.Alloc();

	localUser.Init( inputDevice, "localplayer", localUsers.Num() );
	localUser.SetInputDevice( inputDevice );
	localUser.SetLocalUserHandle( GetUniqueLocalUserHandle( localUser.GetGamerTag() ) );

	session->OnLocalUserSignin( &localUser );
}

/*
========================
hfAchievementSystemLinux
========================
*/
class hfAchievementSystemLinux : public hfAchievementSystem {
public:
	virtual void			AchievementUnlock( hfLocalUser * user, const int achievementID ) {}
	virtual void			Pump() {}
};

/*
========================
hfVoiceChatMgrLinux
========================
*/
class hfVoiceChatMgrLinux : public hfVoiceChatMgr {
public:
	virtual bool	GetLocalChatDataInternal( int talkerIndex, byte * data, int & dataSize ) { return false; }
	virtual void	SubmitIncomingChatDataInternal( int talkerIndex, const byte * data, int dataSize ) { }
	virtual bool	TalkerHasData( int talkerIndex ) { return false; }
	virtual bool	RegisterTalkerInternal( int index ) { return true; }
	virtual void	UnregisterTalkerInternal( int index ) { }
};

/*
========================
hfSessionLocalLinux
========================
*/
class hfSessionLocalLinux : public hfSessionLocal {
friend class hfLobbyToSessionCBLocal;

public:
	hfSessionLocalLinux();
	virtual ~hfSessionLocalLinux();

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

/*
========================
hfLobbyToSessionCBLocal
========================
*/
class hfLobbyToSessionCBLocal : public hfLobbyToSessionCB {
public:
	hfLobbyToSessionCBLocal( hfSessionLocalLinux * sessionLocalLinux_ ) : sessionLocalLinux( sessionLocalLinux_ ) { }

	virtual bool CanJoinLocalHost() const { sessionLocalLinux->EnsurePort(); return sessionLocalLinux->canJoinLocalHost; }
	virtual class hfLobbyBackend * GetLobbyBackend( hfLobbyBackend::lobbyBackendType_t type ) const { return sessionLocalLinux->lobbyBackends[ type ]; }

private:
	hfSessionLocalLinux *			sessionLocalLinux;
};

hfSessionLocalLinux sessionLocalLinux;
hfSession * session = &sessionLocalLinux;

hfLobbyToSessionCBLocal lobbyToSessionCBLocal( &sessionLocalLinux );
hfLobbyToSessionCB * lobbyToSessionCB = &lobbyToSessionCBLocal;

/*
========================
hfSessionLocalLinux::hfSessionLocalLinux
========================
*/
hfSessionLocalLinux::hfSessionLocalLinux() {
	signInManager		= new (TAG_SYSTEM) hfSignInManagerLinux;
	saveGameManager		= new (TAG_SAVEGAMES) hfSaveGameManager();
	voiceChat			= new (TAG_SYSTEM) hfVoiceChatMgrLinux();
	lobbyToSessionCB	= new (TAG_SYSTEM) hfLobbyToSessionCBLocal( this );

	canJoinLocalHost	= false;

	lobbyBackends.Zero();
}

/*
========================
hfSessionLocalLinux::~hfSessionLocalLinux
========================
*/
hfSessionLocalLinux::~hfSessionLocalLinux() {
	delete voiceChat;
	delete lobbyToSessionCB;
}

/*
========================
hfSessionLocalLinux::Initialize
========================
*/
void hfSessionLocalLinux::Initialize() {
	hfSessionLocal::Initialize();

	titleStorageVars.Set( "MAX_PLAYERS_ALLOWED", "8" );
	titleStorageLoaded = true;

	// First-time check for downloadable content once game is launched
	EnumerateDownloadableContent();

	GetPartyLobby().Initialize( hfLobby::TYPE_PARTY, sessionCallbacks );
	GetGameLobby().Initialize( hfLobby::TYPE_GAME, sessionCallbacks );
	GetGameStateLobby().Initialize( hfLobby::TYPE_GAME_STATE, sessionCallbacks );

	achievementSystem = new (TAG_SYSTEM) hfAchievementSystemLinux();
	achievementSystem->Init();
}

/*
========================
hfSessionLocalLinux::Shutdown
========================
*/
void hfSessionLocalLinux::Shutdown() {
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
=======================
hfSessionLocalLinux::InitializeSoundRelatedSystems
=======================
*/
void hfSessionLocalLinux::InitializeSoundRelatedSystems() {
	if ( voiceChat != NULL ) {
		voiceChat->Init( NULL );
	}
}

/*
=======================
hfSessionLocalLinux::ShutdownSoundRelatedSystems
=======================
*/
void hfSessionLocalLinux::ShutdownSoundRelatedSystems() {
	if ( voiceChat != NULL ) {
		voiceChat->Shutdown();
	}
}

/*
=======================
hfSessionLocalLinux::PlatformPump
=======================
*/
void hfSessionLocalLinux::PlatformPump() {
	if ( signInManager != NULL ) {
		signInManager->Pump();
	}
}

/*
=======================
hfSessionLocalLinux::InviteFriends
=======================
*/
void hfSessionLocalLinux::InviteFriends() {
}

/*
=======================
hfSessionLocalLinux::InviteParty
=======================
*/
void hfSessionLocalLinux::InviteParty() {
}

/*
=======================
hfSessionLocalLinux::ShowPartySessions
=======================
*/
void hfSessionLocalLinux::ShowPartySessions() {
}

/*
=======================
hfSessionLocalLinux::ShowSystemMarketplaceUI
=======================
*/
void hfSessionLocalLinux::ShowSystemMarketplaceUI() const {
}

/*
=======================
hfSessionLocalLinux::ListServers
=======================
*/
void hfSessionLocalLinux::ListServers( const hfCallback & callback ) { 
	ListServersCommon();
}

/*
=======================
hfSessionLocalLinux::CancelListServers
=======================
*/
void hfSessionLocalLinux::CancelListServers() {
}

/*
=======================
hfSessionLocalLinux::NumServers
=======================
*/
int hfSessionLocalLinux::NumServers() const {
	return 0;
}

/*
=======================
hfSessionLocalLinux::ServerInfo
=======================
*/
const serverInfo_t * hfSessionLocalLinux::ServerInfo( int i ) const {
	return NULL;
}

/*
=======================
hfSessionLocalLinux::ConnectToServer
=======================
*/
void hfSessionLocalLinux::ConnectToServer( int i ) {
}

/*
=======================
hfSessionLocalLinux::Connect_f
=======================
*/
void hfSessionLocalLinux::Connect_f( const hfCmdArgs &args ) {
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
=======================
void Connect_f
=======================
*/
CONSOLE_COMMAND( connect, "Connect to the specified IP", NULL ) {
	sessionLocalLinux.Connect_f( args );
}

/*
=======================
hfSessionLocalLinux::ShowServerGamerCardUI
=======================
*/
void hfSessionLocalLinux::ShowServerGamerCardUI( int i ) {
}

/*
=======================
hfSessionLocalLinux::ShowLobbyUserGamerCardUI(
=======================
*/
void hfSessionLocalLinux::ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID ) {
}

/*
=======================
hfSessionLocalLinux::ProcessInputEvent
=======================
*/
bool hfSessionLocalLinux::ProcessInputEvent( const sysEvent_t * ev ) {
	if ( GetSignInManager().ProcessInputEvent( ev ) ) {
		return true;
	}
	return false;
}

/*
=======================
hfSessionLocalLinux::IsSystemUIShowing
=======================
*/
bool hfSessionLocalLinux::IsSystemUIShowing() const {
	return isSysUIShowing;
}

/*
=======================
hfSessionLocalLinux::SetSystemUIShowing
=======================
*/
void hfSessionLocalLinux::SetSystemUIShowing( bool show ) {
	isSysUIShowing = show;
}

/*
=======================
hfSessionLocalLinux::HandleServerQueryRequest
=======================
*/
void hfSessionLocalLinux::HandleServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType ) {
	NET_VERBOSE_PRINT( "HandleServerQueryRequest from %s\n", remoteAddr.ToString() );
}

/*
=======================
hfSessionLocalLinux::HandleServerQueryAck
=======================
*/
void hfSessionLocalLinux::HandleServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg ) {
	NET_VERBOSE_PRINT( "HandleServerQueryAck from %s\n", remoteAddr.ToString() );
}

/*
=======================
hfSessionLocalLinux::ClearBootableInvite
=======================
*/
void hfSessionLocalLinux::ClearBootableInvite() {
}

/*
=======================
hfSessionLocalLinux::ClearPendingInvite
=======================
*/
void hfSessionLocalLinux::ClearPendingInvite() {
}

/*
=======================
hfSessionLocalLinux::HandleBootableInvite
=======================
*/
void hfSessionLocalLinux::HandleBootableInvite( int64 lobbyId ) {
}

/*
=======================
hfSessionLocalLinux::HasPendingBootableInvite
=======================
*/
bool hfSessionLocalLinux::HasPendingBootableInvite() {
	return false;
}

/*
=======================
hfSessionLocalLinux::SetDiscSwapMPInvite
=======================
*/
void hfSessionLocalLinux::SetDiscSwapMPInvite( void * parm ) {
}

/*
=======================
hfSessionLocalLinux::GetDiscSwapMPInviteParms
=======================
*/
void * hfSessionLocalLinux::GetDiscSwapMPInviteParms() {
	return NULL;
}

/*
=======================
hfSessionLocalLinux::EnumerateDownloadableContent
=======================
*/
void hfSessionLocalLinux::EnumerateDownloadableContent() {
}

/*
=======================
hfSessionLocalLinux::LeaderboardUpload
=======================
*/
void hfSessionLocalLinux::LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t * leaderboard, const column_t * stats, const hfFile_Memory * attachment ) {
}

/*
=======================
hfSessionLocalLinux::LeaderboardFlush
=======================
*/
void hfSessionLocalLinux::LeaderboardFlush() {
}

/*
=======================
hfSessionLocalLinux::LeaderboardDownload
=======================
*/
void hfSessionLocalLinux::LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int startingRank, int numRows, const hfLeaderboardCallback & callback ) {
}

/*
=======================
hfSessionLocalLinux::LeaderboardDownloadAttachment
=======================
*/
void hfSessionLocalLinux::LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int64 attachmentID ) {
}

/*
=======================
hfSessionLocalLinux::EnsurePort
=======================
*/
void hfSessionLocalLinux::EnsurePort() {	
	// Init the port using regular sockets
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
=======================
hfSessionLocalLinux::GetPort
=======================
*/
hfNetSessionPort & hfSessionLocalLinux::GetPort( bool dedicated ) { 
	EnsurePort(); 
	return port; 
}

/*
=======================
hfSessionLocalLinux::CreateLobbyBackend
=======================
*/
hfLobbyBackend * hfSessionLocalLinux::CreateLobbyBackend( const hfMatchParameters & p, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	hfLobbyBackend * lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->StartHosting( p, skillLevel, lobbyType );
	return lobbyBackend;
}

/*
=======================
hfSessionLocalLinux::FindLobbyBackend
=======================
*/
hfLobbyBackend * hfSessionLocalLinux::FindLobbyBackend( const hfMatchParameters & p, int numPartyUsers, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	hfLobbyBackend * lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->StartFinding( p, numPartyUsers, skillLevel );
	return lobbyBackend;
}

/*
=======================
hfSessionLocalLinux::JoinFromConnectInfo
=======================
*/
hfLobbyBackend * hfSessionLocalLinux::JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo, hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	hfLobbyBackend * lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->JoinFromConnectInfo( connectInfo );
	return lobbyBackend;
}

/*
=======================
hfSessionLocalLinux::DestroyLobbyBackend
=======================
*/
void hfSessionLocalLinux::DestroyLobbyBackend( hfLobbyBackend * lobbyBackend ) {
    assert( lobbyBackend != NULL );
    assert( lobbyBackends[lobbyBackend->GetLobbyType()] == lobbyBackend );

	lobbyBackends[lobbyBackend->GetLobbyType()] = NULL;

	lobbyBackend->Shutdown();
	delete lobbyBackend;
}

/*
=======================
hfSessionLocalLinux::PumpLobbies
=======================
*/
void hfSessionLocalLinux::PumpLobbies() {
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
=======================
hfSessionLocalLinux::JoinAfterSwap
=======================
*/
void hfSessionLocalLinux::JoinAfterSwap( void * joinID ) {
}

/*
=======================
hfSessionLocalLinux::CreateLobbyInternal
=======================
*/
hfLobbyBackend * hfSessionLocalLinux::CreateLobbyInternal( hfLobbyBackend::lobbyBackendType_t lobbyType ) {
	EnsurePort();
	hfLobbyBackend * lobbyBackend = new (TAG_NETWORKING) hfLobbyBackendDirect();

	lobbyBackend->SetLobbyType( lobbyType );

	assert( lobbyBackends[lobbyType] == NULL );
	lobbyBackends[lobbyType] = lobbyBackend;

	return lobbyBackend;
}

/*
=======================
hfSessionLocalLinux::GetLobbyAddressFromNetAddress
=======================
*/
bool hfSessionLocalLinux::GetLobbyAddressFromNetAddress( const netadr_t & netAddr, lobbyAddress_t & outAddr ) const {
	return false;
}

/*
=======================
hfSessionLocalLinux::GetNetAddressFromLobbyAddress
=======================
*/
bool hfSessionLocalLinux::GetNetAddressFromLobbyAddress( const lobbyAddress_t & lobbyAddress, netadr_t & outNetAddr ) const {
	return false;
}
