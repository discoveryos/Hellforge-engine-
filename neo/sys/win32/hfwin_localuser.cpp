#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "hfwin_localuser.h"

extern hfCVar win_userPersistent;
extern hfCVar win_userOnline;
extern hfCVar win_isInParty;
extern hfCVar win_partyCount;

/*
========================
hfLocalUserWin::Init
========================
*/
void hfLocalUserWin::Init( int inputDevice_, const char * gamertag_, int numLocalUsers ) {
	if ( numLocalUsers == 1 ) {  // Check for 1, since this is now incremented before we get in here
		// This is the master user
		gamertag = gamertag_;
	} else {
		// On steam, we need to generate a name based off the master user for split-screen users.
		// We use the number of users on the system to generate the name rather than the device
		// number so that it is always consistently "username (2)" for the second player.
		gamertag.Format( "%s (%i)", gamertag_, numLocalUsers );
	}

	inputDevice = inputDevice_;
}

/*
========================
hfLocalUserWin::IsProfileReady
========================
*/
bool hfLocalUserWin::IsProfileReady() const {
#ifdef _DEBUG
	return win_userPersistent.GetBool();
#else
	return true;
#endif
}

/*
========================
hfLocalUserWin::IsOnline
========================
*/
bool hfLocalUserWin::IsOnline() const {
#ifdef _DEBUG
	return win_userOnline.GetBool();
#else
	return true;
#endif
}

/*
========================
hfLocalUserWin::IsInParty
========================
*/
bool hfLocalUserWin::IsInParty() const {
#ifdef _DEBUG
	return win_isInParty.GetBool();
#else
	return false;
#endif
}

/*
========================
hfLocalUserWin::GetPartyCount
========================
*/
int hfLocalUserWin::GetPartyCount() const {
	// TODO: Implement
#ifdef _DEBUG
	return win_partyCount.GetInteger();
#else
	return 0;
#endif
}

/*
========================
hfLocalUserWin::VerifyUserState
========================
*/
bool hfLocalUserWin::VerifyUserState( winUserState_t & state ) {

	if ( state.inputDevice != inputDevice ) {
		return false;
	}
	return true;
}

