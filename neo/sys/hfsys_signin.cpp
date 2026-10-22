#pragma hdrstop
#include "../hflib/hfprecompiled.h"

hfCVar com_requireNonProductionSignIn( "com_requireNonProductionSignIn", "1", CVAR_BOOL|CVAR_ARCHIVE, "If true, will require sign in, even on non production builds." );
extern hfCVar fs_savepath;

extern hfCVar g_demoMode;

/*
========================
hfSignInManagerBase::ProcessInputEvent
========================
*/
bool hfSignInManagerBase::ProcessInputEvent( const sysEvent_t * ev ) {
	// If we could use more local users, poll for them
	if ( GetNumLocalUsers() < maxDesiredLocalUsers && !IsAnyDeviceBeingRegistered() ) {
		if ( ev->IsKeyDown() ) {
			if ( ev->GetKey() == K_JOY1 || ev->GetKey() == K_JOY9 || ev->GetKey() == K_ENTER || ev->GetKey() == K_KP_ENTER ) {


				// Register a local user so they can use this input device only done for demos press start screen
				// otherwise the user is registered when selecting which game to play.
			}
		}
	}
		
	return false;
}

/*
========================
hfSignInManagerBase::GetDefaultProfile
========================
*/
hfPlayerProfile * hfSignInManagerBase::GetDefaultProfile() {
	if ( defaultProfile == NULL ) {
		// Create a new profile
		defaultProfile = hfPlayerProfile::CreatePlayerProfile( 0 );
	}
	return defaultProfile;
}

/*
========================
hfSignInManagerBase::GetLocalUserByInputDevice
========================
*/
hfLocalUser * hfSignInManagerBase::GetLocalUserByInputDevice( int index ) {
	for ( int i = 0; i < GetNumLocalUsers(); i++ ) {
		if ( GetLocalUserByIndex( i )->GetInputDevice() == index ) {
			return GetLocalUserByIndex( i );	// Found it
		}
	}

	return NULL;		// Not found
}

/*
========================
hfSignInManagerBase::GetLocalUserByHandle
========================
*/
hfLocalUser * hfSignInManagerBase::GetLocalUserByHandle( localUserHandle_t handle ) {
	for ( int i = 0; i < GetNumLocalUsers(); i++ ) {
		if ( GetLocalUserByIndex( i )->GetLocalUserHandle() == handle ) {
			return GetLocalUserByIndex( i );	// Found it
		}
	}

	return NULL;		// Not found
}

/*
========================
hfSignInManagerBase::GetPlayerProfileByInputDevice
========================
*/
hfPlayerProfile * hfSignInManagerBase::GetPlayerProfileByInputDevice( int index ) {
	hfLocalUser * user = session->GetSignInManager().GetLocalUserByInputDevice( index );
	hfPlayerProfile * profile = NULL;
	if ( user != NULL ) {
		profile = user->GetProfile();	
	}
	return profile;
}

/*
========================
hfSignInManagerBase::RemoveLocalUserByInputDevice
========================
*/
bool hfSignInManagerBase::RemoveLocalUserByInputDevice( int index ) {
	for ( int i = 0; i < GetNumLocalUsers(); i++ ) {
		if ( GetLocalUserByIndex( i )->GetInputDevice() == index ) {
			RemoveLocalUserByIndex( i );
			return true;
		}
	}

	return false;		// Not found
}

/*
========================
hfSignInManagerBase::RemoveLocalUserByHandle
========================
*/
bool hfSignInManagerBase::RemoveLocalUserByHandle( localUserHandle_t handle ) {
	for ( int i = 0; i < GetNumLocalUsers(); i++ ) {
		if ( GetLocalUserByIndex( i )->GetLocalUserHandle() == handle ) {
			RemoveLocalUserByIndex( i );
			return true;
		}
	}

	return false;		// Not found
}

/*
========================
hfSignInManagerBase::SaveUserProfiles
========================
*/
void hfSignInManagerBase::SaveUserProfiles() {
	for ( int i = 0; i < GetNumLocalUsers(); i++ ) {
		hfLocalUser * localUser = GetLocalUserByIndex( i );
		if ( localUser != NULL ) {
			hfPlayerProfile * profile = localUser->GetProfile();
			if ( profile != NULL ) {
				profile->SaveSettings( false );
			}
		}
	}
}

/*
========================
hfSignInManagerBase::RemoveAllLocalUsers
========================
*/
void hfSignInManagerBase::RemoveAllLocalUsers() {
	while ( GetNumLocalUsers() > 0 ) {
		RemoveLocalUserByIndex( 0 );
	}
}

/*
========================
hfSignInManagerBase::ValidateLocalUsers
========================
*/
void hfSignInManagerBase::ValidateLocalUsers( bool requireOnline ) {
	if ( !RequirePersistentMaster() ) {
		return;
	}
	for ( int i = GetNumLocalUsers() - 1; i >= 0; i-- ) {
		hfLocalUser * localUser = GetLocalUserByIndex( i );

		// If this user does not have a profile, remove them.
		// If this user is not online-capable and we require online, remove them.
		if ( !localUser->IsProfileReady() ||
			( requireOnline && ( !localUser->IsOnline() || !localUser->CanPlayOnline() ) ) ) {
			RemoveLocalUserByIndex( i );
		}
	}
}

/*
========================
hfSignInManagerBase::RequirePersistentMaster
========================
*/
bool hfSignInManagerBase::RequirePersistentMaster() {
#ifdef HF_RETAIL
	return true;		// ALWAYS require persistent master on retail builds
#else
	// Non retail production builds require a persistent profile unless si_splitscreen is set
	extern hfCVar si_splitscreen;

	// If we are forcing splitscreen, then we won't require a profile (this is for development)
	if ( si_splitscreen.GetInteger() != 0 ) {
		return false;
	}

	return com_requireNonProductionSignIn.GetBool();
#endif
}

/*
========================
hfSignInManagerBase::GetUniqueLocalUserHandle
Uniquely generate a handle based on name and time
========================
*/
localUserHandle_t hfSignInManagerBase::GetUniqueLocalUserHandle( const char * name ) { 
	MD5_CTX			ctx;
	unsigned char	digest[16];
	int64			clockTicks = Sys_GetClockTicks();

	MD5_Init( &ctx );
	MD5_Update( &ctx, (const unsigned char *)name, hfStr::Length( name ) );
	MD5_Update( &ctx, (const unsigned char *)&clockTicks, sizeof( clockTicks ) );
	MD5_Final( &ctx, (unsigned char *)digest );

	// Quantize the 128 bit hash down to the number of bits needed for a localUserHandle_t
	const int STRIDE_BYTES	= sizeof( localUserHandle_t::userHandleType_t );
	const int NUM_LOOPS		= 16 / STRIDE_BYTES;

	localUserHandle_t::userHandleType_t handle = 0;
	
	for ( int i = 0; i < NUM_LOOPS; i++ ) {
		localUserHandle_t::userHandleType_t tempHandle = 0;

		for ( int j = 0; j < STRIDE_BYTES; j++ ) {
			tempHandle |= ( ( localUserHandle_t::userHandleType_t )digest[ ( i * STRIDE_BYTES ) + j ] ) << ( j * 8 );
		}

		handle ^= tempHandle;
	}

	return localUserHandle_t( handle ); 
}

