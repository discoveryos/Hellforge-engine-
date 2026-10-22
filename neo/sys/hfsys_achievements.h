#ifndef __SYS_ACHIEVEMENTS_H__
#define __SYS_ACHIEVEMENTS_H__

class hfLocalUser;

// data structure for online achievement entry descriptions
// this is used for testing purposes to make sure that the consoles
// achievement settings match the game's decls
struct achievementDescription_t {
	void	Clear() {
		name[0] = '\0';
		description[0] = '\0';
		hidden = false;
	};
	char	name[500];
	char	description[1000];
	bool	hidden;
};

/*
================================================
hfAchievementSystem 
================================================
*/
class hfAchievementSystem {
public:
	static const int MAX_ACHIEVEMENTS = 128;		// This matches the max number of achievements bits in the profile

	virtual			~hfAchievementSystem() {}

	// PC and PS3 initialize for the system, not for a particular controller
	virtual void	Init() {}

	// PS3 has to wait to install the .TRP file until *after* we determine there's enough space, for consistent user messaging
	virtual void	Start() {}

	// Do any necessary cleanup
	virtual void	Shutdown() {}

	// Is the achievement system ready for requests
	virtual bool	IsInitialized() { return false; }
	
	// Add a local user to the system
	virtual void	RegisterLocalUser( hfLocalUser * user ) {}

	// This is only necessary on the 360 right now, we need this because the 360 maintains a buffer of pending actions
	// per user.  If a user is removed from the system, we need to inform the system so it can cancel it's in flight actions
	// and allow the buffers to be reused
	virtual void	RemoveLocalUser( hfLocalUser * user ) {}

	// Unlocks the achievement, all platforms silently fail if the achievement has already been unlocked
	virtual void	AchievementUnlock( hfLocalUser * user, const int achievementID ) = 0;

	// Puts the achievement back to its original state, platform implementation may not allow this
	virtual void	AchievementLock( hfLocalUser * user, const int achievementID ) {}

	// Puts alls achievements back to their original state, platform implementation may not allow this
	virtual void	AchievementLockAll( hfLocalUser * user, const int maxId ) {}

	// Should be done every frame
	virtual void	Pump() = 0;

	// Cancels all in-flight achievements for all users if NULL, resets the system so a Init() must be re-issued
	virtual void	Reset( hfLocalUser * user = NULL ) {}

	// Cancels all in-flight achievements, not very useful on PC
	virtual void	Cancel( hfLocalUser * user ) {}

	// Retrieves textual information about a given achievement
	// returns false if there was an error
	virtual bool	GetAchievementDescription( hfLocalUser * user, const int id, achievementDescription_t & data ) const { return false; }

	// How much storage is required
	// returns false if there was an error
	virtual bool	GetRequiredStorage( uint64 & requiredSizeTrophiesBytes ) { requiredSizeTrophiesBytes = 0; return true; }

	// Retrieves state about of all achievements cached locally (may not be online yet)
	// returns false if there was an error
	virtual bool	GetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements ) const { return false; }

	// Sets state of all the achievements within list (for debug purposes only)
	// returns false if there was an error
	virtual bool	SetAchievementState( hfLocalUser * user, hfArray< bool, hfAchievementSystem::MAX_ACHIEVEMENTS > & achievements ) { return false; }

	// You want to get the server's cached achievement status into the user because the profile may not have been
	// saved with the achievement bits after an achievement was granted.
	void			SyncAchievementBits( hfLocalUser * user );

protected:
	// Retrieves the index from the local user list
	int				GetLocalUserIndex( hfLocalUser * user ) const { return users.FindIndex( user ); }

	hfStaticList< hfLocalUser *, MAX_LOCAL_PLAYERS > users;
};

#endif // __SYS_ACHIEVEMENTS_H__

