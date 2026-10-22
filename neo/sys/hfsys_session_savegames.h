#ifndef __SYS_SESSION_SAVEGAMES_H__
#define __SYS_SESSION_SAVEGAMES_H__

/*
================================================
hfSaveGameProcessorLoadFiles
================================================
*/
class hfSaveGameProcessorLoadFiles : public hfSaveGameProcessor {
public:
	DEFINE_CLASS( hfSaveGameProcessorLoadFiles );

	virtual bool	InitLoadFiles( const char * folder, 
									const saveFileEntryList_t & files, 
									hfSaveGameManager::packageType_t type = hfSaveGameManager::PACKAGE_GAME );
	virtual bool	Process();
};

/*
================================================win_sav
hfSaveGameProcessorDelete
================================================
*/
class hfSaveGameProcessorDelete : public hfSaveGameProcessor {
public:
	DEFINE_CLASS( hfSaveGameProcessorDelete );

	bool			InitDelete( const char * folder, hfSaveGameManager::packageType_t type = hfSaveGameManager::PACKAGE_GAME );
	virtual bool	Process();
};

/*
================================================
hfSaveGameProcessorSaveFiles
================================================
*/
class hfSaveGameProcessorSaveFiles : public hfSaveGameProcessor {
public:
	DEFINE_CLASS( hfSaveGameProcessorSaveFiles );

	// Passing in hfSaveGameDetails so that we have a copy on output
	bool			InitSave( const char * folder, 
								const saveFileEntryList_t & files, 
								const hfSaveGameDetails & description,
								hfSaveGameManager::packageType_t type = hfSaveGameManager::PACKAGE_GAME );
	virtual bool	Process();
};

/*
================================================
hfSaveGameProcessorEnumerateGames
================================================
*/
class hfSaveGameProcessorEnumerateGames : public hfSaveGameProcessor {
public:
	DEFINE_CLASS( hfSaveGameProcessorEnumerateGames );

	virtual bool	Process();
};

#endif

