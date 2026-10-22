#ifndef __WIN_ACHIEVEMENTS_H__
#define __WIN_ACHIEVEMENTS_H__

/*
================================================
hfAchievementSystemWin 
================================================
*/
class hfAchievementSystemWin : public hfAchievementSystem {
public:
			hfAchievementSystemWin();

	bool	IsInitialized();
	void	AchievementUnlock( hfLocalUser * user, const int achievementID );
	void	AchievementLock( hfLocalUser * user, const int achievementID );
	void	AchievementLockAll( hfLocalUser * user, const int maxId );
	void	Pump();
	bool	GetAchievementDescription( hfLocalUser * user, const int id, achievementDescription_t & data ) const;
	bool	GetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements ) const;
};

#endif // __WIN_ACHIEVEMENTS_H__

