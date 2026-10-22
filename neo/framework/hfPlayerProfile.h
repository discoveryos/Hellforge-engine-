#ifndef __PLAYERPROFILE_H__
#define __PLAYERPROFILE_H__

#define	MAX_PROFILE_SIZE			( 1024 * 1000 ) // High number for the key bindings

/*
================================================
profileStatValue_t 
================================================
*/
union profileStatValue_t {
	int		i; 
	float	f;
};

/*
================================================
hfPlayerProfile

The general rule for using cvars for settings is that if you want the player's profile settings to affect the startup
of the game before there is a player associated with the game, use cvars.  Example: video & volume settings.
================================================
*/
class hfPlayerProfile {
	friend class hfLocalUser;
	friend class hfProfileMgr;

public: 
	// Only have room to squeeze ~450 archived cvars right now
	static const int MAX_PLAYER_PROFILE_STATS = 200;

	enum state_t {
		IDLE = 0,
		SAVING,
		LOADING,
		SAVE_REQUESTED,
		LOAD_REQUESTED,
		ERR
	};
protected:
					hfPlayerProfile(); // don't instantiate. we static_cast the child all over the place
public:

	virtual			~hfPlayerProfile();

	static hfPlayerProfile * CreatePlayerProfile( int deviceIndex );

	void			SetDefaults();
	bool			Serialize( hfSerializer & ser );

	const int		GetDeviceNumForProfile() const { return deviceNum; }
	void			SetDeviceNumForProfile( int num ) { deviceNum = num; }
	void			SaveSettings( bool forceDirty );
	void			LoadSettings();
	state_t			GetState() const { return state; }
	state_t			GetRequestedState() const { return requestedState; }
	bool			IsDirty() { return dirty; }

	bool			GetAchievement( const int id ) const;
	void			SetAchievement( const int id );
	void			ClearAchievement( const int id );

	int				GetDlcReleaseVersion() const { return dlcReleaseVersion; }
	void			SetDlcReleaseVersion( int version ) { dlcReleaseVersion = version; }

	int				GetLevel() const { return 0; }

	//------------------------
	// Config
	//------------------------
	int				GetConfig() const { return configSet; }
	void			SetConfig( int config, bool save );
	void			RestoreDefault();

	void			SetLeftyFlip( bool lf );
	bool			GetLeftyFlip() const { return leftyFlip; }

private:
	void			StatSetInt( int s, int v );
	void			StatSetFloat( int s, float v );
	int				StatGetInt( int s ) const;
	float			StatGetFloat( int s ) const;
	void			SetState( state_t value ) { state = value; }
	void			SetRequestedState( state_t value ) { requestedState = value; }
	void			MarkDirty( bool isDirty ) { dirty = isDirty; }

	void			ExecConfig( bool save = false, bool forceDefault = false );

protected:
	// Do not save:
	state_t			state;
	state_t			requestedState;
	int				deviceNum;

	// Save:
	uint64			achievementBits;
	uint64			achievementBits2;
	int				dlcReleaseVersion;
	int				configSet;
	bool			customConfig;
	bool			leftyFlip;

	bool			dirty;		// dirty bit to indicate whether or not we need to save

	hfStaticList< profileStatValue_t, MAX_PLAYER_PROFILE_STATS > stats;
};

#endif

