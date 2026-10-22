#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "../../framework/hfPlayerProfile.h"
#include "../hfsys_session_local.h"
#include "hfwin_signin.h"

#ifdef _DEBUG
hfCVar win_userPersistent( "win_userPersistent", "1", CVAR_BOOL, "debugging cvar for profile persistence status" );
hfCVar win_userOnline( "win_userOnline", "1", CVAR_BOOL, "debugging cvar for profile online status" );
hfCVar win_isInParty( "win_isInParty", "0", CVAR_BOOL, "debugging cvar for platform party status" );
hfCVar win_partyCount( "win_partyCount", "0", CVAR_INTEGER, "debugginc var for platform party count" );
#endif

/*
========================
hfSignInManagerWin::Shutdown
========================
*/
void hfSignInManagerWin::Shutdown() {
}

/*
========================
hfSignInManagerWin::Pump
========================
*/
void hfSignInManagerWin::Pump() {
	
	// If we have more users than we need, then set to the lower amount
	// (don't remove the master user though)
	if ( localUsers.Num() > 1 && localUsers.Num() > maxDesiredLocalUsers ) {
		localUsers.SetNum( maxDesiredLocalUsers );
	}
	
#ifndef HF_RETAIL
	// If we don't have enough, then make sure we do
	// NOTE - We always want at least one user on windows for now, 
	// and this master user will always use controller 0
	while ( localUsers.Num() < minDesiredLocalUsers ) {
		RegisterLocalUser( localUsers.Num() );
	}
#endif
	
	// See if we need to save settings on any of the profiles
	for ( int i = 0; i < localUsers.Num(); i++ ) {
		localUsers[i].Pump();
	}
}

/*
========================
hfSignInManagerWin::RemoveLocalUserByIndex
========================
*/
void hfSignInManagerWin::RemoveLocalUserByIndex( int index ) {
	session->OnLocalUserSignout( &localUsers[index] );
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
	
	static char machineName[128];
	DWORD len = 128;
	::GetComputerName( machineName, &len );

	const char * nameSource = machineName;

	hfStr name( nameSource );
	int nameLength = name.Length();
	if ( hfStr::IsValidUTF8( nameSource, nameLength ) ) {
		int nameIndex = 0;
		int numChars = 0;
		name.Empty();
		while ( nameIndex < nameLength && numChars++ < hfLocalUserWin::MAX_GAMERTAG_CHARS ) {
			uint32 c = hfStr::UTF8Char( nameSource, nameIndex );
			name.AppendUTF8Char( c );
		}
	}
	
	hfLocalUserWin & localUser = *localUsers.Alloc();
	
	localUser.Init( inputDevice, name.c_str(), localUsers.Num() );
	localUser.SetLocalUserHandle( GetUniqueLocalUserHandle( localUser.GetGamerTag() ) );

	session->OnLocalUserSignin( &localUser );
}

/*
========================
hfSignInManagerWin::CreateNewUser
========================
*/
bool hfSignInManagerWin::CreateNewUser( winUserState_t & state ) {
	//hfScopedGlobalHeap	everythingHereGoesInTheGlobalHeap;	// users obviously persist across maps

	RemoveAllLocalUsers();
	RegisterLocalUser( state.inputDevice );

	if ( localUsers.Num() > 0 ) {
		if ( !localUsers[0].VerifyUserState( state ) ) {
			RemoveAllLocalUsers();
		}
	}

	return true;
}

CONSOLE_COMMAND( testRemoveAllLocalUsers, "Forces removal of local users - mainly for PC testing", NULL ) {
	session->GetSignInManager().RemoveAllLocalUsers();
}

