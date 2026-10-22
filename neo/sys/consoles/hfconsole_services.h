#ifndef __HF_ENGINE_CONSOLE_SERVICES_H__
#define __HF_ENGINE_CONSOLE_SERVICES_H__

/*
================================================
hfconsole_services.h

Console (game console) service backends.

These classes are the engine side adapters that bind the
platform neutral console service contract declared in
hflib/console/hfConsoleServices.h to the engine session
services (sign in, achievements, voice chat).

They are intentionally free of any vendor SDK dependency:
the platform specific SDK calls live behind the hflib
service interface, so the same engine code compiles for
a desktop build (offline stub) and for a real console
target (SDK backed implementation installed at boot).
================================================
*/

#include "../../hflib/hfprecompiled.h"
#include "../../hflib/console/hfConsoleServices.h"
#include "../hfsys_localuser.h"
#include "../hfsys_signin.h"
#include "../hfsys_achievements.h"
#include "../hfsys_voicechat.h"

/*
================================================
hfLocalUserConsole
================================================
*/
class hfLocalUserConsole : public hfLocalUser {
public:
	hfLocalUserConsole() : inputDevice( 0 ) {}

	void					Init( int inputDevice_, const char * gamerTag_, int numLocalUsers );

	// hfLocalUser interface
	virtual void			PumpPlatform() {}
	virtual bool			IsProfileReady() const { return true; }
	virtual bool			IsOnline() const { return true; }
	virtual uint32			GetOnlineCaps() const { return ( IsPersistent() && IsOnline() ) ? ( CAP_IS_ONLINE | CAP_CAN_PLAY_ONLINE ) : 0; }
	virtual int				GetInputDevice() const { return inputDevice; }
	virtual const char *	GetGamerTag() const { return gamerTag.c_str(); }
	virtual bool			IsInParty() const { return false; }
	virtual int				GetPartyCount() const { return 1; }

	void					SetInputDevice( int inputDevice_ ) { inputDevice = inputDevice_; }

private:
	hfStrStatic< 64 >		gamerTag;
	int						inputDevice;
};

/*
================================================
hfSignInManagerConsole
================================================
*/
class hfSignInManagerConsole : public hfSignInManagerBase {
public:
	virtual ~hfSignInManagerConsole() {}

	// hfSignInManagerBase interface
	virtual void					Pump();
	virtual int						GetNumLocalUsers() const { return localUsers.Num(); }
	virtual hfLocalUser *			GetLocalUserByIndex( int index ) { return &localUsers[ index ]; }
	virtual const hfLocalUser *		GetLocalUserByIndex( int index ) const { return &localUsers[ index ]; }
	virtual void					RemoveLocalUserByIndex( int index );
	virtual void					RegisterLocalUser( int inputDevice );

private:
	hfStaticList< hfLocalUserConsole, MAX_INPUT_DEVICES >	localUsers;
};

/*
================================================
hfAchievementSystemConsole
================================================
*/
class hfAchievementSystemConsole : public hfAchievementSystem {
public:
	virtual bool	IsInitialized() { return hfconsole::HfConsole_GetServices()->IsInitialized(); }
	virtual void	AchievementUnlock( hfLocalUser * user, const int achievementID );
	virtual void	AchievementLock( hfLocalUser * user, const int achievementID );
	virtual void	AchievementLockAll( hfLocalUser * user, const int maxId );
	virtual void	Pump();
	virtual bool	GetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements ) const;
	virtual bool	SetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements );
};

/*
================================================
hfVoiceChatMgrConsole
================================================
*/
class hfVoiceChatMgrConsole : public hfVoiceChatMgr {
public:
	virtual bool	GetLocalChatDataInternal( int talkerIndex, byte * data, int & dataSize ) { return false; }
	virtual void	SubmitIncomingChatDataInternal( int talkerIndex, const byte * data, int dataSize ) {}
	virtual bool	TalkerHasData( int talkerIndex ) { return false; }
	virtual bool	RegisterTalkerInternal( int index ) { return true; }
	virtual void	UnregisterTalkerInternal( int index ) {}
};

#endif // __HF_ENGINE_CONSOLE_SERVICES_H__
