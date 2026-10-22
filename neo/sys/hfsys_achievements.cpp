#pragma hdrstop
#include "../hflib/hfprecompiled.h"

/*
========================
hfAchievementSystem::SyncAchievementBits
========================
*/
void hfAchievementSystem::SyncAchievementBits( hfLocalUser * user ) {
	if ( user != NULL ) {
		hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > achievements;

		if ( GetAchievementState( user, achievements ) ) {
			for ( int i = 0; i < achievements.Num(); i++ ) {
				if ( achievements[i] ) {
					user->GetProfile()->SetAchievement( i );
				}
			}
		}
	}
}

