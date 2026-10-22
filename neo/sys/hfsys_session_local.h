
#undef private
#undef protected

#include "win32/hfwin_achievements.h"
#include "win32/hfwin_signin.h"

#include "hfsys_lobby_backend.h"
#include "hfsys_lobby.h"

class hfSaveGameProcessorNextMap;
class hfSaveGameProcessorSaveGame;
class hfSaveGameProcessorLoadGame;
class hfSaveGameProcessorDelete;
class hfSaveGameProcessorEnumerateGames;

/*
================================================
hfLobbyStub
================================================
*/
class hfLobbyStub : public hfLobbyBase {
public:
	virtual bool						IsHost() const { return false; }
	virtual bool						IsPeer() const { return false; }
	virtual bool						HasActivePeers() const { return false; }
	virtual int							GetNumLobbyUsers() const { return 0; }
	virtual int							GetNumActiveLobbyUsers() const { return 0; }
	virtual bool						IsLobbyUserConnected( int index ) const { return false; }

	virtual lobbyUserID_t				GetLobbyUserIdByOrdinal( int userIndex ) const { return lobbyUserID_t(); }
	virtual	int							GetLobbyUserIndexFromLobbyUserID( lobbyUserID_t lobbyUserID ) const { return -1; }

	virtual void						SendReliable( int type, hfBitMsg & msg, bool callReceiveReliable = true, peerMask_t sessionUserMask = MAX_UNSIGNED_TYPE( peerMask_t ) ) {}
	virtual void						SendReliableToLobbyUser( lobbyUserID_t lobbyUserID, int type, hfBitMsg & msg ) {}
	virtual void						SendReliableToHost( int type, hfBitMsg & msg ) {}

	virtual const char *				GetLobbyUserName( lobbyUserID_t lobbyUserID ) const { return "INVALID"; }
	virtual void						KickLobbyUser( lobbyUserID_t lobbyUserID ) {}
	virtual bool						IsLobbyUserValid( lobbyUserID_t lobbyUserID ) const { return false; }
	virtual bool						IsLobbyUserLoaded( lobbyUserID_t lobbyUserID ) const { return false; }
	virtual bool						LobbyUserHasFirstFullSnap( lobbyUserID_t lobbyUserID ) const { return false; }
	virtual void						EnableSnapshotsForLobbyUser( lobbyUserID_t lobbyUserID ) {}

	virtual int							GetLobbyUserSkinIndex( lobbyUserID_t lobbyUserID ) const { return 0; }
	virtual bool						GetLobbyUserWeaponAutoReload( lobbyUserID_t lobbyUserID ) const { return false; }
	virtual bool						GetLobbyUserWeaponAutoSwitch( lobbyUserID_t lobbyUserID ) const { return false; }
	virtual int							GetLobbyUserLevel( lobbyUserID_t lobbyUserID ) const { return 0; }
	virtual int							GetLobbyUserQoS( lobbyUserID_t lobbyUserID ) const { return 0; }
	virtual int							GetLobbyUserTeam( lobbyUserID_t lobbyUserID ) const { return 0; }
	virtual bool						SetLobbyUserTeam( lobbyUserID_t lobbyUserID, int teamNumber ) { return false; }
	virtual int							GetLobbyUserPartyToken( lobbyUserID_t lobbyUserID ) const { return 0; }
	virtual hfPlayerProfile *			GetProfileFromLobbyUser( lobbyUserID_t lobbyUserID ) { return NULL; }
	virtual hfLocalUser *				GetLocalUserFromLobbyUser( lobbyUserID_t lobbyUserID ) { return NULL; }
	virtual int							GetNumLobbyUsersOnTeam( int teamNumber ) const { return 0; }

	virtual int							PeerIndexFromLobbyUser( lobbyUserID_t lobbyUserID ) const { return -1; }

	virtual int							GetPeerTimeSinceLastPacket( int peerIndex ) const { return 0; }
	virtual int							PeerIndexForHost() const { return -1; }

	virtual lobbyUserID_t				AllocLobbyUserSlotForBot( const char * botName ) { return lobbyUserID_t(); }
	virtual void						RemoveBotFromLobbyUserList( lobbyUserID_t lobbyUserID ) {}
	virtual bool						GetLobbyUserIsBot( lobbyUserID_t lobbyUserID ) const { return false; }

	virtual const char *				GetHostUserName() const { return "INVALID"; }
	virtual const hfMatchParameters &	GetMatchParms() const { return fakeParms; }
	virtual bool						IsLobbyFull() const { return false; }

	virtual bool						EnsureAllPeersHaveBaseState() { return false; }
	virtual bool						AllPeersInGame() const { return false; }
	virtual int							GetNumConnectedPeers() const { return 0; }
	virtual int							GetNumConnectedPeersInGame() const { return 0; }
	virtual int							PeerIndexOnHost() const { return -1; }
	virtual bool						IsPeerDisconnected( int peerIndex ) const { return false; }

	virtual bool						AllPeersHaveStaleSnapObj( int objId ) { return false; }
	virtual bool						AllPeersHaveExpectedSnapObj( int objId ) { return false; }
	virtual void						RefreshSnapObj( int objId ) {}
	virtual void						MarkSnapObjDeleted( int objId ) {}
	virtual void						AddSnapObjTemplate( int objID, hfBitMsg & msg ) {}

	virtual void						DrawDebugNetworkHUD() const {}
	virtual void						DrawDebugNetworkHUD2() const {}
	virtual void						DrawDebugNetworkHUD_ServerSnapshotMetrics( bool draw ) {}
private:
	hfMatchParameters					fakeParms;
};

/*
================================================
hfSessionLocal 
================================================
*/
class hfSessionLocal : public hfSession {
	friend class hfLeaderboards;
	friend class hfStatsSession;
	friend class hfLobbyBackend360;
	friend class hfLobbyBackendPS3;
	friend class hfSessionLocalCallbacks;
	friend class hfPsnAsyncSubmissionLookupPS3_TitleStorage;
	friend class hfNetSessionPort;
	friend class lobbyAddress_t;
	
protected:
	//=====================================================================================================
	//	Mixed Common/Platform enums/structs
	//=====================================================================================================

	// Overall state of the session
	enum state_t {
		STATE_PRESS_START,							// We are at press start
		STATE_IDLE,									// We are at the main menu
		STATE_PARTY_LOBBY_HOST,						// We are in the party lobby menu as host
		STATE_PARTY_LOBBY_PEER,						// We are in the party lobby menu as a peer
		STATE_GAME_LOBBY_HOST,						// We are in the game lobby as a host
		STATE_GAME_LOBBY_PEER,						// We are in the game lobby as a peer
		STATE_GAME_STATE_LOBBY_HOST,				// We are in the game state lobby as a host
		STATE_GAME_STATE_LOBBY_PEER,				// We are in the game state lobby as a peer
		STATE_CREATE_AND_MOVE_TO_PARTY_LOBBY,		// We are creating a party lobby, and will move to that state when done
		STATE_CREATE_AND_MOVE_TO_GAME_LOBBY,		// We are creating a game lobby, and will move to that state when done
		STATE_CREATE_AND_MOVE_TO_GAME_STATE_LOBBY,	// We are creating a game state lobby, and will move to that state when done
		STATE_FIND_OR_CREATE_MATCH,							
		STATE_CONNECT_AND_MOVE_TO_PARTY,
		STATE_CONNECT_AND_MOVE_TO_GAME,
		STATE_CONNECT_AND_MOVE_TO_GAME_STATE,
		STATE_BUSY,									// Doing something internally like a QoS/bandwidth challenge

		// These are last, so >= STATE_LOADING tests work
		STATE_LOADING,								// We are loading the map, preparing to go into a match
		STATE_INGAME,								// We are currently in a match
		NUM_STATES
	};

	enum connectType_t {
		CONNECT_NONE				= 0,
		CONNECT_DIRECT				= 1,
		CONNECT_FIND_OR_CREATE		= 2,
	};

	enum pendingInviteMode_t {
		PENDING_INVITE_NONE			= 0,		// No invite waiting
		PENDING_INVITE_WAITING		= 1,		// Invite is waiting
		PENDING_SELF_INVITE_WAITING	= 2,		// We invited ourselves to a match
	};

	struct contentData_t {
		bool						isMounted;
		hfStrStatic<128>			displayName;
		hfStrStatic< MAX_OSPATH >	packageFileName;
		hfStrStatic< MAX_OSPATH >	rootPath;
		int							dlcID;
	};

public:
							hfSessionLocal();
	virtual					~hfSessionLocal();

	void					InitBaseState();

	virtual bool			IsPlatformPartyInLobby();

	// Downloadable Content
	virtual int				GetNumContentPackages() const;
	virtual int				GetContentPackageID( int contentIndex ) const;
	virtual const char *	GetContentPackagePath( int contentIndex ) const;
	virtual int				GetContentPackageIndexForID( int contentID ) const;

	virtual bool			GetSystemMarketplaceHasNewContent() const { return marketplaceHasNewContent; }
	virtual void			SetSystemMarketplaceHasNewContent( bool hasNewContent ) { marketplaceHasNewContent = hasNewContent; }

	// Lobby management 
	virtual void			CreatePartyLobby( const hfMatchParameters & parms_ );
	virtual void			FindOrCreateMatch( const hfMatchParameters & parms );
	virtual void			CreateMatch( const hfMatchParameters & parms_ );
	virtual void			CreateGameStateLobby( const hfMatchParameters & parms_ );

	virtual void			UpdatePartyParms( const hfMatchParameters & parms_ );
	virtual void			UpdateMatchParms( const hfMatchParameters & parms_ );
	virtual void			StartMatch();
	virtual	void			SetSessionOption( sessionOption_t option ) { sessionOptions |= option; }
	virtual	void			ClearSessionOption( sessionOption_t option ) { sessionOptions &= ~option; }
	virtual sessionState_t	GetBackState();
	virtual void			Cancel();
	virtual void			MoveToPressStart();
	virtual void			FinishDisconnect();
	virtual bool			ShouldShowMigratingDialog() const;	// Note this is not in hfsys_session.h
	virtual bool			IsCurrentLobbyMigrating() const;
	virtual bool			IsLosingConnectionToHost() const;
	
	// Migration
	virtual bool			WasMigrationGame() const;
	virtual bool			ShouldRelaunchMigrationGame() const;
	virtual bool			GetMigrationGameData( hfBitMsg & msg, bool reading );
	virtual bool			GetMigrationGameDataUser( lobbyUserID_t lobbyUserID, hfBitMsg & msg, bool reading );

	virtual bool			WasGameLobbyCoalesced() const { return gameLobbyWasCoalesced; }

	// Misc
	virtual	int				GetLoadingID() { return loadingID; }
	virtual bool			IsAboutToLoad() const { return GetGameLobby().IsLobbyActive() && GetGameLobby().startLoadingFromHost; }
	virtual bool			GetMatchParamUpdate( int &peer, int &msg );
	virtual int				GetInputRouting( int inputRouting[ MAX_INPUT_DEVICES ] );
	virtual void			EndMatch( bool premature=false );			// Meant for host to end match gracefully, go back to lobby, tally scores, etc
	virtual void			MatchFinished();							// this is for when the game is over before we go back to lobby. Need this incase the host leaves during this time
	virtual void			QuitMatch();		// Meant for host or peer to quit the match before it ends, will instigate host migration, etc
	virtual void			QuitMatchToTitle(); // Will forcefully quit the match and return to the title screen.
	virtual void			LoadingFinished();
	virtual void			Pump();
	virtual void			ProcessSnapAckQueue();

	virtual sessionState_t	GetState() const;
	virtual const char *	GetStateString() const ;
	
	virtual void			SendUsercmds( hfBitMsg & msg );
	virtual void			SendSnapshot( hfSnapShot & ss );
	virtual const char *	GetPeerName( int peerNum );

	virtual const char *	GetLocalUserName( int i ) const { return signInManager->GetLocalUserByIndex( i )->GetGamerTag(); }
	virtual void			UpdateSignInManager();
	virtual hfPlayerProfile * GetProfileFromMasterLocalUser();

	virtual void			PrePickNewHost( hfLobby & lobby, bool forceMe, bool inviteOldHost );
	virtual bool			PreMigrateInvite( hfLobby & lobby );

	//=====================================================================================================
	// Title Storage Vars
	//=====================================================================================================
	virtual float			GetTitleStorageFloat( const char * name, float defaultFloat ) const { return titleStorageVars.GetFloat( name, defaultFloat ); }
	virtual int				GetTitleStorageInt( const char * name, int defaultInt ) const { return titleStorageVars.GetInt( name, defaultInt ); }
	virtual bool			GetTitleStorageBool( const char * name, bool defaultBool ) const { return titleStorageVars.GetBool( name, defaultBool ); }
	virtual const char *	GetTitleStorageString( const char * name, const char * defaultString ) const { return titleStorageVars.GetString( name, defaultString ); }

	virtual bool			GetTitleStorageFloat( const char * name, float defaultFloat, float & out ) const { return titleStorageVars.GetFloat( name, defaultFloat, out ); }
	virtual bool			GetTitleStorageInt( const char * name, int defaultInt, int & out ) const { return titleStorageVars.GetInt( name, defaultInt, out ); }
	virtual bool			GetTitleStorageBool( const char * name, bool defaultBool, bool & out ) const { return titleStorageVars.GetBool( name, defaultBool, out ); }
	virtual bool			GetTitleStorageString( const char * name, const char * defaultString, const char ** out ) const { return titleStorageVars.GetString( name, defaultString, out ); }

	virtual bool			IsTitleStorageLoaded() { return titleStorageLoaded; }

	//=====================================================================================================
	//	Voice chat
	//=====================================================================================================
	virtual voiceState_t		GetLobbyUserVoiceState( lobbyUserID_t lobbyUserID );
	virtual voiceStateDisplay_t GetDisplayStateFromVoiceState( voiceState_t voiceState ) const;
	virtual void				ToggleLobbyUserVoiceMute( lobbyUserID_t lobbyUserID );
	virtual void				SetActiveChatGroup( int groupIndex );
	virtual void				UpdateMasterUserHeadsetState();

	//=====================================================================================================
	//	Bandwidth / QoS checking
	//=====================================================================================================
	virtual bool			StartOrContinueBandwidthChallenge( bool forceStart );
	virtual void			DebugSetPeerSnaprate( int peerIndex, int snapRateMS );
	virtual float			GetIncomingByteRate();

	//=====================================================================================================
	// Invites
	//=====================================================================================================
	virtual void			HandleBootableInvite( int64 lobbyId = 0 ) = 0;
	virtual void			ClearBootableInvite() = 0;
	virtual void			ClearPendingInvite() = 0;
	virtual bool			HasPendingBootableInvite() = 0;
	virtual void			SetDiscSwapMPInvite( void * parm ) = 0;		// call to request a discSwap multiplayer invite
	virtual void *			GetDiscSwapMPInviteParms() = 0;
	virtual bool			IsDiscSwapMPInviteRequested() const { return inviteInfoRequested; }

	bool					GetFlushedStats() { return flushedStats; }
	void					SetFlushedStats( bool _flushedStats ) { flushedStats = _flushedStats; }
	
	//=====================================================================================================
	// Notifications
	//=====================================================================================================
	// This is called when a LocalUser is signed in/out
	virtual void			OnLocalUserSignin( hfLocalUser * user );
	virtual void			OnLocalUserSignout( hfLocalUser * user );

	// This is called when the master LocalUser is signed in/out, these are called after OnLocalUserSignin/out()
	virtual void			OnMasterLocalUserSignout();
	virtual void			OnMasterLocalUserSignin();

	// After a local user has signed in and their profile has loaded
	virtual void			OnLocalUserProfileLoaded( hfLocalUser * user );

	//=====================================================================================================
	//	Platform specific (different platforms implement these differently)
	//=====================================================================================================

	virtual void			Initialize() = 0;
	virtual void			Shutdown() = 0;

	virtual void			InitializeSoundRelatedSystems() = 0;
	virtual void			ShutdownSoundRelatedSystems() = 0;

	virtual void			PlatformPump() = 0;

	virtual void			InviteFriends() = 0;
	virtual void			InviteParty() = 0;
	virtual void			ShowPartySessions() = 0;

	virtual bool			ProcessInputEvent( const sysEvent_t * ev ) = 0;

	// Play with Friends server listing
	virtual int				NumServers() const = 0;
	virtual void			ListServers( const hfCallback & callback ) = 0;
	virtual void			ListServersCommon();
	virtual void			CancelListServers() = 0;
	virtual void			ConnectToServer( int i ) = 0;
	virtual const serverInfo_t * ServerInfo( int i ) const = 0;
	virtual const hfList< hfStr > * ServerPlayerList( int i );
	virtual void			ShowServerGamerCardUI( int i ) = 0;

	virtual void 			HandleServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType ) = 0;
	virtual void 			HandleServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg ) = 0;

	// System UI
	virtual bool			IsSystemUIShowing() const = 0;
	virtual void			SetSystemUIShowing( bool show ) = 0;

	virtual void			ShowSystemMarketplaceUI() const = 0;
	virtual void			ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID ) = 0;

	// Leaderboards
	virtual void			LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t * leaderboard, const column_t * stats, const hfFile_Memory * attachment = NULL ) = 0;
	virtual void			LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int startingRank, int numRows, const hfLeaderboardCallback & callback ) = 0;
	virtual void			LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int64 attachmentID ) = 0;

	// Scoring (currently just for TrueSkill)
	virtual void			SetLobbyUserRelativeScore( lobbyUserID_t lobbyUserID, int relativeScore, int team ) = 0;

	virtual void			LeaderboardFlush() = 0;

	//=====================================================================================================i'
	//	Savegames
	//=====================================================================================================
	virtual saveGameHandle_t	SaveGameSync( const char * name, const saveFileEntryList_t & files, const hfSaveGameDetails & description );
	virtual saveGameHandle_t	SaveGameAsync( const char * name, const saveFileEntryList_t & files, const hfSaveGameDetails & description );
	virtual saveGameHandle_t	LoadGameSync( const char * name, saveFileEntryList_t & files );
	virtual saveGameHandle_t	EnumerateSaveGamesSync();
	virtual saveGameHandle_t	EnumerateSaveGamesAsync();
	virtual saveGameHandle_t	DeleteSaveGameSync( const char * name );
	virtual saveGameHandle_t	DeleteSaveGameAsync( const char * name );

	virtual bool							IsSaveGameCompletedFromHandle( const saveGameHandle_t & handle ) const { return saveGameManager->IsSaveGameCompletedFromHandle( handle ); }
	virtual void							CancelSaveGameWithHandle( const saveGameHandle_t & handle ) { GetSaveGameManager().CancelWithHandle( handle ); }
	virtual const saveGameDetailsList_t &	GetEnumeratedSavegames() const { return saveGameManager->GetEnumeratedSavegames(); }
	virtual bool							IsEnumerating() const;
	virtual saveGameHandle_t				GetEnumerationHandle() const;
	virtual void							SetCurrentSaveSlot( const char * slotName ) { currentSaveSlot = slotName; }
	virtual const char *					GetCurrentSaveSlot() const { return currentSaveSlot.c_str(); }

	// Notifications
	void					OnSaveCompleted( hfSaveLoadParms * parms );
	void					OnLoadCompleted( hfSaveLoadParms * parms );
	void					OnDeleteCompleted( hfSaveLoadParms * parms );
	void					OnEnumerationCompleted( hfSaveLoadParms * parms );

	// Error checking
	virtual bool			IsDLCAvailable( const char * mapName );
	virtual bool			LoadGameCheckDiscNumber( hfSaveLoadParms & parms );
	bool					LoadGameCheckDescriptionFile( hfSaveLoadParms & parms );

	// Downloadable Content
	virtual void			EnumerateDownloadableContent() = 0;

	void					DropClient( int peerNum, int session );

protected:

	float					GetUpstreamDropRate() { return upstreamDropRate; }
	float					GetUpstreamQueueRate() { return upstreamQueueRate; }
	int						GetQueuedBytes() { return queuedBytes; }

	//=====================================================================================================
	// Common functions (hfsys_session_local.cpp)
	//=====================================================================================================
	void					HandleLobbyControllerState( int lobbyType );
	virtual void			UpdatePendingInvite();
	bool					HandleState();

	// The party and game lobby are the two platform lobbies that notify the backends (Steam/PSN/LIVE of changes)
	hfLobby &				GetPartyLobby()				{ return partyLobby; }
	const hfLobby &			GetPartyLobby() const		{ return partyLobby; }
	hfLobby &				GetGameLobby()				{ return gameLobby; }
	const hfLobby &			GetGameLobby() const		{ return gameLobby; }

	// Game state lobby is the lobby used while in-game.  It is so the dedicated server can host this lobby
	// and have all platform clients join. It does NOT notify the backends of changes, it's purely for the dedicated
	// server to be able to host the in-game lobby.
	// Generally, you would call GetActingGameStateLobby.  If we are not using game state lobby, GetActingGameStateLobby will return GetGameLobby insread.
	hfLobby &				GetGameStateLobby()			{ return gameStateLobby; }
	const hfLobby &			GetGameStateLobby() const	{ return gameStateLobby; }

	hfLobby &				GetActingGameStateLobby();
	const hfLobby &			GetActingGameStateLobby() const;
	
	// GetActivePlatformLobby will return either the game or party lobby, it won't return the game state lobby
	// This function is generally used for menus, in-game code should refer to GetActingGameStateLobby
	hfLobby *				GetActivePlatformLobby();
	const hfLobby *			GetActivePlatformLobby() const;
	
	hfLobby *				GetLobbyFromType( hfLobby::lobbyType_t lobbyType );

	virtual hfLobbyBase &	GetPartyLobbyBase()					{ return partyLobby; }
	virtual hfLobbyBase &	GetGameLobbyBase()					{ return gameLobby; }
	virtual hfLobbyBase &	GetActingGameStateLobbyBase()		{ return GetActingGameStateLobby(); }

	virtual hfLobbyBase &	GetActivePlatformLobbyBase();
	virtual hfLobbyBase &	GetLobbyFromLobbyUserID( lobbyUserID_t lobbyUserID );

	void					SetState( state_t newState );
	bool					HandlePackets();

	void					HandleVoiceRestrictionDialog();
	void					SetDroppedByHost( bool dropped ) { droppedByHost = dropped; }
	bool					GetDroppedByHost() { return droppedByHost; }

public:
	int						storedPeer;
	int						storedMsgType;

protected:
	static const char *		stateToString[ NUM_STATES ];

	state_t					localState;
	uint32					sessionOptions;

	connectType_t			connectType;
	int						connectTime;

	hfLobby					partyLobby;
	hfLobby					gameLobby;
	hfLobby					gameStateLobby;
	hfLobbyStub				stubLobby;				// We use this when we request the active lobby when we are not in a lobby (i.e at press start)

	int						currentID;				// The host used this to send out a unique id to all users so we can identify them

	class hfVoiceChatMgr *	voiceChat;	
	int						lastVoiceSendtime;
	bool					hasShownVoiceRestrictionDialog;

	pendingInviteMode_t		pendingInviteMode;
	int						pendingInviteDevice;
	lobbyConnectInfo_t		pendingInviteConnectInfo;

	bool					isSysUIShowing;

	hfDict					titleStorageVars;
	bool					titleStorageLoaded;

	int						showMigratingInfoStartTime;

	int						nextGameCoalesceTime;
	bool					gameLobbyWasCoalesced;
	int						numFullSnapsReceived;
	
	bool					flushedStats;

	int						loadingID;

	bool					inviteInfoRequested;

	hfSaveGameProcessorSaveFiles * processorSaveFiles;
	hfSaveGameProcessorLoadFiles * processorLoadFiles;
	hfSaveGameProcessorDelete * processorDelete;
	hfSaveGameProcessorEnumerateGames * processorEnumerate;

	hfStr							currentSaveSlot;
	saveGameHandle_t				enumerationHandle;

	//------------------------
	// State functions
	//------------------------
	bool	State_Party_Lobby_Host();
	bool	State_Party_Lobby_Peer();
	bool	State_Game_Lobby_Host();
	bool	State_Game_Lobby_Peer();
	bool	State_Game_State_Lobby_Host();
	bool	State_Game_State_Lobby_Peer();
	bool	State_Loading();
	bool	State_InGame();	
	bool	State_Find_Or_Create_Match();
	bool	State_Create_And_Move_To_Party_Lobby();
	bool	State_Create_And_Move_To_Game_Lobby();
	bool	State_Create_And_Move_To_Game_State_Lobby();

	bool	State_Connect_And_Move_To_Party();
	bool	State_Connect_And_Move_To_Game();
	bool	State_Connect_And_Move_To_Game_State();
	bool	State_Finalize_Connect();
	bool	State_Busy();

	// -----------------------
	// Downloadable Content
	// -----------------------
	static const int MAX_CONTENT_PACKAGES = 16;


	hfStaticList<contentData_t, MAX_CONTENT_PACKAGES>	downloadedContent;
	bool												marketplaceHasNewContent;

	class hfQueuePacket {
	public:
		byte						data[ hfPacketProcessor::MAX_FINAL_PACKET_SIZE ];
		lobbyAddress_t				address;
		int							size;
		int							time;
		bool						dedicated;
		hfQueueNode<hfQueuePacket>	queueNode;
	};

	hfBlockAlloc< hfQueuePacket, 64, TAG_NETWORKING >	packetAllocator;
	hfQueue< hfQueuePacket,&hfQueuePacket::queueNode >	sendQueue;
	hfQueue< hfQueuePacket,&hfQueuePacket::queueNode >	recvQueue;

	float												upstreamDropRate;		// instant rate in B/s at which we are dropping packets due to simulated upstream saturation
	int													upstreamDropRateTime;

	float												upstreamQueueRate;		// instant rate in B/s at which queued packets are coming out after local buffering due to upstream saturation
	int													upstreamQueueRateTime;

	int													queuedBytes;

	int													waitingOnGameStateMembersToLeaveTime;
	int													waitingOnGameStateMembersToJoinTime;

	void	TickSendQueue();

	void	QueuePacket( hfQueue< hfQueuePacket,&hfQueuePacket::queueNode > & queue, int time, const lobbyAddress_t & to, const void * data, int size, bool dedicated );
	bool	ReadRawPacketFromQueue( int time, lobbyAddress_t & from, void * data, int & size, bool & outDedicated, int maxSize );

	void	SendRawPacket( const lobbyAddress_t & to, const void * data, int size, bool dedicated );
	bool	ReadRawPacket( lobbyAddress_t & from, void * data, int & size, bool & outDedicated, int maxSize );

	void	ConnectAndMoveToLobby( hfLobby & lobby, const lobbyConnectInfo_t & connectInfo, bool fromInvite );
	void	GoodbyeFromHost( hfLobby & lobby, int peerNum, const lobbyAddress_t & remoteAddress, int msgType );

	void	WriteLeaderboardToMsg( hfBitMsg & msg, const leaderboardDefinition_t * leaderboard, const column_t * stats );
	void	SendLeaderboardStatsToPlayer( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t * leaderboard, const column_t * stats );
	void	RecvLeaderboardStatsForPlayer( hfBitMsg & msg );

	const leaderboardDefinition_t * ReadLeaderboardFromMsg( hfBitMsg & msg, column_t * stats );
	bool	RequirePersistentMaster();
	
	virtual hfNetSessionPort &	GetPort( bool dedicated = false ) = 0; 
	virtual hfLobbyBackend *	CreateLobbyBackend( const hfMatchParameters & p, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) = 0;
	virtual hfLobbyBackend *	FindLobbyBackend( const hfMatchParameters & p, int numPartyUsers, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType ) = 0;
	virtual hfLobbyBackend *	JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo , hfLobbyBackend::lobbyBackendType_t lobbyType ) = 0;
	virtual void				DestroyLobbyBackend( hfLobbyBackend * lobby ) = 0;
	virtual void				PumpLobbies() = 0;
	virtual bool				GetLobbyAddressFromNetAddress( const netadr_t & netAddr, lobbyAddress_t & outAddr ) const = 0;
	virtual bool				GetNetAddressFromLobbyAddress( const lobbyAddress_t & lobbyAddress, netadr_t & outNetAddr ) const = 0;

	void 	HandleDedicatedServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType );
	void 	HandleDedicatedServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg );


	void	ClearMigrationState();	
	// this is called when the mathc is over and returning to lobby
	void	EndMatchInternal( bool premature=false );
	// this is called when the game finished and we are in the end match recap
	void	MatchFinishedInternal();
	void	EndMatchForMigration();

	void	MoveToPressStart( gameDialogMessages_t msg );

	// Voice chat
	void	SendVoiceAudio();
	void	HandleOobVoiceAudio( const lobbyAddress_t & from, const hfBitMsg & msg );
	void	SetVoiceGroupsToTeams();
	void	ClearVoiceGroups();

	// All the new functions going here for now until it can all be cleaned up
	void	StartSessions();
	void	EndSessions();
	void	SetLobbiesAreJoinable( bool joinable );
	void	MoveToMainMenu();				// End all session (async), and return to IDLE state
	bool	WaitOnLobbyCreate( hfLobby & lobby );
	bool	DetectDisconnectFromService( bool cancelAndShowMsg );
	void	HandleConnectionFailed( hfLobby & lobby, bool wasFull );
	void	ConnectToNextSearchResultFailed( hfLobby & lobby );
	bool	HandleConnectAndMoveToLobby( hfLobby & lobby );

	void	VerifySnapshotInitialState();

	void	ComputeNextGameCoalesceTime();

	void	StartLoading();

	bool	ShouldHavePartyLobby();
	void	ValidateLobbies();
	void	ValidateLobby( hfLobby & lobby );

	void	ReadTitleStorage( void * buffer, int bufferLen );

	bool	ReadDLCInfo( hfDict & dlcInfo, void * buffer, int bufferLen );

	hfSessionCallbacks	* sessionCallbacks;

	int		offlineTransitionTimerStart;

	bool	droppedByHost;


};

/*
========================
hfSessionLocalCallbacks
	The more the hfLobby class needs to call back into this class, the more likely we're doing something wrong, and there is a better way.
========================
*/
class hfSessionLocalCallbacks : public hfSessionCallbacks {
public:
	hfSessionLocalCallbacks( hfSessionLocal * sessionLocal_ ) { sessionLocal = sessionLocal_; }

	virtual hfLobby &				GetPartyLobby()				{ return sessionLocal->GetPartyLobby(); }
	virtual hfLobby &				GetGameLobby()				{ return sessionLocal->GetGameLobby(); }
	virtual hfLobby &				GetActingGameStateLobby()	{ return sessionLocal->GetActingGameStateLobby(); }
	virtual hfLobby *				GetLobbyFromType( hfLobby::lobbyType_t lobbyType ) { return sessionLocal->GetLobbyFromType( lobbyType ); }

	virtual int						GetUniquePlayerId() const { return sessionLocal->currentID++; }
	virtual hfSignInManagerBase	&	GetSignInManager() { return *sessionLocal->signInManager; }
	virtual	void					SendRawPacket( const lobbyAddress_t & to, const void * data, int size, bool useDirectPort ) { sessionLocal->SendRawPacket( to, data, size, useDirectPort ); }
	
	virtual bool					BecomingHost( hfLobby & lobby );
	virtual void					BecameHost( hfLobby & lobby );
	virtual bool					BecomingPeer( hfLobby & lobby );
	virtual void					BecamePeer( hfLobby & lobby );

	virtual void					FailedGameMigration( hfLobby & lobby );
	virtual void					MigrationEnded( hfLobby & lobby );

	virtual void					GoodbyeFromHost( hfLobby & lobby, int peerNum, const lobbyAddress_t & remoteAddress, int msgType );
	virtual	uint32					GetSessionOptions() { return sessionLocal->sessionOptions; }

	virtual bool					AnyPeerHasAddress( const lobbyAddress_t & remoteAddress ) const;

	virtual hfSession::sessionState_t GetState() const { return sessionLocal->GetState(); }

	virtual void					ClearMigrationState() { GetPartyLobby().ResetAllMigrationState(); GetGameLobby().ResetAllMigrationState(); }

	virtual void					EndMatchInternal( bool premature=false ) { sessionLocal->EndMatchInternal( premature ); }

	virtual void					RecvLeaderboardStats( hfBitMsg & msg );

	virtual void					ReceivedFullSnap();

	virtual void					LeaveGameLobby();
	
	virtual void					PrePickNewHost( hfLobby & lobby, bool forceMe, bool inviteOldHost );
	virtual bool					PreMigrateInvite( hfLobby & lobby );

	virtual void					HandleOobVoiceAudio( const lobbyAddress_t & from, const hfBitMsg & msg ) { sessionLocal->HandleOobVoiceAudio( from, msg ); }

	virtual void					ConnectAndMoveToLobby( hfLobby::lobbyType_t destLobbyType, const lobbyConnectInfo_t & connectInfo, bool waitForPartyOk );

	virtual hfVoiceChatMgr *		GetVoiceChat() { return sessionLocal->voiceChat; }

	virtual void					HandleServerQueryRequest( lobbyAddress_t & remoteAddr, hfBitMsg & msg, int msgType );
	virtual void 					HandleServerQueryAck( lobbyAddress_t & remoteAddr, hfBitMsg & msg );

	virtual void 					HandlePeerMatchParamUpdate( int peer, int msg );

	virtual hfLobbyBackend *		CreateLobbyBackend( const hfMatchParameters & p, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType );
	virtual hfLobbyBackend *		FindLobbyBackend( const hfMatchParameters & p, int numPartyUsers, float skillLevel, hfLobbyBackend::lobbyBackendType_t lobbyType );
	virtual hfLobbyBackend *		JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo , hfLobbyBackend::lobbyBackendType_t lobbyType );
	virtual void					DestroyLobbyBackend( hfLobbyBackend * lobby );

	hfSessionLocal * sessionLocal;
};


