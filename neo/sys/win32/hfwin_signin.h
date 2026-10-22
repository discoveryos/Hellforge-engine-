#ifndef __WIN_SIGNIN_H__
#define __WIN_SIGNIN_H__

#include "hfwin_localuser.h"

/*
================================================
hfSignInManagerWin 
================================================
*/
class hfSignInManagerWin : public hfSignInManagerBase {
public:

			hfSignInManagerWin() : dlcVersionChecked( false ) {}
	virtual ~hfSignInManagerWin() {}

	//==========================================================================================
	// hfSignInManagerBase interface
	//==========================================================================================
	virtual void					Pump();
	virtual void					Shutdown();
	virtual int						GetNumLocalUsers() const { return localUsers.Num(); }
	virtual hfLocalUser *			GetLocalUserByIndex( int index ) { return &localUsers[index]; }
	virtual const hfLocalUser *		GetLocalUserByIndex( int index ) const { return &localUsers[index]; }
	virtual void					RemoveLocalUserByIndex( int index );
	virtual void					RegisterLocalUser( int inputDevice );		// Register a local user to the passed in controller

	bool							CreateNewUser( winUserState_t & state );

private:
	hfStaticList< hfLocalUserWin, MAX_INPUT_DEVICES >	localUsers;
	bool												dlcVersionChecked;
};

#endif

