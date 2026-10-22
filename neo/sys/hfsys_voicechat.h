#ifndef	__SYS_VOICECHATMGR_H__
#define	__SYS_VOICECHATMGR_H__

#include "hfsys_lobby_backend.h"

/*
================================================
hfVoiceChatMgr
================================================
*/
class hfVoiceChatMgr {
public:
	hfVoiceChatMgr() : activeLobbyType( -1 ), activeGroupIndex( 0 ), sendFrame( 0 ), disableVoiceReasons( 0 ), sendGlobal( false )  {}
	
	virtual void	Init( void * pXAudio2 );
	virtual void	Shutdown();

	void			RegisterTalker( lobbyUser_t * user, int lobbyType, bool isLocal  );
	void			UnregisterTalker( lobbyUser_t * user, int lobbyType, bool isLocal );
	void			GetActiveLocalTalkers( hfStaticList< int, MAX_PLAYERS > & localTalkers );
	void			GetRecipientsForTalker( int talkerIndex, hfStaticList< const lobbyAddress_t *, MAX_PLAYERS > & recipients );
	
	void			SetTalkerGroup( const lobbyUser_t * user, int lobbyType, int groupIndex );

	void			SetActiveLobby( int lobbyType );
	void			SetActiveChatGroup( int groupIndex );
	int				FindTalkerByUserId( lobbyUserID_t lobbyUserID, int lobbyType );
	bool			GetLocalChatData( int talkerIndex, byte * data, int & dataSize );
	void			SubmitIncomingChatData( const byte * data, int dataSize );
	voiceState_t	GetVoiceState( const lobbyUser_t * user );
	bool			CanSendVoiceTo( int talkerFromIndex, int talkerToIndex );
	bool			IsRestrictedByPrivleges();

	void			SetHeadsetState( int talkerIndex, bool state );
	bool			GetHeadsetState( int talkerIndex ) const	{ return talkers[ talkerIndex ].hasHeadset; }
	bool			HasHeadsetStateChanged( int talkerIndex );

	enum disableVoiceReason_t {
		REASON_GENERIC				= BIT( 0 ),
		REASON_PRIVILEGES			= BIT( 1 ),
	};

	void			SetDisableVoiceReason( disableVoiceReason_t reason );
	void			ClearDisableVoiceReason( disableVoiceReason_t reason );

	virtual bool		GetLocalChatDataInternal( int talkerIndex, byte * data, int & dataSize ) = 0;
	virtual void		SubmitIncomingChatDataInternal( int talkerIndex, const byte * data, int dataSize ) = 0;
	virtual bool		TalkerHasData( int talkerIndex ) = 0;
	virtual void		Pump() {}
	virtual void		FlushBuffers() {}
	virtual void		ToggleMuteLocal( const lobbyUser_t * src, const lobbyUser_t * target );

protected:

	struct remoteMachine_t {
		int					lobbyType;
		lobbyAddress_t	address;
		int					refCount;
		int					sendFrame;
	};

	struct talker_t {
		talker_t() : 
			user( NULL ), 
			isLocal( false ), 
			lobbyType( -1 ), 
			groupIndex( -1 ), 
			registered( false ), 
			registeredSuccess( false ),
			machineIndex( -1 ),
			isMuted( false ),
			hasHeadset( true ),
			hasHeadsetChanged( false ),
			talking( false ),
			talkingGlobal( false ),
			talkingTime( 0 )
		{}

		lobbyUser_t *	user;
		bool			isLocal;
		int				lobbyType;
		int				groupIndex;
		bool			registered;			// True if this user is currently registered with the XHV engine
		bool			registeredSuccess;	// True if this user is currently successfully registered with the XHV engine
		int				machineIndex;		// Index into remote machines array (-1 if this is a local talker)
		bool			isMuted;			// This machine is not allowed to hear or talk to this player
		bool			hasHeadset;			// This user has a headset connected
		bool			hasHeadsetChanged;	// This user's headset state has changed
		bool			talking;
		bool			talkingGlobal;
		int				talkingTime;

		bool IsLocal() const { return isLocal; }
	};

	virtual bool	RegisterTalkerInternal( int index ) = 0;
	virtual void	UnregisterTalkerInternal( int index ) = 0;

	int		FindTalkerIndex( const lobbyUser_t * user, int lobbyType );
	int		FindMachine( const lobbyAddress_t & address, int lobbyType );
	int		AddMachine( const lobbyAddress_t & address, int lobbyType );
	void	RemoveMachine( int machineIndex, int lobbyType );
	void	UpdateRegisteredTalkers();
	
	hfStaticList< talker_t, MAX_PLAYERS * 2 >			talkers;			// * 2 to account for handling both session types
	hfStaticList< remoteMachine_t, MAX_PLAYERS * 2 >	remoteMachines;		// * 2 to account for handling both session types
	
	int						activeLobbyType;
	int						activeGroupIndex;
	int						sendFrame;
	uint32					disableVoiceReasons;
	bool					sendGlobal;
};


#endif	// __SYS_VOICECHATMGR_H__ 
