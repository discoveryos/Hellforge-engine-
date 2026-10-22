/*
===========================================================================

        Hellforge Engine - Cross-platform game engine
        Powered by Hellforge Power 1
        Copyright (C) 1993-2026

        This file is part of the Hellforge Engine.

        Public game module interface.
        Any game can implement this interface to run on the engine.

===========================================================================
*/

#ifndef __GAMEMODULE_H__
#define __GAMEMODULE_H__

/*
==============================================================================

        Public game interface with methods to run the game.

==============================================================================
*/

// default scripts
#define SCRIPT_DEFAULTDEFS       "script/default_defs.script"
#define SCRIPT_DEFAULT           "script/default_main.script"
#define SCRIPT_DEFAULTFUNC       "default_main"

struct gameReturn_t {

	gameReturn_t() :
		syncNextGameFrame( false ),
		vibrationLow( 0 ),
		vibrationHigh( 0 ) {
	}

	char        sessionCommand[MAX_STRING_CHARS];	// "map", "disconnect", "victory", etc
	bool        syncNextGameFrame;					// used when cinematics are skipped to prevent session from simulating several game frames to
													// keep the game time in sync with real time
	int         vibrationLow;
	int         vibrationHigh;
};

#define TIME_GROUP1     0
#define TIME_GROUP2     1

class hfGame {
public:
	virtual                 ~hfGame() {}

	// Initialize the game for the first time.
	virtual void            Init() = 0;

	// Shut down the entire game.
	virtual void            Shutdown() = 0;

	// Sets the serverinfo at map loads and when it changes.
	virtual void            SetServerInfo( const hfDict &serverInfo ) = 0;

	// Gets the serverinfo, common calls this before saving the game
	virtual const hfDict &  GetServerInfo() = 0;

	// Interpolated server time
	virtual void            SetServerGameTimeMs( const int time ) = 0;

	// Interpolated server time
	virtual int             GetServerGameTimeMs() const = 0;

	virtual int             GetSSEndTime() const = 0;
	virtual int             GetSSStartTime() const = 0;

	// common calls this before moving the single player game to a new level.
	virtual const hfDict &  GetPersistentPlayerInfo( int clientNum ) = 0;

	// common calls this right before a new level is loaded.
	virtual void            SetPersistentPlayerInfo( int clientNum, const hfDict &playerInfo ) = 0;

	// Loads a map and spawns all the entities.
	virtual void            InitFromNewMap( const char *mapName, hfRenderWorld *renderWorld, hfSoundWorld *soundWorld, int gameMode, int randseed ) = 0;

	// Loads a map from a savegame file.
	virtual bool            InitFromSaveGame( const char *mapName, hfRenderWorld *renderWorld, hfSoundWorld *soundWorld, hfFile *saveGameFile, hfFile *stringTableFile, int saveGameVersion ) = 0;

	// Saves the current game state, common may have written some data to the file already.
	virtual void            SaveGame( hfFile *saveGameFile, hfFile *stringTableFile ) = 0;

	// Pulls the current player location from the game information
	virtual void            GetSaveGameDetails( hfSaveGameDetails & gameDetails ) = 0;

	// Shut down the current map.
	virtual void            MapShutdown() = 0;

	// Caches media referenced from in key/value pairs in the given dictionary.
	virtual void            CacheDictionaryMedia( const hfDict *dict ) = 0;

	virtual void            Preload( const hfPreloadManifest &manifest ) = 0;

	// Runs a game frame, may return a session command for level changing, etc
	virtual void            RunFrame( hfUserCmdMgr & cmdMgr, gameReturn_t & gameReturn ) = 0;

	// Makes rendering and sound system calls to display for a given clientNum.
	virtual bool            Draw( int clientNum ) = 0;

	virtual bool            HandlePlayerGuiEvent( const sysEvent_t * ev ) = 0;

	// Writes a snapshot of the server game state.
	virtual void            ServerWriteSnapshot( hfSnapShot & ss ) = 0;

	// Processes a reliable message
	virtual void            ProcessReliableMessage( int clientNum, int type, const hfBitMsg &msg ) = 0;

	virtual void            SetInterpolation( const float fraction, const int serverGameMS, const int ssStartTime, const int ssEndTime ) = 0;

	// Reads a snapshot and updates the client game state.
	virtual void            ClientReadSnapshot( const hfSnapShot & ss ) = 0;

	// Runs prediction on entities at the client.
	virtual void            ClientRunFrame( hfUserCmdMgr & cmdMgr, bool lastPredictFrame, gameReturn_t & ret ) = 0;

	// Used to manage divergent time-lines
	virtual int             GetTimeGroupTime( int timeGroup ) = 0;

	// Returns a list of available multiplayer game modes
	virtual int             GetMPGameModes( const char *** gameModes, const char *** gameModesDisplay ) = 0;

	// Returns a summary of stats for a given client
	virtual void            GetClientStats( int clientNum, char *data, const int len ) = 0;

	virtual bool            IsInGame() const = 0;

	// Get the player entity number for a network peer.
	virtual int             MapPeerToClient( int peer ) const = 0;

	// Get the player entity number of the local player.
	virtual int             GetLocalClientNum() const = 0;

	// compute an angle offset to be applied to the given client's aim
	virtual void            GetAimAssistAngles( hfAngles & angles ) = 0;
	virtual float           GetAimAssistSensitivity() = 0;

	// Release the mouse when an overlay is open
	virtual bool            IsPDAOpen() const = 0;
	virtual bool            IsPlayerChatting() const = 0;

	// Creates leaderboards for each map/mode defined.
	virtual void            Leaderboards_Init() = 0;
	virtual void            Leaderboards_Shutdown() = 0;

	// MAIN MENU FUNCTIONS
	virtual bool            InhibitControls() = 0;
	virtual void            Shell_Init( const char * filename, hfSoundWorld * sw ) = 0;
	virtual void            Shell_Cleanup() = 0;
	virtual void            Shell_CreateMenu( bool inGame ) = 0;
	virtual void            Shell_ClosePause() = 0;
	virtual void            Shell_Show( bool show ) = 0;
	virtual bool            Shell_IsActive() const = 0;
	virtual bool            Shell_HandleGuiEvent( const sysEvent_t * sev ) = 0;
	virtual void            Shell_Render() = 0;
	virtual void            Shell_ResetMenu() = 0;
	virtual void            Shell_SyncWithSession() = 0;
	virtual void            Shell_UpdateSavedGames() = 0;
	virtual void            Shell_SetCanContinue( bool valid ) = 0;
	virtual void            Shell_UpdateClientCountdown( int countdown ) = 0;
	virtual void            Shell_UpdateLeaderboard( const hfLeaderboardCallback * callback ) = 0;
	virtual void            Shell_SetGameComplete() = 0;
};

extern hfGame *             game;


/*
==============================================================================

        Game API.

==============================================================================
*/

const int GAME_API_VERSION      = 1;

typedef struct {

	int                     version;				// API version
	hfSys *                 sys;					// non-portable system services
	hfCommon *              common;					// common
	hfCmdSystem *           cmdSystem;				// console command system
	hfCVarSystem *          cvarSystem;				// console variable system
	hfFileSystem *          fileSystem;				// file system
	hfRenderSystem *        renderSystem;			// render system
	hfSoundSystem *         soundSystem;			// sound system
	hfRenderModelManager *  renderModelManager;		// render model manager
	hfUserInterfaceManager * uiManager;				// user interface manager
	hfDeclManager *         declManager;			// declaration manager
	hfAASFileManager *      AASFileManager;			// AAS file manager
	hfCollisionModelManager * collisionModelManager;	// collision model manager

} gameImport_t;

typedef struct {

	int                     version;				// API version
	hfGame *                game;					// interface to run the game

} gameExport_t;

extern "C" {
typedef gameExport_t * ( *GetGameAPI_t )( gameImport_t *import );
}

#endif /* !__GAMEMODULE_H__ */