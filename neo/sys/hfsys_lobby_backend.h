#ifndef	__SYS_LOBBY_BACKEND_H__
#define	__SYS_LOBBY_BACKEND_H__


extern hfCVar net_verboseResource;
#define NET_VERBOSERESOURCE_PRINT if ( net_verboseResource.GetBool() ) hfLib::Printf

extern hfCVar net_verbose;
#define NET_VERBOSE_PRINT if ( net_verbose.GetBool() ) hfLib::Printf

class lobbyAddress_t {
public:
	lobbyAddress_t();
	
	void InitFromNetadr( const netadr_t & netadr );

	void InitFromIPandPort( const char * ip, int port );

	const char * ToString() const;
	bool UsingRelay() const;
	bool Compare( const lobbyAddress_t & addr, bool ignoreSessionCheck = false ) const;
	void WriteToMsg( hfBitMsg & msg ) const;
	void ReadFromMsg( hfBitMsg & msg );

	// IP address
	netadr_t	netAddr;
};

struct lobbyConnectInfo_t {
public:
	void WriteToMsg( hfBitMsg & msg ) const {
		msg.WriteNetadr( netAddr );
	}
	void ReadFromMsg( hfBitMsg & msg ) {
		msg.ReadNetadr( &netAddr );
	}
	lobbyConnectInfo_t() : netAddr() { }

	netadr_t				netAddr;
};

class hfNetSessionPort {
public:
	hfNetSessionPort();

	bool InitPort( int portNumber, bool useBackend );
	bool ReadRawPacket( lobbyAddress_t & from, void * data, int & size, int maxSize  );
	void SendRawPacket( const lobbyAddress_t & to, const void * data, int size );

	bool IsOpen();
	void Close();
	
private:
	float	forcePacketDropCurr;	// Used with net_forceDrop and net_forceDropCorrelation
	float	forcePacketDropPrev;

	hfUDP	UDP;
};

struct lobbyUser_t {
	static const int INVALID_PING = 9999;
	// gamertags can be up to 16 4-byte characters + \0
	static const int MAX_GAMERTAG	= 64 + 1; 

	lobbyUser_t() {
		isBot				= false;
		peerIndex			= -1;
		disconnecting		= false;
		level				= 1;
		pingMs				= INVALID_PING;
		teamNumber			= 0;
		arbitrationAcked	= false;
		partyToken			= 0;

		selectedSkin		= 0;
		weaponAutoSwitch	= true;
		weaponAutoReload	= true;

		migrationGameData	= -1;
	}

	// Common variables
	bool				isBot;				// true if lobbyUser is a bot.
	int					peerIndex;			// peer number on host
	lobbyUserID_t		lobbyUserID;		// Locally generated to be unique, and internally keeps the local user handle
	char				gamertag[MAX_GAMERTAG];
	int					pingMs;				// round trip time in milliseconds
	
	bool				disconnecting;		// true if we've sent a msg to disconnect this user from the session
	int					level;
	int					teamNumber;
	uint32				partyToken;			// set by the server when people join as a party

	int					selectedSkin;
	bool				weaponAutoSwitch;
	bool				weaponAutoReload;

	bool				arbitrationAcked;	// if the user is verified for arbitration

	lobbyAddress_t		address;

	int					migrationGameData;	// index into the local migration gamedata array that is associated with this user. -1=no migration game data available

	// Platform variables
	
	bool IsDisconnected() const { return lobbyUserID.IsValid() ? false : true; }
	
	void WriteToMsg( hfBitMsg & msg ) {
		address.WriteToMsg( msg );
		lobbyUserID.WriteToMsg( msg );
		msg.WriteLong( peerIndex );
		msg.WriteShort( pingMs );
		msg.WriteLong( partyToken );
		msg.WriteString( gamertag, MAX_GAMERTAG, false );
		WriteClientMutableData( msg );
	}
	
	void ReadFromMsg( hfBitMsg & msg ) {
		address.ReadFromMsg( msg );
		lobbyUserID.ReadFromMsg( msg );
		peerIndex = msg.ReadLong();
		pingMs = msg.ReadShort();
		partyToken = msg.ReadLong();
		msg.ReadString( gamertag, MAX_GAMERTAG );
		ReadClientMutableData( msg );
	}

	bool UpdateClientMutableData( const hfLocalUser * localUser );

	void WriteClientMutableData( hfBitMsg & msg ) {
		msg.WriteBits( selectedSkin, 4 );
		msg.WriteBits( teamNumber, 2 );		// We need two bits since we use team value of 2 for spectating
		msg.WriteBool( weaponAutoSwitch );
		msg.WriteBool( weaponAutoReload );
		release_assert( msg.GetWriteBit() == 0 );
	}

	void ReadClientMutableData( hfBitMsg & msg ) {
		selectedSkin = msg.ReadBits( 4 );
		teamNumber = msg.ReadBits( 2 );		// We need two bits since we use team value of 2 for spectating
		weaponAutoSwitch = msg.ReadBool();
		weaponAutoReload = msg.ReadBool();
	}
};

/*
================================================
hfLobbyBackend
This class interfaces with the various back ends for the different platforms
================================================
*/
class hfLobbyBackend {
public:
	enum lobbyBackendState_t {
		STATE_INVALID			= 0,
		STATE_READY				= 1,
		STATE_CREATING			= 2,		// In the process of creating the lobby as a host
		STATE_SEARCHING			= 3,		// In the process of searching for a lobby to join
		STATE_OBTAINING_ADDRESS	= 4,		// In the process of obtaining the address of the lobby owner
		STATE_ARBITRATING		= 5,		// Arbitrating
		STATE_SHUTTING_DOWN		= 6,		// In the process of shutting down
		STATE_SHUTDOWN			= 7,		// Was a host or peer at one point, now ready to be deleted
		STATE_FAILED			= 8,		// Failure occurred
		NUM_STATES
	};

	static const char * GetStateString( lobbyBackendState_t state_ ) { 
		static const char * stateToString[NUM_STATES] = {
			"STATE_INVALID",
			"STATE_READY",
			"STATE_CREATING",
			"STATE_SEARCHING",
			"STATE_OBTAINING_ADDRESS",
			"STATE_ARBITRATING",
			"STATE_SHUTTING_DOWN",
			"STATE_SHUTDOWN",
			"STATE_FAILED"
		};

		return stateToString[ state_ ]; 
	}
	
	enum lobbyBackendType_t {
		TYPE_PARTY		= 0,
		TYPE_GAME		= 1,
		TYPE_GAME_STATE	= 2,
		TYPE_INVALID	= 0xff,
	};

	hfLobbyBackend() : type( TYPE_INVALID ), isHost( false ), isLocal( false ) {}
	hfLobbyBackend( lobbyBackendType_t lobbyType ) : type( lobbyType ), isHost( false ), isLocal( false ) {}

	virtual void			StartHosting( const hfMatchParameters & p, float skillLevel, lobbyBackendType_t type ) = 0;
	virtual void			StartFinding( const hfMatchParameters & p, int numPartyUsers, float skillLevel ) = 0;
	virtual void			JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo ) = 0;
	virtual void			GetSearchResults( hfList< lobbyConnectInfo_t > & searchResults ) = 0;
	virtual lobbyConnectInfo_t GetConnectInfo()	= 0;
	virtual void			FillMsgWithPostConnectInfo( hfBitMsg & msg ) = 0;				// Passed itno PostConnectFromMsg
	virtual void			PostConnectFromMsg( hfBitMsg & msg ) = 0;						// Uses results from FillMsgWithPostConnectInfo
	virtual bool			IsOwnerOfConnectInfo( const lobbyConnectInfo_t & connectInfo ) const { return false; }
	virtual void			Shutdown() = 0;
	virtual void			GetOwnerAddress( lobbyAddress_t & outAddr ) = 0;
	virtual bool			IsHost() { return isHost; }
	virtual void			SetIsJoinable( bool joinable ) {}
	virtual void			Pump() = 0;
	virtual void			UpdateMatchParms( const hfMatchParameters & p ) = 0;
	virtual void			UpdateLobbySkill( float lobbySkill ) = 0;
	virtual void			SetInGame( bool value ) {}
	
	virtual lobbyBackendState_t	GetState() = 0;
	virtual bool			IsLocal() const { return isLocal; }
	virtual bool			IsOnline() const { return !isLocal; }

	virtual bool			StartArbitration() { return false; }
	virtual void			Arbitrate() {}
	virtual void			VerifyArbitration() {}
	virtual bool			UserArbitrated( lobbyUser_t * user ) { return false; }

	virtual void			RegisterUser( lobbyUser_t * user, bool isLocal ) {}
	virtual void			UnregisterUser( lobbyUser_t * user, bool isLocal ) {}

	virtual void			StartSession() {}
	virtual void			EndSession() {}
	virtual bool			IsSessionStarted() { return false; }
	virtual void			FlushStats() {}

	virtual void			BecomeHost( int numInvites ) {}						// Become the host of this lobby
	virtual	void			RegisterAddress( lobbyAddress_t & address ) {}	// Called after becoming a new host, to register old addresses to send invites to
	virtual void			FinishBecomeHost() {}
	
	void					SetLobbyType( lobbyBackendType_t lobbyType ) { type = lobbyType; }
	lobbyBackendType_t		GetLobbyType() const { return type; }
	const char *			GetLobbyTypeString() const { return ( GetLobbyType() == TYPE_PARTY ) ? "Party" : "Game"; }

	bool					IsRanked() { return MatchTypeIsRanked( parms.matchFlags ); }
	bool					IsPrivate() { return MatchTypeIsPrivate( parms.matchFlags ); }

protected:
	lobbyBackendType_t		type;
	hfMatchParameters		parms;
	bool					isLocal;		// True if this lobby is restricted to local play only (won't need and can't connect to online lobbies)
	bool					isHost;			// True if we created this lobby
};

class hfLobbyToSessionCB {
public:
	virtual class hfLobbyBackend *				GetLobbyBackend( hfLobbyBackend::lobbyBackendType_t type ) const = 0;
	virtual bool								CanJoinLocalHost() const = 0;

	// Ugh, hate having to ifdef these, but we're doing some fairly platform specific callbacks
};

#endif	// __SYS_LOBBY_BACKEND_H__ 

