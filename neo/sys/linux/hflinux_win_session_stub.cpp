/*
===========================================================================

Hellforge Power 1 Engine - Linux port

The platform session code ( hfsys_session_local.h / hfsys_session_local.cpp )
is written against the Windows sign-in classes ( hfSignInManagerWin /
hfLocalUserWin ) whose non-inline virtual methods would otherwise be
undefined at link time on non-Windows builds.

At runtime the Linux sign-in implementations ( hfSignInManagerLinux /
hfLocalUserLinux, defined in hflinux_session.cpp ) are used.  These stubs
mirror the Linux runtime behavior purely to satisfy the vtable
requirements for the Windows classes referenced by the shared code.

===========================================================================
*/

#include "../../hflib/hfprecompiled.h"
#include "../hfsys_session_local.h"

/*
========================
hfLocalUserWin::IsProfileReady
========================
*/
bool hfLocalUserWin::IsProfileReady() const {
	return true;
}

/*
========================
hfLocalUserWin::IsOnline
========================
*/
bool hfLocalUserWin::IsOnline() const {
	return true;
}

/*
========================
hfLocalUserWin::IsInParty
========================
*/
bool hfLocalUserWin::IsInParty() const {
	return false;
}

/*
========================
hfLocalUserWin::GetPartyCount
========================
*/
int hfLocalUserWin::GetPartyCount() const {
	return 1;
}

/*
========================
hfLocalUserWin::VerifyUserState
========================
*/
bool hfLocalUserWin::VerifyUserState( winUserState_t & state ) {
	return state.inputDevice == inputDevice;
}

/*
========================
hfLocalUserWin::Init
========================
*/
void hfLocalUserWin::Init( int inputDevice_, const char * gamertag_, int numLocalUsers ) {
	if ( numLocalUsers == 1 ) {
		SetGamerTag( gamertag_ );
	} else {
		hfStr tmp;
		tmp.Format( "%s (%i)", gamertag_, numLocalUsers );
		SetGamerTag( tmp.c_str() );
	}
	inputDevice = inputDevice_;
}

/*
========================
hfSignInManagerWin::Pump
========================
*/
void hfSignInManagerWin::Pump() {
	for ( int i = 0; i < localUsers.Num(); i++ ) {
		localUsers[i].Pump();
	}
}

/*
========================
hfSignInManagerWin::Shutdown
========================
*/
void hfSignInManagerWin::Shutdown() {
	if ( localUsers.Num() > 1 ) {
		localUsers.SetNum( 1 );
	}
}

/*
========================
hfSignInManagerWin::RemoveLocalUserByIndex
========================
*/
void hfSignInManagerWin::RemoveLocalUserByIndex( int index ) {
	if ( session != NULL ) {
		session->OnLocalUserSignout( &localUsers[index] );
	}
	localUsers.RemoveIndex( index );
}

/*
========================
hfSignInManagerWin::RegisterLocalUser
========================
*/
void hfSignInManagerWin::RegisterLocalUser( int inputDevice ) {
	if ( GetLocalUserByInputDevice( inputDevice ) != NULL ) {
		return;
	}

	hfLocalUserWin & localUser = *localUsers.Alloc();

	localUser.Init( inputDevice, "localplayer", localUsers.Num() );
	localUser.SetInputDevice( inputDevice );
	localUser.SetLocalUserHandle( GetUniqueLocalUserHandle( localUser.GetGamerTag() ) );

	if ( session != NULL ) {
		session->OnLocalUserSignin( &localUser );
	}
}

/*
========================
hfSignInManagerWin::CreateNewUser
========================
*/
bool hfSignInManagerWin::CreateNewUser( winUserState_t & state ) {
	RegisterLocalUser( state.inputDevice );
	return true;
}
