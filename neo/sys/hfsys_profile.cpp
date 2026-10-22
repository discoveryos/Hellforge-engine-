#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#define SAVEGAME_PROFILE_FILENAME			"profile.bin"

hfCVar profile_verbose( "profile_verbose", "0", CVAR_BOOL, "Turns on debug spam for profiles" );

/*
================================================
hfProfileMgr
================================================
*/

/*
========================
hfProfileMgr
========================
*/
hfProfileMgr::hfProfileMgr() : 
	profileSaveProcessor( new (TAG_SAVEGAMES) hfSaveGameProcessorSaveProfile ),
	profileLoadProcessor( new (TAG_SAVEGAMES) hfSaveGameProcessorLoadProfile ),
	profile( NULL ),
	handle( 0 ) {
}


/*
================================================
~hfProfileMgr
================================================
*/
hfProfileMgr::~hfProfileMgr() {
}

/*
========================
hfProfileMgr::Init
========================
*/
void hfProfileMgr::Init( hfLocalUser * user_ ) {
	user = user_;
	handle = 0;
}

/*
========================
hfProfileMgr::Pump
========================
*/
void hfProfileMgr::Pump() {
	// profile can be NULL if we forced the user to register as in the case of map-ing into a level from the press start screen
	if ( profile == NULL ) {
		return;
	}

	// See if we are done with saving/loading the profile
	bool saving = profile->GetState() == hfPlayerProfile::SAVING;
	bool loading = profile->GetState() == hfPlayerProfile::LOADING;
	if ( ( saving || loading ) && session->IsSaveGameCompletedFromHandle( handle ) ) {
		profile->SetState( hfPlayerProfile::IDLE );

		if ( saving ) {
			// Done saving
		} else if ( loading ) {
			// Done loading
			const hfSaveLoadParms & parms = profileLoadProcessor->GetParms();
			if ( parms.GetError() == SAVEGAME_E_FOLDER_NOT_FOUND || parms.GetError() == SAVEGAME_E_FILE_NOT_FOUND ) {
				profile->SaveSettings( true );
			} else if ( parms.GetError() == SAVEGAME_E_CORRUPTED ) {
				hfLib::Warning( "Profile corrupt, creating a new one..." );
				common->Dialog().AddDialog( GDM_CORRUPT_PROFILE, DIALOG_CONTINUE, NULL, NULL, false );
				profile->SetDefaults();
				profile->SaveSettings( true );
			} else if ( parms.GetError() != SAVEGAME_E_NONE ) {
				profile->SetState( hfPlayerProfile::ERR );
			}

			session->OnLocalUserProfileLoaded( user );
		}
	} else if ( saving || loading ) {
		return;
	}

	// See if we need to save/load the profile
	if ( profile->GetRequestedState() == hfPlayerProfile::SAVE_REQUESTED && profile->IsDirty() ) {
		profile->MarkDirty( false );
		SaveSettingsAsync();
		// Syncs the steam data
		//session->StoreStats();
		profile->SetRequestedState( hfPlayerProfile::IDLE );
	} else if ( profile->GetRequestedState() == hfPlayerProfile::LOAD_REQUESTED ) {
		LoadSettingsAsync();
		profile->SetRequestedState( hfPlayerProfile::IDLE );
	}
}

/*
========================
hfProfileMgr::GetProfile
========================
*/
hfPlayerProfile * hfProfileMgr::GetProfile() {
	assert( user != NULL );
	if ( profile == NULL ) {
		// Lazy instantiation
		// Create a new profile
		profile = hfPlayerProfile::CreatePlayerProfile( user->GetInputDevice() );
		if ( profile == NULL ) {
			return NULL;
		}
	}

	bool loading = ( profile->GetState() == hfPlayerProfile::LOADING ) || ( profile->GetRequestedState() == hfPlayerProfile::LOAD_REQUESTED );
	if ( loading ) {
		return NULL;
	}

	return profile;
}

/*
========================
hfProfileMgr::SaveSettingsAsync
========================
*/
void hfProfileMgr::SaveSettingsAsync() {
	if ( !saveGame_enable.GetBool() ) {
		hfLib::Warning( "Skipping profile save because saveGame_enable = 0" );
	}

	if ( GetProfile() != NULL ) {
		// Issue the async save...
		if ( profileSaveProcessor->InitSaveProfile( profile, "" ) ) {


			profileSaveProcessor->AddCompletedCallback( MakeCallback( this, &hfProfileMgr::OnSaveSettingsCompleted, &profileSaveProcessor->GetParmsNonConst() ) );
			handle = session->GetSaveGameManager().ExecuteProcessor( profileSaveProcessor.get() );
			profile->SetState( hfPlayerProfile::SAVING );

		}
	} else {
		hfLib::Warning( "Not saving profile, profile is NULL." );
	}
}

/*
========================
hfProfileMgr::LoadSettingsAsync
========================
*/
void hfProfileMgr::LoadSettingsAsync() {
 	if ( profile != NULL && saveGame_enable.GetBool() ) {
		if ( profileLoadProcessor->InitLoadProfile( profile, "" ) ) {
			// Skip the not found error because this might be the first time to play the game!
			profileLoadProcessor->SetSkipSystemErrorDialogMask( SAVEGAME_E_FOLDER_NOT_FOUND | SAVEGAME_E_FILE_NOT_FOUND );

			profileLoadProcessor->AddCompletedCallback( MakeCallback( this, &hfProfileMgr::OnLoadSettingsCompleted, &profileLoadProcessor->GetParmsNonConst() ) );
			handle = session->GetSaveGameManager().ExecuteProcessor( profileLoadProcessor.get() );
			profile->SetState( hfPlayerProfile::LOADING );


		}
	} else {
		// If not able to save the profile, just change the state and leave
		if ( profile == NULL ) {
			hfLib::Warning( "Not loading profile, profile is NULL." );
		}
		if ( !saveGame_enable.GetBool() ) {
			hfLib::Warning( "Skipping profile load because saveGame_enable = 0" );
		}
	}
}

/*
========================
hfProfileMgr::OnLoadSettingsCompleted
========================
*/
void hfProfileMgr::OnLoadSettingsCompleted( hfSaveLoadParms * parms ) {





	// Don't process if error already detected
	if ( parms->errorCode != SAVEGAME_E_NONE ) {
		return;
	}

	// Serialize the loaded profile
	hfFile_SaveGame ** profileFileContainer = FindFromGenericPtr( parms->files, SAVEGAME_PROFILE_FILENAME );
	hfFile_SaveGame * profileFile = profileFileContainer == NULL ? NULL : *profileFileContainer;

	bool foundProfile = profileFile != NULL && profileFile->Length() > 0;

	if ( foundProfile ) {
		hfTempArray< byte > buffer( MAX_PROFILE_SIZE );

		// Serialize settings from this buffer
		profileFile->MakeReadOnly();
		unsigned int originalChecksum;
		profileFile->ReadBig( originalChecksum );

		int dataLength = profileFile->Length() - (int)sizeof( originalChecksum );
		profileFile->ReadBigArray( buffer.Ptr(), dataLength );

		// Validate the checksum before we let the game serialize the settings
		unsigned int checksum = MD5_BlockChecksum( buffer.Ptr(), dataLength );
		if ( originalChecksum != checksum ) {
			hfLib::Warning( "Checksum: 0x%08x, originalChecksum: 0x%08x, size = %d", checksum, originalChecksum, dataLength );
			parms->errorCode = SAVEGAME_E_CORRUPTED;
		} else {
			hfBitMsg msg;
			msg.InitRead( buffer.Ptr(), (int)buffer.Size() );
			hfSerializer ser( msg, false );
			if ( !profile->Serialize( ser ) ) {
				parms->errorCode = SAVEGAME_E_CORRUPTED;
			}
		}

	} else {
		parms->errorCode = SAVEGAME_E_FILE_NOT_FOUND;
	}
}

/*
========================
hfProfileMgr::OnSaveSettingsCompleted
========================
*/
void hfProfileMgr::OnSaveSettingsCompleted( hfSaveLoadParms * parms ) {
	common->Dialog().ShowSaveIndicator( false );

	if ( parms->GetError() != SAVEGAME_E_NONE ) {
		common->Dialog().AddDialog( GDM_PROFILE_SAVE_ERROR, DIALOG_CONTINUE, NULL, NULL, false );
	}
	if ( game ) {
		game->Shell_UpdateSavedGames();
	}
}

/*
================================================
hfSaveGameProcessorSaveProfile
================================================
*/

/*
========================
hfSaveGameProcessorSaveProfile::hfSaveGameProcessorSaveProfile
========================
*/
hfSaveGameProcessorSaveProfile::hfSaveGameProcessorSaveProfile() {
	profileFile = NULL;
	profile = NULL;

}

/*
========================
hfSaveGameProcessorSaveProfile::InitSaveProfile
========================
*/
bool hfSaveGameProcessorSaveProfile::InitSaveProfile( hfPlayerProfile * profile_, const char * folder ) {
	// Serialize the profile and pass a file to the processor
	profileFile = new (TAG_SAVEGAMES) hfFile_SaveGame( SAVEGAME_PROFILE_FILENAME, SAVEGAMEFILE_BINARY | SAVEGAMEFILE_AUTO_DELETE );
	profileFile->MakeWritable();
	profileFile->SetMaxLength( MAX_PROFILE_SIZE );

	// Create a serialization object and let the game serialize the settings into the buffer
	const int serializeSize = MAX_PROFILE_SIZE - 8;	// -8 for checksum (all platforms) and length (on 360)
	hfTempArray< byte > buffer( serializeSize );
	hfBitMsg msg;
	msg.InitWrite( buffer.Ptr(), serializeSize );
	hfSerializer ser( msg, true );
	profile_->Serialize( ser );

	// Get and write the checksum & length first
	unsigned int checksum = MD5_BlockChecksum( msg.GetReadData(), msg.GetSize() );
	profileFile->WriteBig( checksum );

	hfLib::PrintfIf( profile_verbose.GetBool(), "checksum: 0x%08x, length: %d\n", checksum, msg.GetSize() );

	// Add data to the file and prepare for save
	profileFile->Write( msg.GetReadData(), msg.GetSize() );
	profileFile->MakeReadOnly();

	saveFileEntryList_t files;
	files.Append( profileFile );

	hfSaveGameDetails description;
	if ( !hfSaveGameProcessorSaveFiles::InitSave( folder, files, description, hfSaveGameManager::PACKAGE_PROFILE ) ) {
		return false;
	}


	profile = profile_;


	return true;
}

/*
========================
hfSaveGameProcessorSaveProfile::Process
========================
*/
bool hfSaveGameProcessorSaveProfile::Process() {


	// Files already setup for save, just execute as normal files
	return hfSaveGameProcessorSaveFiles::Process();

}


/*
================================================
hfSaveGameProcessorLoadProfile
================================================
*/

/*
========================
hfSaveGameProcessorLoadProfile::hfSaveGameProcessorLoadProfile
========================
*/
hfSaveGameProcessorLoadProfile::hfSaveGameProcessorLoadProfile() {
	profileFile = NULL;
	profile = NULL;

}

/*
========================
hfSaveGameProcessorLoadProfile::~hfSaveGameProcessorLoadProfile
========================
*/
hfSaveGameProcessorLoadProfile::~hfSaveGameProcessorLoadProfile() {
}

/*
========================
hfSaveGameProcessorLoadProfile::InitLoadFiles
========================
*/
bool hfSaveGameProcessorLoadProfile::InitLoadProfile( hfPlayerProfile * profile_, const char * folder_ ) {
	if ( !hfSaveGameProcessor::Init() ) {
		return false;
	}
	
	parms.directory = AddSaveFolderPrefix( folder_, hfSaveGameManager::PACKAGE_PROFILE );
	parms.description.slotName = folder_;
	parms.mode = SAVEGAME_MBF_LOAD;

	profileFile = new (TAG_SAVEGAMES) hfFile_SaveGame( SAVEGAME_PROFILE_FILENAME, SAVEGAMEFILE_BINARY | SAVEGAMEFILE_AUTO_DELETE );
	parms.files.Append( profileFile );

	profile = profile_;


	return true;
}

/*
========================
hfSaveGameProcessorLoadProfile::Process
========================
*/
bool hfSaveGameProcessorLoadProfile::Process() {


	return hfSaveGameProcessorLoadFiles::Process();

}


/*
========================
Sys_SaveGameProfileCheck
========================
*/
bool Sys_SaveGameProfileCheck() {
	bool exists = false;
	const char * saveFolder = "savegame";

	if ( fileSystem->IsFolder( saveFolder, "fs_savePath" ) == FOLDER_YES ) {
		hfFileList * files = fileSystem->ListFiles( saveFolder, SAVEGAME_PROFILE_FILENAME );
		const hfStrList & fileList = files->GetList();

		for ( int i = 0; i < fileList.Num(); i++ ) {
			hfStr filename = fileList[i];
			if ( filename == SAVEGAME_PROFILE_FILENAME ) {
				exists = true;
				break;
			}
		}

		fileSystem->FreeFileList( files );
	}

	return exists;
}

