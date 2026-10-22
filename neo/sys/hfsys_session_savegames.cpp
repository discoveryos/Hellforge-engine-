#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfsys_savegame.h"
#include "hfsys_session_local.h"
#include "hfsys_session_savegames.h"


extern hfCVar saveGame_verbose;

hfCVar savegame_error( "savegame_error", "0", CVAR_INTEGER, "Combination of bits that will simulate and error, see 'savegamePrintErrors'.  0 = no error" );

void OutputDetailList( const saveGameDetailsList_t & savegameList );

#pragma region PROCESSORS

/*
================================================================================================

hfSaveGameProcessorLoadFiles

================================================================================================
*/

/*
========================
hfSaveGameProcessorLoadFiles::InitLoadFiles
========================
*/
bool hfSaveGameProcessorLoadFiles::InitLoadFiles( const char * folder_, const saveFileEntryList_t & files, hfSaveGameManager::packageType_t type ) {
	if ( !hfSaveGameProcessor::Init() ) {
		return false;
	}
	
	parms.directory = AddSaveFolderPrefix( folder_, type );
	parms.description.slotName = folder_;
	parms.mode = SAVEGAME_MBF_LOAD;

	for ( int i = 0; i < files.Num(); ++i ) {
		parms.files.Append( files[i] );
	}

	return true;
}

/*
========================
hfSaveGameProcessorLoadFiles::Process
========================
*/
bool hfSaveGameProcessorLoadFiles::Process() {
	// Platform-specific impl
	// This will populate an hfFile_Memory with the contents of the save game
	// This will not initialize the game, only load the file from the file-system
	Sys_ExecuteSavegameCommandAsync( &parms );
	return false;
}

/*
================================================================================================

hfSaveGameProcessorDelete

================================================================================================
*/

/*
========================
hfSaveGameProcessorDelete::Init
========================
*/
bool hfSaveGameProcessorDelete::InitDelete( const char * folder_, hfSaveGameManager::packageType_t type ) {
	if ( !hfSaveGameProcessor::Init() ) {
		return false;
	}

	parms.description.slotName = folder_;
	parms.directory = AddSaveFolderPrefix( folder_, type );
	parms.mode = SAVEGAME_MBF_DELETE_FOLDER;

	return true;
}

/*
========================
hfSaveGameProcessorDelete::Process
========================
*/
bool hfSaveGameProcessorDelete::Process() {
	// Platform-specific impl
	// This will populate an hfFile_Memory with the contents of the save game
	// This will not initialize the game, only load the file from the file-system
	Sys_ExecuteSavegameCommandAsync( &parms );

	return false;
}

/*
================================================================================================

hfSaveGameProcessorSaveFiles

================================================================================================
*/

/*
========================
hfSaveGameProcessorSaveFiles::InitSave
========================
*/
bool hfSaveGameProcessorSaveFiles::InitSave( const char * folder, const saveFileEntryList_t & files, const hfSaveGameDetails & descriptionForPS3, hfSaveGameManager::packageType_t type ) {
	if ( !hfSaveGameProcessor::Init() ) {
		return false;
	}

	if ( files.Num() == 0 ) {
		hfLib::Warning( "No files to save." );
		return false;
	}

	// Setup save system
	parms.directory = AddSaveFolderPrefix( folder, type );
	parms.mode = SAVEGAME_MBF_SAVE;	// do NOT delete the existing files
	for ( int i = 0; i < files.Num(); ++i ) {
		parms.files.Append( files[i] );
	}


	this->parms.description = descriptionForPS3;
	parms.description.slotName = folder;

	return true;
}

/*
========================
hfSaveGameProcessorSaveFiles::Process 
========================
*/
bool hfSaveGameProcessorSaveFiles::Process() {
	// Platform-specific implementation
	// This will start a worker thread for async operation.
	// It will always signal when it's completed.
	Sys_ExecuteSavegameCommandAsync( &parms );

	return false;
}

/*
================================================================================================
hfSaveGameProcessorEnumerateGames
================================================================================================
*/

/*
========================
hfSaveGameProcessorEnumerateGames::Process
========================
*/
bool hfSaveGameProcessorEnumerateGames::Process() {
	parms.mode = SAVEGAME_MBF_ENUMERATE | SAVEGAME_MBF_READ_DETAILS;

	// Platform-specific implementation
	// This will start a worker thread for async operation.
	// It will always signal when it's completed.
	Sys_ExecuteSavegameCommandAsync( &parms );

	return false;
}

#pragma endregion 

/*
========================
hfSessionLocal::SaveGameSync
========================
*/
saveGameHandle_t hfSessionLocal::SaveGameSync( const char * name, const saveFileEntryList_t & files, const hfSaveGameDetails & description ) {
	saveGameHandle_t handle = 0;

	// serialize the description file behind their back...
	saveFileEntryList_t filesWithDetails( files );
	hfFile_SaveGame * gameDetailsFile = new (TAG_SAVEGAMES) hfFile_SaveGame( SAVEGAME_DETAILS_FILENAME, SAVEGAMEFILE_TEXT | SAVEGAMEFILE_AUTO_DELETE );
	gameDetailsFile->MakeWritable();
	description.descriptors.WriteToIniFile( gameDetailsFile );
	filesWithDetails.Append( gameDetailsFile );

	if ( processorSaveFiles->InitSave( name, filesWithDetails, description ) ) {
		processorSaveFiles->AddCompletedCallback( MakeCallback( this, &hfSessionLocal::OnSaveCompleted, &processorSaveFiles->GetParmsNonConst() ) );
		handle = GetSaveGameManager().ExecuteProcessorAndWait( processorSaveFiles );
	}

	// Errors within the process of saving are handled in OnSaveCompleted()
	// so that asynchronous save errors are handled the same was as synchronous.
	if ( handle == 0 ) {
		hfSaveLoadParms & parms = processorSaveFiles->GetParmsNonConst();
		parms.errorCode = SAVEGAME_E_UNKNOWN;

		// Uniform error handling
		OnSaveCompleted( &parms );
	}

	return handle;
}

/*
========================
hfSessionLocal::SaveGameAsync
========================
*/
saveGameHandle_t hfSessionLocal::SaveGameAsync( const char * name, const saveFileEntryList_t & files, const hfSaveGameDetails & description ) {
	saveGameHandle_t handle = 0;

	// Done this way so we know it will be shutdown properly on early exit or exception
	struct local_t {
		local_t( hfSaveLoadParms * localparms ) : parms( localparms ) { 
			// Prepare background renderer
		}
		~local_t() { 
			// Shutdown background renderer
		}
		hfSaveLoadParms * parms;
	} local( &processorSaveFiles->GetParmsNonConst() );

	// serialize the description file behind their back...
	saveFileEntryList_t filesWithDetails( files );
	hfFile_SaveGame * gameDetailsFile = new (TAG_SAVEGAMES) hfFile_SaveGame( SAVEGAME_DETAILS_FILENAME, SAVEGAMEFILE_TEXT | SAVEGAMEFILE_AUTO_DELETE );
	gameDetailsFile->MakeWritable();
	description.descriptors.WriteToIniFile( gameDetailsFile );
	filesWithDetails.Append( gameDetailsFile );

	if ( processorSaveFiles->InitSave( name, filesWithDetails, description ) ) {
		processorSaveFiles->AddCompletedCallback( MakeCallback( this, &hfSessionLocal::OnSaveCompleted, &processorSaveFiles->GetParmsNonConst() ) );
		handle = GetSaveGameManager().ExecuteProcessor( processorSaveFiles );
	}

	// Errors within the process of saving are handled in OnSaveCompleted()
	// so that asynchronous save errors are handled the same was as synchronous.
	if ( handle == 0 ) {
		hfSaveLoadParms & parms = processorSaveFiles->GetParmsNonConst();
		parms.errorCode = SAVEGAME_E_UNKNOWN;

		common->Dialog().ShowSaveIndicator( false );
		// Uniform error handling
		OnSaveCompleted( &parms );
	}


	return handle;
}

/*
========================
hfSessionLocal::OnSaveCompleted
========================
*/
void hfSessionLocal::OnSaveCompleted( hfSaveLoadParms * parms ) {
	hfLocalUser * master = session->GetSignInManager().GetMasterLocalUser();

	if ( parms->GetError() != SAVEGAME_E_INSUFFICIENT_ROOM ) {
		// if savegame completeed we can clear retry info
		GetSaveGameManager().ClearRetryInfo();
	}

	// Only turn off the indicator if we're not also going to save the profile settings
	if ( master != NULL && master->GetProfile() != NULL && !master->GetProfile()->IsDirty() ) {
		common->Dialog().ShowSaveIndicator( false );
	}

	if ( parms->GetError() == SAVEGAME_E_NONE ) {
		// Save the profile any time we save the game
		if ( master != NULL && master->GetProfile() != NULL ) {
			master->GetProfile()->SaveSettings( false );
		}

		// Update the enumerated savegames
		saveGameDetailsList_t & detailList = session->GetSaveGameManager().GetEnumeratedSavegamesNonConst();
		hfSaveGameDetails * details = detailList.Find( parms->description );
		if ( details == NULL ) {
			// add it
			detailList.Append( parms->description );
		} else {
			// replace it
			*details = parms->description;
		}
	}

	// Error handling and additional processing
	common->OnSaveCompleted( *parms );
}

/*
========================
hfSessionLocal::LoadGameSync

We still want to use the savegame manager because we could have file system operations in flight and need to 
========================
*/
saveGameHandle_t hfSessionLocal::LoadGameSync( const char * name, saveFileEntryList_t & files ) {
	hfSaveLoadParms & parms = processorLoadFiles->GetParmsNonConst();
	saveGameHandle_t handle = 0;

	{
		// Put in a local block so everything will go in the global heap before the map change, but the heap is 
		// automatically popped out on early return or exception
		// You cannot be in the global heap during a map change...
		//hfScopedGlobalHeap everythingGoesInTheGlobalHeap;

		// Done this way so we know it will be shutdown properly on early exit or exception
		struct local_t {
			local_t( hfSaveLoadParms * parms_ ) : parms( parms_ ) { 
				// Prepare background renderer or loadscreen with what you want to show
				{
					// with mode: SAVE_GAME_MODE_LOAD
				}
			}
			~local_t() { 
				// Shutdown background renderer or loadscreen
				{
				}

				common->OnLoadCompleted( *parms );
			}
			hfSaveLoadParms * parms;
		} local( &parms );

		// Read the details file when loading games
		saveFileEntryList_t	filesWithDetails( files );
		std::auto_ptr< hfFile_SaveGame > gameDetailsFile( new (TAG_SAVEGAMES) hfFile_SaveGame( SAVEGAME_DETAILS_FILENAME, SAVEGAMEFILE_TEXT ) );
		filesWithDetails.Append( gameDetailsFile.get() );

		// Check the cached save details from the enumeration and make sure we don't load a save from a newer version of the game!
		const saveGameDetailsList_t details = GetSaveGameManager().GetEnumeratedSavegames();
		for ( int i = 0; i < details.Num(); ++i ) {
			if ( hfStr::Cmp( name, details[i].slotName ) == 0 ) {
				if ( details[i].GetSaveVersion() > BUILD_NUMBER ) {
					parms.errorCode = SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION;
					return 0;
				}
			}
		}

		// Synchronous load
		if ( processorLoadFiles->InitLoadFiles( name, filesWithDetails ) ) {
			handle = GetSaveGameManager().ExecuteProcessorAndWait( processorLoadFiles );
		}

		if ( handle == 0 ) {
			parms.errorCode = SAVEGAME_E_UNKNOWN;
		}

		if ( parms.GetError() != SAVEGAME_E_NONE ) {
			return 0;
		}

		// Checks the description file to see if corrupted or if it's from a newer savegame
		if ( !LoadGameCheckDescriptionFile( parms ) ) {
			return 0;
		}
		
		// Checks to see if loaded map is from a DLC map and if that DLC is active
		if ( !IsDLCAvailable( parms.description.GetMapName() ) ) {
			parms.errorCode = SAVEGAME_E_DLC_NOT_FOUND;
			return 0;
		}
	}

	common->OnLoadFilesCompleted( parms );

	return handle;
}

/*
========================
hfSessionLocal::OnLoadCompleted
========================
*/
void hfSessionLocal::OnLoadCompleted( hfSaveLoadParms * parms ) {
}

/*
========================
hfSessionLocal::EnumerateSaveGamesSync
========================
*/
saveGameHandle_t hfSessionLocal::EnumerateSaveGamesSync() {
	saveGameHandle_t handle = 0;

	// Done this way so we know it will be shutdown properly on early exit or exception
	struct local_t {
		local_t() { 
			// Prepare background renderer or loadscreen with what you want to show
			{
				// with mode: SAVE_GAME_MODE_ENUMERATE
			}
		}
		~local_t() { 
			// Shutdown background renderer or loadscreen
			{
			}
		}
	} local;

	// flush the old enumerated list
	GetSaveGameManager().GetEnumeratedSavegamesNonConst().Clear();

	if ( processorEnumerate->Init() ) {
		processorEnumerate->AddCompletedCallback( MakeCallback( this, &hfSessionLocal::OnEnumerationCompleted, &processorEnumerate->GetParmsNonConst() ) );
		handle = GetSaveGameManager().ExecuteProcessorAndWait( processorEnumerate );
	}

	// Errors within the process of saving are handled in OnEnumerationCompleted()
	// so that asynchronous save errors are handled the same was as synchronous.
	if ( handle == 0 ) {
		hfSaveLoadParms & parms = processorEnumerate->GetParmsNonConst();
		parms.errorCode = SAVEGAME_E_UNKNOWN;

		// Uniform error handling
		OnEnumerationCompleted( &parms );
	}

	return handle;
}

/*
========================
hfSessionLocal::EnumerateSaveGamesAsync
========================
*/
saveGameHandle_t hfSessionLocal::EnumerateSaveGamesAsync() {
	saveGameHandle_t handle = 0;

	// flush the old enumerated list
	GetSaveGameManager().GetEnumeratedSavegamesNonConst().Clear();

	if ( processorEnumerate->Init() ) {
		processorEnumerate->AddCompletedCallback( MakeCallback( this, &hfSessionLocal::OnEnumerationCompleted, &processorEnumerate->GetParmsNonConst() ) );
		handle = GetSaveGameManager().ExecuteProcessor( processorEnumerate );
	}

	// Errors within the process of saving are handled in OnEnumerationCompleted()
	// so that asynchronous save errors are handled the same was as synchronous.
	if ( handle == 0 ) {
		hfSaveLoadParms & parms = processorEnumerate->GetParmsNonConst();
		parms.errorCode = SAVEGAME_E_UNKNOWN;

		// Uniform error handling
		OnEnumerationCompleted( &parms );
	}

	return handle;
}

int hfSort_EnumeratedSavegames( const hfSaveGameDetails * a, const hfSaveGameDetails * b ) {
	return b->date - a->date;
}

/*
========================
hfSessionLocal::OnEnumerationCompleted
========================
*/
void hfSessionLocal::OnEnumerationCompleted( hfSaveLoadParms * parms ) {
	// hfTech4 hfList::sort is just a qsort wrapper, which doesn't deal with
	// hfStrStatic properly!
	// parms->detailList.Sort( hfSort_EnumeratedSavegames );
	std::sort( parms->detailList.Ptr(), parms->detailList.Ptr() + parms->detailList.Num() );

	if ( parms->GetError() == SAVEGAME_E_NONE ) {
		// Copy into the maintained list 
		saveGameDetailsList_t & detailsList = session->GetSaveGameManager().GetEnumeratedSavegamesNonConst();
		//mem.PushHeap();
		detailsList = parms->detailList;	// copies new list into the savegame manager's reference
		//mem.PopHeap();

		// The platform-specific implementations don't know about the prefixes
		// If we don't do this here, we will end up with slots like: GAME-GAME-GAME-GAME-AUTOSAVE...
		for ( int i = 0; i < detailsList.Num(); i++ ) {
			hfSaveGameDetails & details = detailsList[i];

			const hfStr original = details.slotName;
			const hfStr stripped = RemoveSaveFolderPrefix( original, hfSaveGameManager::PACKAGE_GAME );
			details.slotName = stripped;
		}

		if ( saveGame_verbose.GetBool() ) {
			OutputDetailList( detailsList );
		}
	}

	common->OnEnumerationCompleted( *parms );
}

/*
========================
hfSessionLocal::DeleteSaveGameSync
========================
*/
saveGameHandle_t hfSessionLocal::DeleteSaveGameSync( const char * name ) {
	saveGameHandle_t handle = 0;

	// Done this way so we know it will be shutdown properly on early exit or exception
	struct local_t {
		local_t() { 
			// Prepare background renderer or loadscreen with what you want to show
			{
				// with mode: SAVE_GAME_MODE_DELETE
			}
		}
		~local_t() { 
			// Shutdown background renderer or loadscreen
			{
			}
		}
	} local;

	if ( processorDelete->InitDelete( name ) ) {
		processorDelete->AddCompletedCallback( MakeCallback( this, &hfSessionLocal::OnDeleteCompleted, &processorDelete->GetParmsNonConst() ) );
		handle = GetSaveGameManager().ExecuteProcessorAndWait( processorDelete );
	}

	// Errors within the process of saving are handled in OnDeleteCompleted()
	// so that asynchronous save errors are handled the same was as synchronous.
	if ( handle == 0 ) {
		hfSaveLoadParms & parms = processorDelete->GetParmsNonConst();
		parms.errorCode = SAVEGAME_E_UNKNOWN;

		// Uniform error handling
		OnDeleteCompleted( &parms );
	}

	return handle;
}

/*
========================
hfSessionLocal::DeleteSaveGameAsync
========================
*/
saveGameHandle_t hfSessionLocal::DeleteSaveGameAsync( const char * name ) {
	saveGameHandle_t handle = 0;
	if ( processorDelete->InitDelete( name ) ) {
		processorDelete->AddCompletedCallback( MakeCallback( this, &hfSessionLocal::OnDeleteCompleted, &processorDelete->GetParmsNonConst() ) );
		common->Dialog().ShowSaveIndicator( true );
		handle = GetSaveGameManager().ExecuteProcessor( processorDelete );
	}

	// Errors within the process of saving are handled in OnDeleteCompleted()
	// so that asynchronous save errors are handled the same was as synchronous.
	if ( handle == 0 ) {
		hfSaveLoadParms & parms = processorDelete->GetParmsNonConst();
		parms.errorCode = SAVEGAME_E_UNKNOWN;

		// Uniform error handling
		OnDeleteCompleted( &parms );
	}

	return handle;
}

/*
========================
hfSessionLocal::OnDeleteCompleted
========================
*/
void hfSessionLocal::OnDeleteCompleted( hfSaveLoadParms * parms ) {
	common->Dialog().ShowSaveIndicator( false );

	if ( parms->GetError() == SAVEGAME_E_NONE ) {
		// Update the enumerated list
		saveGameDetailsList_t & details = session->GetSaveGameManager().GetEnumeratedSavegamesNonConst();
		details.Remove( parms->description );
	}

	common->OnDeleteCompleted( *parms );
}

/*
========================
hfSessionLocal::IsEnumerating
========================
*/
bool hfSessionLocal::IsEnumerating() const {
	return !session->IsSaveGameCompletedFromHandle( processorEnumerate->GetHandle() );
}

/*
========================
hfSessionLocal::GetEnumerationHandle
========================
*/
saveGameHandle_t hfSessionLocal::GetEnumerationHandle() const {
	return processorEnumerate->GetHandle();
}

/*
========================
hfSessionLocal::IsDLCAvailable
========================
*/
bool hfSessionLocal::IsDLCAvailable( const char * mapName ) {
	bool hasContentPackage = true;
	return hasContentPackage;
}

/*
========================
hfSessionLocal::LoadGameCheckDiscNumber
========================
*/
bool hfSessionLocal::LoadGameCheckDiscNumber( hfSaveLoadParms & parms ) {
#if 0
	hfStr mapName = parms.description.GetMapName();

	assert( !discSwapStateMgr->IsWorking() );
	discSwapStateMgr->Init( &parms.callbackSignal, hfDiscSwapStateManager::DISC_SWAP_COMMAND_LOAD );
	//// TODO_KC this is probably broken now...
	//discSwapStateMgr->folder = folder;
	//discSwapStateMgr->spawnInfo = newSpawnInfo;
	//discSwapStateMgr->spawnSpot = newSpawnPoint;
	//discSwapStateMgr->instanceFileName = instanceFileName;
	discSwapStateMgr->user = session->GetSignInManager().GetMasterLocalUser();
	discSwapStateMgr->map = mapName;

	discSwapStateMgr->Pump();
	while ( discSwapStateMgr->IsWorking() ) {
		Sys_Sleep( 15 );
		// process input and render

		discSwapStateMgr->Pump();
	}

	hfDiscSwapStateManager::discSwapStateError_t discSwapError = discSwapStateMgr->GetError();

	if ( discSwapError == hfDiscSwapStateManager::DSSE_CANCEL ) {
		parms.errorCode = SAVEGAME_E_CANCELLED;
	} else if ( discSwapError == hfDiscSwapStateManager::DSSE_INSUFFICIENT_ROOM ) {
		parms.errorCode = SAVEGAME_E_INSUFFICIENT_ROOM;
		parms.requiredSpaceInBytes = discSwapStateMgr->GetRequiredStorageBytes();
	} else if ( discSwapError == hfDiscSwapStateManager::DSSE_CORRECT_DISC_ALREADY ) {
		parms.errorCode = SAVEGAME_E_NONE;
	} else if ( discSwapError != hfDiscSwapStateManager::DSSE_OK ) {
		parms.errorCode = SAVEGAME_E_DISC_SWAP;
	}

	if ( parms.errorCode == SAVEGAME_E_UNKNOWN ) {
		parms.errorCode = SAVEGAME_E_DISC_SWAP;
	}
#endif

	return ( parms.GetError() == SAVEGAME_E_NONE );
}

/*
========================
hfSessionLocal::LoadGameCheckDescriptionFile
========================
*/
bool hfSessionLocal::LoadGameCheckDescriptionFile( hfSaveLoadParms & parms ) {
	hfFile_SaveGame ** detailsFile = FindFromGenericPtr( parms.files, SAVEGAME_DETAILS_FILENAME );
	if ( detailsFile == NULL ) {
		parms.errorCode = SAVEGAME_E_FILE_NOT_FOUND;
		return false;
	}

	assert( *detailsFile != NULL );
	(*detailsFile)->MakeReadOnly();

	if ( !SavegameReadDetailsFromFile( *detailsFile, parms.description ) ) {
		parms.errorCode = SAVEGAME_E_CORRUPTED;
	} else {
		if ( parms.description.GetSaveVersion() > BUILD_NUMBER ) {
			parms.errorCode = SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION;
		}
	}

	return ( parms.GetError() == SAVEGAME_E_NONE );
}

#pragma region COMMANDS
/*
================================================================================================

COMMANDS

================================================================================================
*/
CONSOLE_COMMAND( testSavegameDeleteAll, "delete all savegames without confirmation", 0 ) {
	if ( session == NULL ) {
		hfLib::Printf( "Invalid session.\n" );
		return;
	}

	hfSaveLoadParms parms;

	parms.SetDefaults();
	parms.mode = SAVEGAME_MBF_DELETE_ALL_FOLDERS | SAVEGAME_MBF_NO_COMPRESS;

	Sys_ExecuteSavegameCommandAsync( &parms );

	parms.callbackSignal.Wait();
	hfLib::Printf( "Completed process.\n" );
	hfLib::Printf( "Error = 0x%08X, %s\n", parms.GetError(), GetSaveGameErrorString( parms.GetError() ).c_str() );
}

CONSOLE_COMMAND( testSavegameDelete, "deletes a savegames without confirmation", 0 ) {
	if ( session == NULL ) {
		hfLib::Printf( "Invalid session.\n" );
		return;
	}

	if ( args.Argc() != 2 ) {
		hfLib::Printf( "Usage: testSavegameDelete <folder (without 'GAMES-')>\n" );
		return;
	}

	hfStr folder = args.Argv( 1 );
	hfSaveGameProcessorDelete testDeleteSaveGamesProc;
	if ( testDeleteSaveGamesProc.InitDelete( folder ) ) {
		session->GetSaveGameManager().ExecuteProcessorAndWait( &testDeleteSaveGamesProc );
	}

	hfLib::Printf( "Completed process.\n" );
	hfLib::Printf( "Error = 0x%08X, %s\n", testDeleteSaveGamesProc.GetParms().GetError(), GetSaveGameErrorString( testDeleteSaveGamesProc.GetParms().GetError() ).c_str() );
}

CONSOLE_COMMAND( testSavegameEnumerateFiles, "enumerates all the files in a folder (blank for 'current slot' folder, use 'autosave' for the autosave slot)", 0 ) {
	if ( session == NULL ) {
		hfLib::Printf( "Invalid session.\n" );
		return;
	}

	hfStr folder = session->GetCurrentSaveSlot();
	if ( args.Argc() > 1 ) {
		folder = args.Argv( 1 );
	}

	hfLib::Printf( "Testing folder: %s\n\n", folder.c_str() );

	hfSaveLoadParms parms;
	parms.SetDefaults();
	parms.mode = SAVEGAME_MBF_ENUMERATE_FILES;

	// Platform-specific implementation
	// This will start a worker thread for async operation.
	// It will always signal when it's completed.
	Sys_ExecuteSavegameCommandAsync( &parms );
	parms.callbackSignal.Wait();

	for ( int i = 0; i < parms.files.Num(); i++ ) {
		hfLib::Printf( S_COLOR_YELLOW "\t%d: %s\n" S_COLOR_DEFAULT, i, parms.files[i]->GetName() );
	}
}

/*
========================
OutputDetailList
========================
*/
void OutputDetailList( const saveGameDetailsList_t & savegameList ) {
	for ( int i = 0; i < savegameList.Num(); ++i ) {
		hfLib::Printf( S_COLOR_YELLOW "\t%s - %s\n" S_COLOR_DEFAULT
			"\t\tMap: %s\n"
			"\t\tTime: %s\n",
			savegameList[i].slotName.c_str(),
			savegameList[i].damaged ? S_COLOR_RED "CORRUPT" : S_COLOR_GREEN "OK",
			savegameList[i].damaged ? "?" : savegameList[i].descriptors.GetString( SAVEGAME_DETAIL_FIELD_MAP, "" ), 
			Sys_TimeStampToStr( savegameList[i].date )
			);
	}
}

CONSOLE_COMMAND( testSavegameEnumerate, "enumerates the savegames available", 0 ) {
	if ( session == NULL ) {
		hfLib::Printf( "Invalid session.\n" );
		return;
	}

	saveGameHandle_t handle = session->EnumerateSaveGamesSync();
	if ( handle == 0 ) {
		hfLib::Printf( "Error enumerating.\n" );
		return;
	}

	const saveGameDetailsList_t	gameList = session->GetSaveGameManager().GetEnumeratedSavegames();
	hfLib::Printf( "Savegames found: %d\n\n", gameList.Num() );
	OutputDetailList( gameList );
}

CONSOLE_COMMAND( testSaveGameCheck, "tests existence of savegame", 0 ) {
	bool exists;
	bool autosaveExists;
	Sys_SaveGameCheck( exists, autosaveExists );
	hfLib::Printf( "Savegame check: exists = %d, autosaveExists = %d\n", exists, autosaveExists );
}

CONSOLE_COMMAND( testSaveGameOutputEnumeratedSavegames, "outputs the list of savegames already enumerated, this does not re-enumerate", 0 ) {
	if ( session == NULL ) {
		hfLib::Printf( "Invalid session.\n" );
		return;
	}

	const saveGameDetailsList_t & savegames = session->GetSaveGameManager().GetEnumeratedSavegames();
	OutputDetailList( savegames );
}

CONSOLE_COMMAND( testSavegameGetCurrentSlot, "returns the current slot in use", 0 ) {
	if ( session == NULL ) {
		hfLib::Printf( "Invalid session.\n" );
		return;
	}

	hfLib::Printf( "Current slot: %s\n", session->GetCurrentSaveSlot() );
}

CONSOLE_COMMAND( testSavegameSetCurrentSlot, "returns the current slot in use", 0 ) {
	if ( session == NULL ) {
		hfLib::Printf( "Invalid session.\n" );
		return;
	}

	if ( args.Argc() != 2 ) {
		hfLib::Printf( "Usage: testSavegameSetCurrentSlot name\n" );
		return;
	}

	const char * slot = args.Argv( 1 );

	session->SetCurrentSaveSlot( slot );
	hfLib::Printf( "Current slot: %s\n", session->GetCurrentSaveSlot() );
}

CONSOLE_COMMAND( savegameSetErrorBit, "Allows you to set savegame_error by bit instead of integer value", 0 ) {
	int bit = atoi( args.Argv( 1 ) );
	savegame_error.SetInteger( savegame_error.GetInteger() | ( 1 << bit ) );
}

#pragma endregion
