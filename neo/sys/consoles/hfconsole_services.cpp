#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "hfconsole_services.h"
#include "../../framework/hfPlayerProfile.h"

/*
================================================
hfLocalUserConsole::Init
================================================
*/
void hfLocalUserConsole::Init( int inputDevice_, const char * gamerTag_, int numLocalUsers ) {
	if ( numLocalUsers == 1 ) {		// Check for 1, since this is now incremented before we get in here
		// This is the master user
		gamerTag = gamerTag_;
	} else {
		// Generate a name based off the master user for split-screen users
		gamerTag.Format( "%s (%i)", gamerTag_, numLocalUsers );
	}

	inputDevice = inputDevice_;
}

/*
================================================
hfSignInManagerConsole::Pump
================================================
*/
void hfSignInManagerConsole::Pump() {
	hfconsole::hfConsoleServices * services = hfconsole::HfConsole_GetServices();
	services->Pump();

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
		localUsers[ i ].Pump();
	}
}

/*
================================================
hfSignInManagerConsole::RemoveLocalUserByIndex
================================================
*/
void hfSignInManagerConsole::RemoveLocalUserByIndex( int index ) {
	if ( index < 0 || index >= localUsers.Num() ) {
		return;
	}

	if ( session != NULL ) {
		session->OnLocalUserSignout( &localUsers[ index ] );
	}
	localUsers.RemoveIndex( index );
}

/*
================================================
hfSignInManagerConsole::RegisterLocalUser
================================================
*/
void hfSignInManagerConsole::RegisterLocalUser( int inputDevice ) {
	if ( GetLocalUserByInputDevice( inputDevice ) != NULL ) {
		return;
	}

	hfLocalUserConsole * localUser = localUsers.Alloc();
	if ( localUser == NULL ) {
		return;
	}

	hfconsole::hfConsoleServices * services = hfconsole::HfConsole_GetServices();

	localUser->Init( inputDevice, services->GetGamerTag( inputDevice ), localUsers.Num() );
	localUser->SetInputDevice( inputDevice );
	localUser->SetLocalUserHandle( GetUniqueLocalUserHandle( localUser->GetGamerTag() ) );

	if ( session != NULL ) {
		session->OnLocalUserSignin( localUser );
	}
}

/*
================================================
hfAchievementSystemConsole
================================================
*/
void hfAchievementSystemConsole::AchievementUnlock( hfLocalUser * user, const int achievementID ) {
	if ( user == NULL || achievementID < 0 || achievementID >= hfAchievementSystem::MAX_ACHIEVEMENTS ) {
		return;
	}

	hfconsole::hfConsoleServices * services = hfconsole::HfConsole_GetServices();
	services->UnlockAchievement( services->GetUserId( user->GetInputDevice() ), achievementID );

	// Mirror the unlock into the local profile so the profile keeps the achievement bits
	hfPlayerProfile * profile = user->GetProfile();
	if ( profile != NULL ) {
		profile->SetAchievement( achievementID );
	}
}

void hfAchievementSystemConsole::AchievementLock( hfLocalUser * user, const int achievementID ) {
	if ( user == NULL || achievementID < 0 || achievementID >= hfAchievementSystem::MAX_ACHIEVEMENTS ) {
		return;
	}

	hfPlayerProfile * profile = user->GetProfile();
	if ( profile != NULL ) {
		profile->ClearAchievement( achievementID );
	}
}

void hfAchievementSystemConsole::AchievementLockAll( hfLocalUser * user, const int maxId ) {
	if ( user == NULL ) {
		return;
	}

	hfPlayerProfile * profile = user->GetProfile();
	if ( profile == NULL ) {
		return;
	}

	for ( int i = 0; i < maxId && i < hfAchievementSystem::MAX_ACHIEVEMENTS; i++ ) {
		profile->ClearAchievement( i );
	}
}

void hfAchievementSystemConsole::Pump() {
	hfconsole::HfConsole_GetServices()->Pump();
}

bool hfAchievementSystemConsole::GetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements ) const {
	if ( user == NULL ) {
		return false;
	}

	const hfPlayerProfile * profile = user->GetProfile();
	if ( profile == NULL ) {
		return false;
	}

	for ( int i = 0; i < hfAchievementSystem::MAX_ACHIEVEMENTS; i++ ) {
		achievements[ i ] = profile->GetAchievement( i );
	}
	return true;
}

bool hfAchievementSystemConsole::SetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements ) {
	if ( user == NULL ) {
		return false;
	}

	hfPlayerProfile * profile = user->GetProfile();
	if ( profile == NULL ) {
		return false;
	}

	for ( int i = 0; i < hfAchievementSystem::MAX_ACHIEVEMENTS; i++ ) {
		if ( achievements[ i ] ) {
			profile->SetAchievement( i );
		} else {
			profile->ClearAchievement( i );
		}
	}
	return true;
}
