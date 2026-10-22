#pragma hdrstop
#include "../../hflib/hfprecompiled.h"

#include "hfwin_achievements.h"
#include "../hfsys_session_local.h"

extern hfCVar achievements_Verbose;

#define STEAM_ACHIEVEMENT_PREFIX		"ach_"

/*
========================
hfAchievementSystemWin::hfAchievementSystemWin
========================
*/
hfAchievementSystemWin::hfAchievementSystemWin() {
}

/*
========================
hfAchievementSystemWin::IsInitialized
========================
*/
bool hfAchievementSystemWin::IsInitialized() {
	return false;
}

/*
================================
hfAchievementSystemWin::AchievementUnlock
================================
*/
void hfAchievementSystemWin::AchievementUnlock( hfLocalUser * user, int achievementID ) {
}

/*
========================
hfAchievementSystemWin::AchievementLock
========================
*/
void hfAchievementSystemWin::AchievementLock( hfLocalUser * user, const int achievementID ) {
}

/*
========================
hfAchievementSystemWin::AchievementLockAll
========================
*/
void hfAchievementSystemWin::AchievementLockAll( hfLocalUser * user, const int maxId ) {
}

/*
========================
hfAchievementSystemWin::GetAchievementDescription
========================
*/
bool hfAchievementSystemWin::GetAchievementDescription( hfLocalUser * user, const int achievementID, achievementDescription_t & data ) const {
	return false;
}

/*
========================
hfAchievementSystemWin::GetAchievementState
========================
*/
bool hfAchievementSystemWin::GetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements ) const {
	return false;
}

/*
================================
hfAchievementSystemWin::Pump
================================
*/
void hfAchievementSystemWin::Pump() {
}

