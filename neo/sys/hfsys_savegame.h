#ifndef __SYS_SAVEGAME_H__
#define __SYS_SAVEGAME_H__

#ifdef OUTPUT_FUNC
#undef OUTPUT_FUNC
#endif
#ifdef OUTPUT_FUNC_EXIT
#undef OUTPUT_FUNC_EXIT
#endif
#define OUTPUT_FUNC()				hfLib::PrintfIf( saveGame_verbose.GetBool(), "[%s] Enter\n", __FUNCTION__ )
#define OUTPUT_FUNC_EXIT()			hfLib::PrintfIf( saveGame_verbose.GetBool(), "[%s] Exit\n", __FUNCTION__ )


#define DEFINE_CLASS( x )					virtual const char * Name() const { return #x; }
#define MAX_SAVEGAMES						16
#define MAX_FILES_WITHIN_SAVEGAME			10
#define MIN_SAVEGAME_SIZE_BYTES				( 4 * 1024 * 1024 )
#define MAX_SAVEGAME_STRING_TABLE_SIZE		400 * 1024	// 400 kB max string table size


#define MAX_FILENAME_LENGTH					255
#define MAX_FILENAME_LENGTH_PATTERN			8
#define MAX_FOLDER_NAME_LENGTH				64
#define SAVEGAME_DETAILS_FILENAME			"game.details"

// PS3 restrictions:  The only characters that can be used are 0-9 (numbers), A-Z (uppercase alphabet), "_" (underscore), and "-" (hyphen)
#define SAVEGAME_AUTOSAVE_FOLDER			"AUTOSAVE"		// auto save slot

// common descriptors for savegame description fields
#define SAVEGAME_DETAIL_FIELD_EXPANSION		"expansion"
#define SAVEGAME_DETAIL_FIELD_MAP			"mapName"
#define SAVEGAME_DETAIL_FIELD_MAP_LOCATE	"mapLocation"
#define SAVEGAME_DETAIL_FIELD_DIFFICULTY	"difficulty"
#define SAVEGAME_DETAIL_FIELD_PLAYTIME		"playTime"
#define SAVEGAME_DETAIL_FIELD_LANGUAGE		"language"
#define	SAVEGAME_DETAIL_FIELD_SAVE_VERSION	"saveVersion"
#define	SAVEGAME_DETAIL_FIELD_CHECKSUM		"checksum"

#define SAVEGAME_GAME_DIRECTORY_PREFIX		"GAME-"
#define SAVEGAME_PROFILE_DIRECTORY_PREFIX	""
#define SAVEGAME_RAW_DIRECTORY_PREFIX		""


extern hfCVar saveGame_verbose;
extern hfCVar saveGame_enable;

class hfGameSpawnInfo;
class hfSession;
class hfSessionLocal;
class hfSaveGameManager;

// Specific savegame sub-system errors
enum saveGameError_t {
	SAVEGAME_E_NONE								= 0,
	SAVEGAME_E_CANCELLED						= BIT( 0 ),
	SAVEGAME_E_INSUFFICIENT_ROOM				= BIT( 1 ),
	SAVEGAME_E_CORRUPTED						= BIT( 2 ),
	SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE	= BIT( 3 ),
	SAVEGAME_E_UNKNOWN							= BIT( 4 ),
	SAVEGAME_E_INVALID_FILENAME					= BIT( 5 ),
	SAVEGAME_E_STEAM_ERROR						= BIT( 6 ),
	SAVEGAME_E_FOLDER_NOT_FOUND					= BIT( 7 ),
	SAVEGAME_E_FILE_NOT_FOUND					= BIT( 8 ),
	SAVEGAME_E_DLC_NOT_FOUND					= BIT( 9 ),
	SAVEGAME_E_INVALID_USER						= BIT( 10 ),
	SAVEGAME_E_PROFILE_TOO_BIG					= BIT( 11 ),
	SAVEGAME_E_DISC_SWAP						= BIT( 12 ),
	SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION		= BIT( 13 ),

	SAVEGAME_E_BITS_USED						= 14,
	SAVEGAME_E_NUM								= SAVEGAME_E_BITS_USED + 1	// because we're counting "none"
};

// Modes to control behavior of savegame manager
enum saveGameModeBitfield_t {
	SAVEGAME_MBF_NONE				= 0,
	SAVEGAME_MBF_LOAD				= BIT( 0 ),		// standard file load (can be individual/multiple files described in parms)
	SAVEGAME_MBF_SAVE				= BIT( 1 ),		// standard file save (can be individual/multiple files described in parms)
	SAVEGAME_MBF_DELETE_FOLDER		= BIT( 2 ),		// standard package delete
	SAVEGAME_MBF_DELETE_ALL_FOLDERS	= BIT( 3 ),		// deletes all of the savegame folders (should only be used in testing)
	SAVEGAME_MBF_ENUMERATE			= BIT( 4 ),		// gets listing of all savegame folders, typically used with READ_DETAILS to read the description file
	SAVEGAME_MBF_NO_COMPRESS		= BIT( 5 ),		// tells the system the files aren't compressed, usually only needed when reading the descriptors file internally
	SAVEGAME_MBF_ENUMERATE_FILES	= BIT( 6 ),		// enumerates all the files within a particular savegame folder (can be individual/multiple files or pattern described in parms)
	SAVEGAME_MBF_DELETE_FILES		= BIT( 7 ),		// deletes individual files within a particular savegame folder (can be individual/multiple files or pattern described in parms)
	SAVEGAME_MBF_READ_DETAILS		= BIT( 8 ),		// reads the description file (if specified, parms.enumeratedEntry.name & parms.enumeratedEntry.type must be specified)
	SAVEGAME_MBF_KEEP_FOLDER		= BIT( 9 )		// don't delete the folder before saving
};

typedef interlockedInt_t saveGameHandle_t;

typedef int savegameUserId_t;		// [internal] hash of gamer tag for steam

/*
================================================
saveGameCheck_t
================================================
*/
struct saveGameCheck_t {
	saveGameCheck_t() { 
		exists = false;
		autosaveExists = false;
		autosaveFolder = NULL;
	}
	bool exists;
	bool autosaveExists;
	const char * autosaveFolder;
};

/*
================================================
hfSaveGameDetails 
================================================
*/
class hfSaveGameDetails {
public:
	hfSaveGameDetails();
	~hfSaveGameDetails() { Clear(); }

	void	Clear();
	bool	operator==( const hfSaveGameDetails & other ) const { return ( hfStr::Icmp( slotName, other.slotName ) == 0 ); }
	hfSaveGameDetails &	operator=( const hfSaveGameDetails &other ) {
		descriptors.Clear();
		descriptors = other.descriptors;
		damaged = other.damaged;
		date = other.date;
		slotName = other.slotName;
		return *this;
	}
	// for std::sort, sort newer (larger date) towards start of list
	bool	operator<( const hfSaveGameDetails & other ) { return date > other.date; }

	hfStr	GetMapName() const { return descriptors.GetString( SAVEGAME_DETAIL_FIELD_MAP, "" ); }
	hfStr	GetLocation() const { return descriptors.GetString( SAVEGAME_DETAIL_FIELD_MAP_LOCATE, "" ); }
	hfStr	GetLanguage() const { return descriptors.GetString( SAVEGAME_DETAIL_FIELD_LANGUAGE, "" ); }
	int		GetPlaytime() const { return descriptors.GetInt( SAVEGAME_DETAIL_FIELD_PLAYTIME, 0 ); }
	int		GetExpansion() const { return descriptors.GetInt( SAVEGAME_DETAIL_FIELD_EXPANSION, 0 ); }
	int		GetDifficulty() const { return descriptors.GetInt( SAVEGAME_DETAIL_FIELD_DIFFICULTY, -1 ); }
	int		GetSaveVersion() const { return descriptors.GetInt( SAVEGAME_DETAIL_FIELD_SAVE_VERSION, 0 ); }

public:
	hfDict				descriptors;						// [in] Descriptors available to be shown on the save/load screen.  Each game can define their own, e.g. Difficulty, level, map, score, time.
	bool				damaged;							// [out]
	time_t				date;								// [out] read from the filesystem, not set by client
	hfStrStatic< MAX_FOLDER_NAME_LENGTH >	slotName;		// [out] folder/slot name, e.g. AUTOSAVE
};

typedef hfStaticList< hfSaveGameDetails, MAX_SAVEGAMES > saveGameDetailsList_t;

// Making a auto_ptr to handle lifetime issues better
typedef hfList< hfFile_SaveGame *, TAG_SAVEGAMES > saveFileEntryList_t;

/*
================================================
hfSaveLoadParms 
================================================
*/
class hfSaveLoadParms {
public:
								hfSaveLoadParms();
								~hfSaveLoadParms();

	void						ResetCancelled();
	void						Init();
	void						SetDefaults( int inputDevice = -1 );	// doesn't clear out things that should be persistent across entire processor
	void						CancelSaveGameFilePipelines();
	void						AbortSaveGameFilePipeline();
	const int &					GetError() const { return errorCode; }
	const int &					GetHandledErrors() const { return handledErrorCodes; }
	const saveGameHandle_t &	GetHandle() const { return handle; }

public:
	hfStrStatic< MAX_FOLDER_NAME_LENGTH >		directory;			// [in] real directory of the savegame package
	hfStrStatic< MAX_FILENAME_LENGTH_PATTERN >	pattern;			// [in] pattern to use while enumerating/deleting files within a savegame folder
	hfStrStatic< MAX_FILENAME_LENGTH_PATTERN >	postPattern;		// [in] pattern at the end of the file to use while enumerating/deleting files within a savegame folder

	int									mode;						// [in] SAVE, LOAD, ENUM, DELETE, etc.
	hfSaveGameDetails					description;				// [in/out] in: description used to serialize into game.details file, out: if SAVEGAME_MBF_READ_DETAILS used with certain modes, item 0 contains the read details
	saveFileEntryList_t					files;						// [in/out] in: files to be saved, out: objects loaded, for SAVEGAME_MBF_ENUMERATE_FILES, it contains a listing of the filenames only
	saveGameDetailsList_t				detailList;					// [out] listing of the enumerated savegames used only with SAVEGAME_MBF_ENUMERATE

	int									errorCode;					// [out] combination of saveGameError_t bits
	int									handledErrorCodes;			// [out] combination of saveGameError_t bits
	int64								requiredSpaceInBytes;		// [out] when fails for insufficient space, this is populated with additional space required
	int									skipErrorDialogMask;

	// ----------------------
	// Internal vars
	// ----------------------
	hfSysSignal							callbackSignal;				// [internal] used to signal savegame manager that the Process() call is completed (we still might have more Process() calls to make though...)
	volatile bool						cancelled;					// [internal] while processor is running, this can be set outside of the normal operation of the processor.  Each implementation should check this during operation to allow it to shutdown cleanly.
	savegameUserId_t					userId;						// [internal] to get the proper user during every step
	int									inputDeviceId;				// [internal] consoles will use this to segregate each player's files
	saveGameHandle_t					handle;

private:
	// Don't allow copies
						hfSaveLoadParms( const hfSaveLoadParms & s ) {}
	void				operator=( const hfSaveLoadParms & s ) {}
};

// Using function pointers because:
// 1. CompletedCallback methods in processors weren't generic enough, we could use SaveFiles processors
//		for profiles/games, but there would be a single completed callback and we'd have to update
//		the callback to detect what type of call it was, store the type in the processor, etc.
// 2. Using a functor class would require us to define classes for each callback.  The definition of those
//		classes could be scattered and a little difficult to follow
// 3. With callback methods, we assign them when needed and know exactly where they are defined/declared.
//typedef void (*saveGameProcessorCallback_t)( hfSaveLoadParms & parms );

/*
================================================
saveGameThreadArgs_t 
================================================
*/
struct saveGameThreadArgs_t {
	saveGameThreadArgs_t() :
		saveLoadParms( NULL ) {
	}
	

	hfSaveLoadParms *		saveLoadParms;
};

/*
================================================
hfSaveGameThread 
================================================
*/
class hfSaveGameThread : public hfSysThread {
public:
			hfSaveGameThread() : cancel( false ) {}

	int		Run();
	void	CancelOperations() { cancel = true; }

private:
	int		Save();
	int		Load();
	int		Enumerate();
	int		Delete();
	int		DeleteAll();
	int		DeleteFiles();
	int		EnumerateFiles();

public:
	saveGameThreadArgs_t	data;
	volatile bool			cancel;
};

/*
================================================
hfSaveGameProcessor 
================================================
*/
class hfSaveGameProcessor {
	friend class hfSaveGameManager;

public:
	DEFINE_CLASS( hfSaveGameProcessor );
	static const int MAX_COMPLETED_CALLBACKS = 5;

							hfSaveGameProcessor();
	virtual					~hfSaveGameProcessor() { }

	//------------------------
	// Virtuals
	//------------------------
	// Basic init
	virtual bool			Init();

	// This method should returns true if the processor has additional sub-states to 
	// manage.  The saveGameManager will retain the current state and Process() will be called again. When this method
	// returns false Process() will not be called again. For example, during save, you might want to load other files
	// and save them somewhere else, return true until you are done with the entire state.
	virtual bool			Process() { return false; }

	// Gives each processor to validate an error returned from the previous process call.
	// This is useful when processors have a multi-stage Process() and expect some benign errors like
	// deleting a savegame folder before copying into it.
	virtual bool			ValidateLastError() { return false; }

	// Processors need to override this if they will eventually reset the map.
	// If it could possibly reset the map through any of its stages, including kicking off another processor in completed callback, return false.
	// We will force non-simple processors to execute last and won't block the map heap reset due if non-simple processors are still executing.
	virtual bool			IsSimpleProcessor() const { return true; }

	// This is a fail-safe to catch a timing issue on the PS3 where the nextmap processor could sometimes hang during a level transition
	virtual bool			ShouldTimeout() const { return false; }

	//------------------------
	// Commands
	//------------------------
	// Cancels this processor in whatever state it's currently in and sets an error code for SAVEGAME_E_CANCELLED
	void					Cancel() { parms.cancelled = true; parms.errorCode = SAVEGAME_E_CANCELLED; }

	//------------------------
	// Accessors
	//------------------------
	// Returns error status
	hfSysSignal &			GetSignal() { return parms.callbackSignal; }

	// Returns error status
	const int &				GetError() const { return parms.errorCode; }

	// Returns the processor's save/load parms
	const hfSaveLoadParms & GetParms() const { return parms; }

	// Returns the processor's save/load parms
	hfSaveLoadParms &		GetParmsNonConst() { return parms; }

	// Returns if this processor is currently working
	bool					IsWorking() const { return working; }
	
	// This is a way to tell the processor which errors shouldn't be handled by the processor or system.
	void					SetSkipSystemErrorDialogMask( const int errorMask ) { parms.skipErrorDialogMask = errorMask; }
	int						GetSkipSystemErrorDialogMask() const { return parms.skipErrorDialogMask; }

	// Returns the handle given by execution
	saveGameHandle_t		GetHandle() const { return parms.GetHandle(); }

	// These can be overridden by game code, like the GUI, when the processor is done executing.
	// Game classes like the GUI can create a processor derived from a game's Save processor impl and simply use
	// this method to know when everything is done.  It eases the burden of constantly checking the working flag.
	// This will be called back within the game thread during SaveGameManager::Pump().
	void					AddCompletedCallback( const hfCallback & callback );

private:
	// Returns whether or not the thread is finished operating, should only be called by the savegame manager
	bool					IsThreadFinished();

protected:
	hfSaveLoadParms		parms;
	int					savegameLogicTestIterator;

private:
	bool				init;	
	bool				working;

	hfStaticList< hfCallback *, MAX_COMPLETED_CALLBACKS >	completedCallbacks;
};

/*
================================================
hfSaveGameManager 

Why all the object-oriented nonsense?
- Savegames need to be processed asynchronously, saving/loading/deleting files should happen during the game frame
	so there is a common way to update the render device.
- When executing commands, if no "strategy"s are used, the pump() method would need to have a switch statement,
	extending the manager for other commands would mean modifying the manager itself for various commands.
	By making it a strategy, we are able to create custom commands and define the behavior within game code and keep
	the manager code in the engine static.
================================================
*/
class hfSaveGameManager {
public:
	enum packageType_t {
		PACKAGE_PROFILE,
		PACKAGE_GAME,
		PACKAGE_RAW,
		PACKAGE_NUM
	};

	const static int MAX_SAVEGAME_DIRECTORY_DEPTH = 5;

	explicit				hfSaveGameManager();
							~hfSaveGameManager();

	// Called within main game thread
	void					Pump();

	// Has the storage device been selected yet?  This is only an issue on the 360, and primarily for development purposes
	bool					IsStorageAvailable() const { return storageAvailable; }
	void					SetStorageAvailable( const bool available ) { storageAvailable = available; }

	// Check to see if a processor is set within the manager
	bool					IsWorking() const;

	// Assign a processor to the manager.  The processor should belong in game-side code
	// This queues up processors and executes them serially
	// Returns whether or not the processor is immediately executed
	saveGameHandle_t		ExecuteProcessor( hfSaveGameProcessor * processor );
	
	// Synchronous version, CompletedCallback is NOT called.
	saveGameHandle_t		ExecuteProcessorAndWait( hfSaveGameProcessor * processor );

	// Lets the currently processing queue finish, but clears the processor queue
	void					Clear();

	void					WaitForAllProcessors( bool overrideSimpleProcessorCheck = false );

	const bool				IsCancelled() const { return cancel; }
	void					CancelAllProcessors( const bool forceCancelInFlightProcessor );

	void					CancelToTerminate();

	hfSaveGameThread &		GetSaveGameThread() { return saveThread; }

	bool					IsSaveGameCompletedFromHandle( const saveGameHandle_t & handle ) const { return handle <= lastExecutedProcessorHandle || handle == 0; }	// last case should never be reached since it would be also be true in first case, this is just to show intent
	void					Set360RetrySaveAfterDeviceSelected( const char * folder, const int64 bytes );
	bool					DeviceSelectorWaitingOnSaveRetry();
	void					ShowRetySaveDialog( const char * folder, const int64 bytes );
	void					ShowRetySaveDialog();
	void					ClearRetryInfo();
	void					RetrySave();
	// This will cause the processor to cancel execution, the completion callback will be called 
	void					CancelWithHandle( const saveGameHandle_t & handle );

	const saveGameDetailsList_t & GetEnumeratedSavegames() const { return enumeratedSaveGames; }
	saveGameDetailsList_t & GetEnumeratedSavegamesNonConst() { return enumeratedSaveGames; }

private:
	// These are to make sure that all processors start and finish in the same way without a lot of code duplication.
	// We need to make sure that we adhere to PS3 system combination initialization issues.
	void					StartNextProcessor();
	void					FinishProcessor( hfSaveGameProcessor * processor );

	// Calls start on the processor after it's been assigned
	void					Start();

private:
	hfSaveGameProcessor *						processor;
	hfStaticList< hfSaveGameProcessor *, 4 >	processorQueue;
	bool										cancel;
	hfSaveGameThread							saveThread;
	int											startTime;
	bool										continueProcessing;
	saveGameHandle_t							submittedProcessorHandle;
	saveGameHandle_t							executingProcessorHandle;
	saveGameHandle_t							lastExecutedProcessorHandle;
	saveGameDetailsList_t						enumeratedSaveGames;
	bool										storageAvailable;				// On 360, this is false by default, after the storage device is selected
																				// it becomes true.  This allows us to start the game without a storage device
																				// selected and pop the selector when necessary.
	const char *								retryFolder;
	int64										retryBytes;
	bool										retrySave;
	hfSysSignal									deviceRequestedSignal;
};

// Bridge between the session's APIs and the savegame thread
void Sys_ExecuteSavegameCommandAsync( hfSaveLoadParms * savegameParms );

// Folder prefix should be NULL for everything except PS3
// Synchronous check, just checks if any savegame exists for master local user and if one is an autosave
void Sys_SaveGameCheck( bool & exists, bool & autosaveExists );

const hfStr & GetSaveFolder( hfSaveGameManager::packageType_t type );
hfStr AddSaveFolderPrefix( const char * folder, hfSaveGameManager::packageType_t type );
hfStr RemoveSaveFolderPrefix( const char * folder, hfSaveGameManager::packageType_t type );

bool SavegameReadDetailsFromFile( hfFile * file, hfSaveGameDetails & details );

hfStr GetSaveGameErrorString( int errorMask );

#endif // __SYS_SAVEGAME_H__

