#ifndef	__SYS_LOBBY_BACKEND_DIRECT_H__
#define	__SYS_LOBBY_BACKEND_DIRECT_H__

/*
========================
hfLobbyBackendDirect
========================
*/
class hfLobbyBackendDirect : public hfLobbyBackend {
public:
	hfLobbyBackendDirect();

	// hfLobbyBackend interface
	virtual void				StartHosting( const hfMatchParameters & p, float skillLevel, lobbyBackendType_t type );
	virtual void				StartFinding( const hfMatchParameters & p, int numPartyUsers, float skillLevel );
	virtual void				JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo );
	virtual void				GetSearchResults( hfList< lobbyConnectInfo_t > & searchResults );
	virtual void				FillMsgWithPostConnectInfo( hfBitMsg & msg ) {}
	virtual void				PostConnectFromMsg( hfBitMsg & msg ) {}
	virtual void				Shutdown();
	virtual void				GetOwnerAddress( lobbyAddress_t & outAddr );
	virtual void				SetIsJoinable( bool joinable );	
	virtual lobbyConnectInfo_t	GetConnectInfo();
	virtual bool				IsOwnerOfConnectInfo( const lobbyConnectInfo_t & connectInfo ) const;
	virtual void				Pump();
	virtual void				UpdateMatchParms( const hfMatchParameters & p );
	virtual void				UpdateLobbySkill( float lobbySkill );
	virtual void				SetInGame( bool value );
	virtual lobbyBackendState_t	GetState() { return state; }

	virtual void				BecomeHost( int numInvites );
	virtual void				FinishBecomeHost();

	virtual void			RegisterUser( lobbyUser_t * user, bool isLocal );
	virtual void			UnregisterUser( lobbyUser_t * user, bool isLocal );	

private:

	lobbyBackendState_t		state;
	netadr_t				address;
};

#endif	// __SYS_LOBBY_BACKEND_DIRECT_H__ 

