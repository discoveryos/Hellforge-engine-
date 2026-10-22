#pragma hdrstop
#include "../hflib/hfprecompiled.h"

extern hfCVar fs_savepath;

/*
========================
hfLocalUser::hfLocalUser
========================
*/
hfLocalUser::hfLocalUser() {
	memset( joiningLobby, 0, sizeof( joiningLobby ) );
	profileMgr.Init( this );
	syncAchievementsRequested = 0;
}

void hfLocalUser::Pump() {
	// Pump the profile
	GetProfileMgr().Pump();

	if ( GetProfileMgr().GetProfile() != NULL && GetProfileMgr().GetProfile()->GetState() == hfPlayerProfile::IDLE ) {
		// Pump achievements
		if ( syncAchievementsRequested ) {
			if ( session->GetAchievementSystem().IsInitialized() ) {
				session->GetAchievementSystem().SyncAchievementBits( this );
				syncAchievementsRequested = false;
			}
		}
		session->GetAchievementSystem().Pump();
	}

	// Extra platform pump if necessary
	PumpPlatform();
}

/*
========================
hfLocalUser::IsStorageDeviceAvailable
========================
*/
bool hfLocalUser::IsStorageDeviceAvailable() const {
	return saveGame_enable.GetBool();
}

/*
========================
hfLocalUser::ResetStorageDevice
========================
*/
void hfLocalUser::ResetStorageDevice() {
}

/*
========================
hfLocalUser::StorageSizeAvailable
========================
*/
bool hfLocalUser::StorageSizeAvailable( uint64 minSizeInBytes, int64 & neededBytes ) {
	int64 size = Sys_GetDriveFreeSpaceInBytes( fs_savepath.GetString() );

	neededBytes = minSizeInBytes - size;
	if ( neededBytes < 0 ) {
		neededBytes = 0;
	}

	return neededBytes == 0;
}

/*
========================
hfLocalUser::SetStatInt
========================
*/
void hfLocalUser::SetStatInt( int s, int v ) {
	hfPlayerProfile * profile = GetProfile();
	if ( profile != NULL ) {
		return profile->StatSetInt( s, v );
	}
}

/*
========================
hfLocalUser::SetStatFloat
========================
*/
void hfLocalUser::SetStatFloat( int s, float v ) {
	hfPlayerProfile * profile = GetProfile();
	if ( profile != NULL ) {
		return profile->StatSetFloat( s, v );
	}
}

/*
========================
hfLocalUser::GetStatInt
========================
*/
int	hfLocalUser::GetStatInt( int s ) { 
	const hfPlayerProfile * profile = GetProfile();

	if ( profile != NULL && s >= 0 ) {
		return profile->StatGetInt( s );
	}

	return 0; 
}

/*
========================
hfLocalUser::GetStatFloat
========================
*/
float hfLocalUser::GetStatFloat( int s ) {
	const hfPlayerProfile * profile = GetProfile();

	if ( profile != NULL ) {
		return profile->StatGetFloat( s );
	}

	return 0.0f;
}

/*
========================
hfLocalUser::LoadProfileSettings
========================
*/
void hfLocalUser::LoadProfileSettings() {
	hfPlayerProfile * profile = GetProfileMgr().GetProfile();

	// Lazy instantiation
	if ( profile == NULL ) {
		// Create a new profile
		profile = hfPlayerProfile::CreatePlayerProfile( GetInputDevice() );
	}

	if ( profile != NULL ) {
		profile->LoadSettings();
	}

	return;
}

/*
========================
hfLocalUser::SaveProfileSettings
========================
*/
void hfLocalUser::SaveProfileSettings() {
	hfPlayerProfile * profile = GetProfileMgr().GetProfile();
	if ( profile != NULL ) {
		profile->SaveSettings( true );
	}

	return;
}

/*
========================
localUserHandle_t::Serialize
========================
*/
void localUserHandle_t::Serialize( hfSerializer & ser ) {
	ser.Serialize( handle );
}

