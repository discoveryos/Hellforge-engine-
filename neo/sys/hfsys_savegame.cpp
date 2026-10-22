#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfsys_session_local.h"


hfCVar saveGame_verbose( "saveGame_verbose", "0", CVAR_BOOL | CVAR_ARCHIVE, "debug spam" );
hfCVar saveGame_checksum( "saveGame_checksum", "1", CVAR_BOOL, "data integrity check" );
hfCVar saveGame_enable( "saveGame_enable", "1", CVAR_BOOL, "are savegames enabled" );

void Sys_ExecuteSavegameCommandAsyncImpl( hfSaveLoadParms * savegameParms );


/*
========================
Sys_ExecuteSavegameCommandAsync
========================
*/
void Sys_ExecuteSavegameCommandAsync( hfSaveLoadParms * savegameParms ) {
	if ( savegameParms == NULL ) {
		hfLib::Error( "Programming Error with [%s]", __FUNCTION__ );
		return;
	}

	if ( !saveGame_enable.GetBool() ) {
		hfLib::Warning( "Savegames are disabled (saveGame_enable = 0). Skipping physical save to media." );
		savegameParms->errorCode = SAVEGAME_E_CANCELLED;
		savegameParms->callbackSignal.Raise();
		return;
	}

	Sys_ExecuteSavegameCommandAsyncImpl( savegameParms );
}

#define ASSERT_ENUM_STRING_BITFIELD( string, index )		( 1 / (int)!( string - ( 1 << index ) ) ) ? #string : ""

const char * saveGameErrorStrings[ SAVEGAME_E_NUM ] = {
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_CANCELLED,							0 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INSUFFICIENT_ROOM,					1 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_CORRUPTED,							2 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE,	3 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_UNKNOWN,							4 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INVALID_FILENAME,					5 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_STEAM_ERROR,						6 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_FOLDER_NOT_FOUND,					7 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_FILE_NOT_FOUND,						8 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_DLC_NOT_FOUND,						9 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INVALID_USER,						10 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_PROFILE_TOO_BIG,					11 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_DISC_SWAP,							12 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION,			13 ),
};

CONSOLE_COMMAND( savegamePrintErrors, "Prints error code corresponding to each bit", 0 ) {
	hfLib::Printf( "Bit  Description\n"
				   "---  -----------\n" );
	for ( int i = 0; i < SAVEGAME_E_BITS_USED; i++ ) {
		hfLib::Printf( "%03d  %s\n", i, saveGameErrorStrings[i] );
	}
}

/*
========================
GetSaveGameErrorString

This returns a comma delimited string of all the errors within the bitfield.  We don't ever expect more than one error,
the errors are bitfields so that they can be used to mask which errors we want to handle in the engine or leave up to the
game.

Example: 
	SAVEGAME_E_LOAD, SAVEGAME_E_INVALID_FILENAME
========================
*/
hfStr GetSaveGameErrorString( int errorMask ) {
	hfStr errorString;
	bool continueProcessing = errorMask > 0;
	int localError = errorMask;

	for ( int i = 0; i < SAVEGAME_E_NUM && continueProcessing; ++i ) {
		int mask = ( 1 << i );

		if ( localError & mask ) {
			localError ^= mask;	// turn off this error so we can quickly see if we are done

			continueProcessing = localError > 0;
			
			errorString.Append( saveGameErrorStrings[i] );

			if ( continueProcessing ) {
				errorString.Append( ", " );
			}
		}
	}

	return errorString;
}

/*
========================
GetSaveFolder

Prefixes help the PS3 filter what to show in lists
Files using the hidden prefix are not shown in the load game screen
Directory name for savegames, slot number or user-defined name appended to it
TRC R116 - PS3 folder must start with the product code
========================
*/
const hfStr & GetSaveFolder( hfSaveGameManager::packageType_t type ) {
	static bool initialized = false;
	static hfStrStatic<MAX_FOLDER_NAME_LENGTH>	saveFolder[hfSaveGameManager::PACKAGE_NUM];

	if ( !initialized ) {
		initialized = true;

		hfStr ps3Header = "";

		saveFolder[hfSaveGameManager::PACKAGE_GAME].Format( "%s%s", ps3Header.c_str(), SAVEGAME_GAME_DIRECTORY_PREFIX );
		saveFolder[hfSaveGameManager::PACKAGE_PROFILE].Format( "%s%s", ps3Header.c_str(), SAVEGAME_PROFILE_DIRECTORY_PREFIX );
		saveFolder[hfSaveGameManager::PACKAGE_RAW].Format( "%s%s", ps3Header.c_str(), SAVEGAME_RAW_DIRECTORY_PREFIX );
	}

	return saveFolder[type];
}

/*
========================
hfStr AddSaveFolderPrefix

	input	= RAGE_0
	output	= GAMES-RAGE_0
========================
*/
hfStr AddSaveFolderPrefix( const char * folder, hfSaveGameManager::packageType_t type ) {
	hfStr dir = GetSaveFolder( type );
	dir.Append( folder );


	return dir;
}

/*
========================
RemoveSaveFolderPrefix

	input	= GAMES-RAGE_0
	output	= RAGE_0
========================
*/
hfStr RemoveSaveFolderPrefix( const char * folder, hfSaveGameManager::packageType_t type ) {
	hfStr dir = folder;
	hfStr prefix = GetSaveFolder( type );
	dir.StripLeading( prefix );
	return dir;
}

/*
========================
bool SavegameReadDetailsFromFile

returns false when catastrophic error occurs, not when damaged
========================
*/
bool SavegameReadDetailsFromFile( hfFile * file, hfSaveGameDetails & details ) {
	details.damaged = false;

	// Read the DETAIL file for the enumerated data
	if ( !details.descriptors.ReadFromIniFile( file ) ) {
		details.damaged = true;
	}

	bool ignoreChecksum = details.descriptors.GetBool( "ignore_checksum", false );
	if ( !ignoreChecksum ) {
		// Get the checksum from the dict
		int readChecksum = details.descriptors.GetInt( SAVEGAME_DETAIL_FIELD_CHECKSUM, 0 );

		// Calculate checksum
		details.descriptors.Delete( SAVEGAME_DETAIL_FIELD_CHECKSUM );
		int checksum = (int)details.descriptors.Checksum();
		if ( readChecksum == 0 || checksum != readChecksum ) {
			details.damaged = true;
		}
	}

	return true;
}

/*
========================
hfSaveGameDetails::hfSaveGameDetails
========================
*/
hfSaveGameDetails::hfSaveGameDetails() { 
	Clear(); 
}

/*
========================
hfSaveGameDetails::Clear
========================
*/
void hfSaveGameDetails::Clear() {
	descriptors.Clear();
	damaged = false;
	date = 0;
	slotName[0] = NULL;
}

/*
========================
hfSaveLoadParms::hfSaveLoadParms
========================
*/
hfSaveLoadParms::hfSaveLoadParms() {
	// These are not done when we set defaults because SetDefaults is called internally within the execution of the processor and
	// these are set once and shouldn't be touched until the processor is re-initialized
	cancelled = false;

	Init();
}

/*
========================
hfSaveLoadParms::~hfSaveLoadParms
========================
*/
hfSaveLoadParms::~hfSaveLoadParms() {
	for ( int i = 0; i < files.Num(); ++i ) {
		if ( files[i]->type & SAVEGAMEFILE_AUTO_DELETE ) {
			delete files[i];
		}
	}
}

/*
========================
hfSaveLoadParms::ResetCancelled
========================
*/
void hfSaveLoadParms::ResetCancelled() {
	cancelled = false;
}

/*
========================
hfSaveLoadParms::Init

This should not touch anything statically created outside this class!
========================
*/
void hfSaveLoadParms::Init() {
	files.Clear();
	mode = SAVEGAME_MBF_NONE;
	directory = "";
	pattern = "";
	postPattern = "";
	requiredSpaceInBytes = 0;
	description.Clear();
	detailList.Clear();
	callbackSignal.Clear();
	errorCode = SAVEGAME_E_NONE;
	inputDeviceId = -1;
	skipErrorDialogMask = 0;
	
	// These are not done when we set defaults because SetDefaults is called internally within the execution of the processor and
	// these are set once and shouldn't be touched until the processor is re-initialized
	// cancelled = false;
}

/*
========================
hfSaveLoadParms::SetDefaults
========================
*/
void hfSaveLoadParms::SetDefaults( int newInputDevice ) {
	// These are pulled out so SetDefaults() isn't called during global instantiation of objects that have savegame processors
	// in them that then require a session reference.
	Init();	

	// fill in the user information (inputDeviceId & userId) from the master user
	hfLocalUser * user = NULL;

	if ( newInputDevice != -1 ) {
		user = session->GetSignInManager().GetLocalUserByInputDevice( newInputDevice );
	} else if ( session != NULL ) {
		user = session->GetSignInManager().GetMasterLocalUser();
	}

	if ( user != NULL ) {
		userId = hfStr::Hash( user->GetGamerTag() );
		hfLib::PrintfIf( saveGame_verbose.GetBool(), "profile userId/gamertag: %s (%d)\n", user->GetGamerTag(), userId );
		inputDeviceId = user->GetInputDevice();
	}
}

/*
========================
hfSaveLoadParms::CancelSaveGameFilePipelines
========================
*/
void hfSaveLoadParms::CancelSaveGameFilePipelines() {
	for ( int i = 0; i < files.Num(); i++ ) {
		if ( ( files[i]->type & SAVEGAMEFILE_PIPELINED ) != 0 ) {
			hfFile_SaveGamePipelined * file = dynamic_cast< hfFile_SaveGamePipelined * >( files[i] );
			assert( file != NULL );

			if ( file->GetMode() == hfFile_SaveGamePipelined::WRITE ) {
				// Notify the save game file that all writes failed which will cause all
				// writes on the other end of the pipeline to drop on the floor.
				file->NextWriteBlock( NULL );
			} else if ( file->GetMode() == hfFile_SaveGamePipelined::READ ) {
				// Notify end-of-file to the save game file which will cause all
				// reads on the other end of the pipeline to return zero bytes.
				file->NextReadBlock( NULL, 0 );
			}
		}
	}
}

/*
========================
hfSaveLoadParms::AbortSaveGameFilePipeline
========================
*/
void hfSaveLoadParms::AbortSaveGameFilePipeline() {
	for ( int i = 0; i < files.Num(); i++ ) {
		if ( ( files[i]->type & SAVEGAMEFILE_PIPELINED ) != 0 ) {
			hfFile_SaveGamePipelined * file = dynamic_cast< hfFile_SaveGamePipelined * >( files[i] );
			assert( file != NULL );
			file->Abort();
		}
	}
}

/*
================================================================================================
hfSaveGameProcessor
================================================================================================
*/

/*
========================
hfSaveGameProcessor::hfSaveGameProcessor
========================
*/
hfSaveGameProcessor::hfSaveGameProcessor() : working( false ), init( false ) {
}

/*
========================
hfSaveGameProcessor::Init
========================
*/
bool hfSaveGameProcessor::Init() {
	if ( !verify( !IsWorking() ) ) {
		hfLib::Warning( "[%s] Someone is trying to execute this processor twice, this is really bad!", this->Name() );
		return false;
	}
	
	parms.ResetCancelled();
	parms.SetDefaults();
	savegameLogicTestIterator = 0;
	working = false; 
	init = true;
	completedCallbacks.Clear();

	return true;
}

/*
========================
hfSaveGameProcessor::IsThreadFinished
========================
*/
bool hfSaveGameProcessor::IsThreadFinished() {
	return parms.callbackSignal.Wait( 0 );
}

/*
========================
hfSaveGameProcessor::AddCompletedCallback
========================
*/
void hfSaveGameProcessor::AddCompletedCallback( const hfCallback & callback ) {
	completedCallbacks.Append( callback.Clone() );
}

/*
================================================================================================
hfSaveGameManager
================================================================================================
*/

/*
========================
hfSaveGameManager::hfSaveGameManager
========================
*/
hfSaveGameManager::hfSaveGameManager() :
	processor( NULL ),
	cancel( false ),
	startTime( 0 ),
	continueProcessing( false ),
	submittedProcessorHandle( 0 ),
	executingProcessorHandle( 0 ),
	lastExecutedProcessorHandle( 0 ),
	storageAvailable( true ),
	retryFolder( NULL ) {
}

/*
========================
hfSaveGameManager::~hfSaveGameManager
========================
*/
hfSaveGameManager::~hfSaveGameManager() {	
	processor = NULL;
	enumeratedSaveGames.Clear();
}

/*
========================
hfSaveGameManager::ExecuteProcessor
========================
*/
saveGameHandle_t hfSaveGameManager::ExecuteProcessor( hfSaveGameProcessor * processor ) {
	hfLib::PrintfIf( saveGame_verbose.GetBool(), "[%s] : %s\n", __FUNCTION__, processor->Name() );

	// may not be running yet, but if we've init'd successfuly, the IsWorking() call should return true if this 
	// method has been called.  You have problems when callees are asking if the processor is done working by using IsWorking()
	// the next frame after they've executed the processor.
	processor->working = true;

	if ( this->processor != NULL ) {
		if ( !verify( this->processor != processor ) ) {
			hfLib::Warning( "[hfSaveGameManager::ExecuteProcessor]:1 Someone is trying to execute this processor twice, this is really bad, learn patience padawan!" );
			return processor->GetHandle();
		} else {
			hfSaveGameProcessor ** localProcessor = processorQueue.Find( processor );
			if ( !verify( localProcessor == NULL ) ) {
				hfLib::Warning( "[hfSaveGameManager::ExecuteProcessor]:2 Someone is trying to execute this processor twice, this is really bad, learn patience padawan!" );
				return (*localProcessor)->GetHandle();
			}
		}
	}

	processorQueue.Append( processor );
	
	// Don't allow processors to start sub-processors.
	// They need to manage their own internal state.
	assert( hfLib::IsMainThread() );

	Sys_InterlockedIncrement( submittedProcessorHandle );
	processor->parms.handle = submittedProcessorHandle;

	return submittedProcessorHandle;
}

/*
========================
hfSaveGameManager::ExecuteProcessorAndWait
========================
*/
saveGameHandle_t hfSaveGameManager::ExecuteProcessorAndWait( hfSaveGameProcessor * processor ) {
	saveGameHandle_t handle = ExecuteProcessor( processor );
	if ( handle == 0 ) {
		return 0;
	}

	while ( !IsSaveGameCompletedFromHandle( handle ) ) {
		Pump();
		Sys_Sleep( 10 );
	}

	// One more pump to get the completed callback
	//Pump();

	return handle;
}

/*
========================
hfSaveGameManager::WaitForAllProcessors

Since the load & nextMap processors calls execute map change, we can't wait if they are the only ones in the queue
If there are only resettable processors in the queue or no items in the queue, don't wait.

We would need to overrideSimpleProcessorCheck if we were sure we had done something that would cause the processors
to bail out nicely.  Something like canceling a disc swap during a loading disc swap dialog...
========================
*/
void hfSaveGameManager::WaitForAllProcessors( bool overrideSimpleProcessorCheck ) {
	assert( hfLib::IsMainThread() );

	while ( IsWorking() || ( processorQueue.Num() > 0 ) ) {

		if ( !overrideSimpleProcessorCheck ) {
			// BEFORE WE WAIT, and potentially hang everything, make sure processors about to be executed won't sit and
			// wait for themselves to complete.
			// Since we pull off simple processors first, we can stop waiting when the processor being executed is not simple
			if ( processor != NULL ) {
				if ( !processor->IsSimpleProcessor() ) {
					break;
				}
			} else if ( !processorQueue[0]->IsSimpleProcessor() ) {
				break;
			}
		}

		saveThread.WaitForThread();
		Pump();
	}
}

/*
========================
hfSaveGameManager::CancelAllProcessors
========================
*/
void hfSaveGameManager::CancelAllProcessors( const bool forceCancelInFlightProcessor ) {
	assert( hfLib::IsMainThread() );

	cancel = true;
	
	if ( forceCancelInFlightProcessor ) {
		if ( processor != NULL ) {
			processor->GetSignal().Raise();
		}
	}

	Pump();	// must be called from the main thread
	Clear();
	cancel = false;
}

/*
========================
hfSaveGameManager::CancelToTerminate
========================
*/
void hfSaveGameManager::CancelToTerminate() {
	if ( processor != NULL ) {
		processor->parms.cancelled = true;
		processor->GetSignal().Raise();
		saveThread.WaitForThread();
	}
}

/*
========================
hfSaveGameManager::DeviceSelectorWaitingOnSaveRetry
========================
*/
bool hfSaveGameManager::DeviceSelectorWaitingOnSaveRetry() {

	if ( retryFolder == NULL ) {
		return false;
	}

	return ( hfStr::Icmp( retryFolder, "GAME-autosave" ) == 0 );
}

/*
========================
hfSaveGameManager::Set360RetrySaveAfterDeviceSelected
========================
*/
void hfSaveGameManager::Set360RetrySaveAfterDeviceSelected( const char * folder, const int64 bytes ) {
	retryFolder = folder;
	retryBytes = bytes;
}

/*
========================
hfSaveGameManager::ClearRetryInfo
========================
*/
void hfSaveGameManager::ClearRetryInfo() {
	retryFolder = NULL;
	retryBytes = 0;
}

/*
========================
hfSaveGameManager::RetrySave
========================
*/
void hfSaveGameManager::RetrySave() {
	bool isDialogActive = false;
	if ( DeviceSelectorWaitingOnSaveRetry() && !common->Dialog().HasDialogMsg( GDM_WARNING_FOR_NEW_DEVICE_ABOUT_TO_LOSE_PROGRESS, &isDialogActive ) ) {
		cmdSystem->AppendCommandText( "savegame autosave\n" );
	}
}

/*
========================
hfSaveGameManager::ShowRetySaveDialog
========================
*/
void hfSaveGameManager::ShowRetySaveDialog() {
	ShowRetySaveDialog( retryFolder, retryBytes );
}

/*
========================
hfSaveGameManager::ShowRetySaveDialog
========================
*/
void hfSaveGameManager::ShowRetySaveDialog( const char * folder, const int64 bytes ) {

	hfStaticList< hfSWFScriptFunction *, 4 > callbacks;
	hfStaticList< hfStrId, 4 > optionText;

	class hfSWFScriptFunction_Continue : public hfSWFScriptFunction_RefCounted {
	public:
		hfSWFScriptVar Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
			common->Dialog().ClearDialog( GDM_INSUFFICENT_STORAGE_SPACE );
			session->GetSaveGameManager().ClearRetryInfo();
			return hfSWFScriptVar();
		}
	};

	callbacks.Append( new (TAG_SWF) hfSWFScriptFunction_Continue() );
	optionText.Append( hfStrId( "#str_dlg_continue_without_saving" ) );



	// build custom space required string
	// #str_dlg_space_required ~= "There is insufficient storage available.  Please free %s and try again."
	hfStr format = hfStrId( "#str_dlg_space_required" ).GetLocalizedString();
	hfStr size;
	if ( bytes > ( 1024 * 1024 ) ) {
		const float roundUp = ( ( 1024.0f * 1024.0f / 10.0f )- 1.0f );
		size = va( "%.1f MB", ( roundUp + (float) bytes ) / ( 1024.0f * 1024.0f ) );
	} else {
		const float roundUp = 1024.0f - 1.0f;
		size = va( "%.0f KB", ( roundUp + (float) bytes ) / 1024.0f );
	}
	hfStr msg = va( format.c_str(), size.c_str() );

	common->Dialog().AddDynamicDialog( GDM_INSUFFICENT_STORAGE_SPACE, callbacks, optionText, true, msg, true );
}

/*
========================
hfSaveGameManager::CancelWithHandle
========================
*/
void hfSaveGameManager::CancelWithHandle( const saveGameHandle_t & handle ) {
	if ( handle == 0 || IsSaveGameCompletedFromHandle( handle ) ) {
		return;
	}

	// check processor in flight first
	if ( processor != NULL ) {
		if ( processor->GetHandle() == handle ) {
			processor->Cancel();
			return;
		}
	}

	// remove from queue
	for ( int i = 0; i < processorQueue.Num(); ++i ) {
		if ( processorQueue[i]->GetHandle() == handle ) {
			processorQueue[i]->Cancel();
			return;
		}
	}
}

/*
========================
hfSaveGameManager::StartNextProcessor

Get the next not-reset-capable processor.  If there aren't any left, just get what's next.
========================
*/
void hfSaveGameManager::StartNextProcessor() {
	if ( cancel ) {
		return;
	}

	hfSaveGameProcessor * nextProcessor = NULL;
	int index = 0;

	// pick off the first simple processor
	for ( int i = 0; i < processorQueue.Num(); ++i ) {
		if ( processorQueue[i]->IsSimpleProcessor() ) {
			index = i;
			break;
		}
	}

	if ( processorQueue.Num() > 0 ) {
		nextProcessor = processorQueue[index];


		Sys_InterlockedIncrement( executingProcessorHandle );

		processorQueue.RemoveIndex( index );
		processor = nextProcessor;
		processor->parms.callbackSignal.Raise();	// signal that the thread is ready for work
		startTime = Sys_Milliseconds();
	}
}

/*
========================
hfSaveGameManager::FinishProcessor
========================
*/
void hfSaveGameManager::FinishProcessor( hfSaveGameProcessor * localProcessor ) {

	assert( localProcessor != NULL );
	hfLib::PrintfIf( saveGame_verbose.GetBool(), "[%s] : %s, %d ms\n", __FUNCTION__, localProcessor->Name(), Sys_Milliseconds() - startTime );

	// This will delete from the files set for auto-deletion
	// Don't remove files not set for auto-deletion, they may be used outside of the savegame manager by game-side callbacks for example
	for ( int i = ( localProcessor->parms.files.Num() - 1 ); i >= 0; --i ) {
		if ( localProcessor->parms.files[i]->type & SAVEGAMEFILE_AUTO_DELETE ) {
			delete localProcessor->parms.files[i];
			localProcessor->parms.files.RemoveIndexFast( i );
		}
	}

	localProcessor->init = false;
	localProcessor = NULL;
}

/*
========================
hfSaveGameManager::Clear
========================
*/
void hfSaveGameManager::Clear() {
	processorQueue.Clear();
}

/*
========================
hfSaveGameManager::IsWorking
========================
*/
bool hfSaveGameManager::IsWorking() const {
	return processor != NULL;
}

/*
========================
hfSaveGameManager::Pump

Important sections called out with -- EXTRA LARGE -- comments!
========================
*/
void hfSaveGameManager::Pump() {



	// After a processor is done, the next is pulled off the queue so the only way the manager isn't working is if
	// there isn't something executing or in the queue.
	if ( !IsWorking() ) {
		// Unified start to initialize system on PS3 and do appropriate checks for system combination issues
		// ------------------------------------
		// START
		// ------------------------------------
		StartNextProcessor();

		if ( !IsWorking() ) {
			return;
		}

		continueProcessing = true;
	}

	if ( cancel ) {
		processor->parms.AbortSaveGameFilePipeline();
	}

	// Quickly checks to see if the savegame thread is done, otherwise, exit and continue frame commands
	if ( processor->IsThreadFinished() ) {
		hfLib::PrintfIf( saveGame_verbose.GetBool(), "%s waited on processor [%s], error = 0x%08X, %s\n", __FUNCTION__, processor->Name(), processor->GetError(), GetSaveGameErrorString( processor->GetError() ).c_str() );

		if ( !cancel && continueProcessing ) {
			// Check for available storage unit
			if ( session->GetSignInManager().GetMasterLocalUser() != NULL ) {
				if ( !session->GetSignInManager().GetMasterLocalUser()->IsStorageDeviceAvailable() ) {
					// this will not allow further processing
					processor->parms.errorCode = SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE;
				}
			}

			// Execute Process() on the processor, if there was an error in a previous Process() call, give the
			// processor the chance to validate that error and either clean itself up or convert it to another error or none.
			if ( processor->GetError() == SAVEGAME_E_NONE || processor->ValidateLastError() ) {
				hfLib::PrintfIf( saveGame_verbose.GetBool(), "%s calling %s::Process(), error = 0x%08X, %s\n", __FUNCTION__, processor->Name(), processor->GetError(), GetSaveGameErrorString( processor->GetError() ).c_str() );

				// ------------------------------------
				// PROCESS
				// ------------------------------------
				continueProcessing = processor->Process();

				// If we don't return here, the completedCallback will be executed before it's done with it's async operation
				// during it's last process stage.
				return;
			} else { 
				continueProcessing = false;
			}
		}

		// This section does specific post-processing for each of the save commands
		if ( !continueProcessing ) {

			// Clear out details if we detect corruption but keep directory/slot information
			for ( int i = 0; i < processor->parms.detailList.Num(); ++i ) {
				hfSaveGameDetails & details = processor->parms.detailList[i];
				if ( details.damaged ) {
					details.descriptors.Clear();
				}
			}

			hfLib::PrintfIf( saveGame_verbose.GetBool(), "%s calling %s::CompletedCallback()\n", __FUNCTION__, processor->Name() );
			processor->working = false;

			// This ensures that the savegame manager will believe the processor is done when there is a potentially
			// catastrophic thing that will happen within CompletedCallback which might try to sync all threads
			// The most common case of this is executing a map change (which we no longer do).
			// We flush the heap and wait for all background processes to finish.  After all this is called, we will 
			// cleanup the old processor within FinishProcessor()
			hfSaveGameProcessor * localProcessor = processor;
			processor = NULL;

			// ------------------------------------
			// COMPLETEDCALLBACK
			// At this point, the handle will be completed
			// ------------------------------------
			Sys_InterlockedIncrement( lastExecutedProcessorHandle );
			
			for ( int i = 0; i < localProcessor->completedCallbacks.Num(); i++ ) {
				localProcessor->completedCallbacks[i]->Call();
			}
			localProcessor->completedCallbacks.DeleteContents( true );

			// ------------------------------------
			// FINISHPROCESSOR
			// ------------------------------------
			FinishProcessor( localProcessor );
		}
	} else if ( processor->ShouldTimeout() ) {
		// Hack for the PS3 threading hang
		hfLib::PrintfIf( saveGame_verbose.GetBool(), "----- PROCESSOR TIMEOUT ----- (%s)\n", processor->Name() );

		hfSaveGameProcessor * tempProcessor = processor;

		CancelAllProcessors( true );

		class hfSWFScriptFunction_TryAgain : public hfSWFScriptFunction_RefCounted {
		public:
			hfSWFScriptFunction_TryAgain( hfSaveGameManager * manager, hfSaveGameProcessor * processor ) {
				this->manager = manager;
				this->processor = processor; 
			}
			hfSWFScriptVar Call ( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) {
				common->Dialog().ClearDialog( GDM_ERROR_SAVING_SAVEGAME );
				manager->ExecuteProcessor( processor );
				return hfSWFScriptVar();
			}
		private:
			hfSaveGameManager * manager;
			hfSaveGameProcessor * processor;
		};

		hfStaticList< hfSWFScriptFunction *, 4 > callbacks;
		hfStaticList< hfStrId, 4 > optionText;
		callbacks.Append( new (TAG_SWF) hfSWFScriptFunction_TryAgain( this, tempProcessor ) );
		optionText.Append( hfStrId( "#STR_SWF_RETRY" ) );
		common->Dialog().AddDynamicDialog( GDM_ERROR_SAVING_SAVEGAME, callbacks, optionText, true, "" );
	}
}

