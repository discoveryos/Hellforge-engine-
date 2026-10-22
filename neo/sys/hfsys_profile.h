#ifndef __SYS_PROFILE_H__
#define __SYS_PROFILE_H__

#include "hfsys_savegame.h"
#include "hfsys_session_savegames.h"


class hfSaveGameProcessorSaveProfile;
class hfSaveGameProcessorLoadProfile;
class hfLocalUser;
class hfPlayerProfile;

/*
================================================
hfProfileMgr 
================================================
*/
class hfProfileMgr {
public:
						hfProfileMgr();
						~hfProfileMgr();

	// Called the first time it's asked to load
	void				Init( hfLocalUser * user );

	void 				Pump();
	hfPlayerProfile *	GetProfile();

private:
	void				LoadSettingsAsync();
	void				SaveSettingsAsync();
	
	void				OnLoadSettingsCompleted( hfSaveLoadParms * parms );
	void				OnSaveSettingsCompleted( hfSaveLoadParms * parms );

private:
	std::auto_ptr< hfSaveGameProcessorSaveProfile >	profileSaveProcessor;
	std::auto_ptr< hfSaveGameProcessorLoadProfile >	profileLoadProcessor;

	hfLocalUser *						user;					// reference passed in
	hfPlayerProfile *					profile;				
	saveGameHandle_t					handle;
};

/*
================================================
hfSaveGameProcessorSaveProfile
================================================
*/
class hfSaveGameProcessorSaveProfile : public hfSaveGameProcessorSaveFiles {
public:
	DEFINE_CLASS( hfSaveGameProcessorSaveProfile );
					
					hfSaveGameProcessorSaveProfile();

	bool			InitSaveProfile( hfPlayerProfile * profile, const char * folder );
	virtual bool	Process();

private:
	hfFile_SaveGame *	profileFile;
	hfPlayerProfile *	profile;

};

/*
================================================
hfSaveGameProcessorLoadProfile
================================================
*/
class hfSaveGameProcessorLoadProfile : public hfSaveGameProcessorLoadFiles {
public:
	DEFINE_CLASS( hfSaveGameProcessorLoadProfile );

					hfSaveGameProcessorLoadProfile();
					~hfSaveGameProcessorLoadProfile();

	bool			InitLoadProfile( hfPlayerProfile * profile, const char * folder );
	virtual bool	Process();


private:
	hfFile_SaveGame *	profileFile;
	hfPlayerProfile *	profile;

};

// Synchronous check, just checks if a profile exists within the savegame location
bool Sys_SaveGameProfileCheck();

#endif
