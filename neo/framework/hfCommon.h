#ifndef __COMMON_H__
#define __COMMON_H__

/*
==============================================================

  Common

==============================================================
*/

extern hfCVar com_engineHz;
extern float com_engineHz_latched;
extern int64 com_engineHz_numerator;
extern int64 com_engineHz_denominator;

// Returns the msec the frame starts on
HF_INLINE int FRAME_TO_MSEC( int64 frame ) {
	return (int)( ( frame * com_engineHz_numerator ) / com_engineHz_denominator );
}
// Rounds DOWN to the nearest frame
HF_INLINE int MSEC_TO_FRAME_FLOOR( int msec ) {
	return (int)( ( ( (int64)msec * com_engineHz_denominator ) + ( com_engineHz_denominator - 1 ) ) / com_engineHz_numerator );
}
// Rounds UP to the nearest frame
HF_INLINE int MSEC_TO_FRAME_CEIL( int msec ) {
	return (int)( ( ( (int64)msec * com_engineHz_denominator ) + ( com_engineHz_numerator - 1 ) ) / com_engineHz_numerator );
}
// Aligns msec so it starts on a frame bondary
HF_INLINE int MSEC_ALIGN_TO_FRAME( int msec ) {
	return FRAME_TO_MSEC( MSEC_TO_FRAME_CEIL( msec ) );
}

class hfGame;
class hfRenderWorld;
class hfSoundWorld;
class hfSession;
class hfCommonDialog;
class hfDemoFile;
class hfUserInterface;
class hfSaveLoadParms;
class hfMatchParameters;

struct lobbyConnectInfo_t;

HF_INLINE void BeginProfileNamedEventColor( uint32 color, VERIFY_FORMAT_STRING const char * szName ) {
}
HF_INLINE void EndProfileNamedEvent() {
}

HF_INLINE void BeginProfileNamedEvent( VERIFY_FORMAT_STRING const char * szName ) {
	BeginProfileNamedEventColor( (uint32) 0xFF00FF00, szName );
}

class hfScopedProfileEvent {
public:
	hfScopedProfileEvent( const char * name ) { BeginProfileNamedEvent( name ); }
	~hfScopedProfileEvent() { EndProfileNamedEvent(); }
};

#define SCOPED_PROFILE_EVENT( x ) hfScopedProfileEvent scopedProfileEvent_##__LINE__( x )

HF_INLINE bool BeginTraceRecording( char * szName ) {
	return false;
}

HF_INLINE bool EndTraceRecording() {
	return false;
}

typedef enum {
	EDITOR_NONE					= 0,
	EDITOR_RADIANT				= BIT(1),
	EDITOR_GUI					= BIT(2),
	EDITOR_DEBUGGER				= BIT(3),
	EDITOR_SCRIPT				= BIT(4),
	EDITOR_LIGHT				= BIT(5),
	EDITOR_SOUND				= BIT(6),
	EDITOR_DECL					= BIT(7),
	EDITOR_AF					= BIT(8),
	EDITOR_PARTICLE				= BIT(9),
	EDITOR_PDA					= BIT(10),
	EDITOR_AAS					= BIT(11),
	EDITOR_MATERIAL				= BIT(12)
} toolFlag_t;

#define STRTABLE_ID				"#str_"
#define STRTABLE_ID_LENGTH		5

extern hfCVar		com_version;
extern hfCVar		com_developer;
extern hfCVar		com_allowConsole;
extern hfCVar		com_speeds;
extern hfCVar		com_showFPS;
extern hfCVar		com_showMemoryUsage;
extern hfCVar		com_updateLoadSize;
extern hfCVar		com_productionMode;

struct MemInfo_t {
	hfStr			filebase;

	int				total;
	int				assetTotals;

	// memory manager totals
	int				memoryManagerTotal;

	// subsystem totals
	int				gameSubsystemTotal;
	int				renderSubsystemTotal;

	// asset totals
	int				imageAssetsTotal;
	int				modelAssetsTotal;
	int				soundAssetsTotal;
};

struct mpMap_t {

	void operator=( const mpMap_t & src ) {
		mapFile = src.mapFile;
		mapName = src.mapName;
		supportedModes = src.supportedModes;
	}

	hfStr			mapFile;
	hfStr			mapName;
	uint32			supportedModes;
};

static const int	MAX_LOGGED_STATS = 60 * 120;		// log every half second 

enum currentGame_t {
	HELLFORGE
};

class hfCommon {
public:
	virtual						~hfCommon() {}

								// Initialize everything.
								// if the OS allows, pass argc/argv directly (without executable name)
								// otherwise pass the command line in a single string (without executable name)
	virtual void				Init( int argc, const char * const * argv, const char *cmdline ) = 0;

								// Shuts down everything.
	virtual void				Shutdown() = 0;
	virtual bool				IsShuttingDown() const = 0;

	virtual	void				CreateMainMenu() = 0;

								// Shuts down everything.
	virtual void				Quit() = 0;

								// Returns true if common initialization is complete.
	virtual bool				IsInitialized() const = 0;

								// Called repeatedly as the foreground thread for rendering and game logic.
	virtual void				Frame() = 0;

	// Redraws the screen, handling games, guis, console, etc
	// in a modal manner outside the normal frame loop
	virtual void				UpdateScreen( bool captureToImage ) = 0;

	virtual void				UpdateLevelLoadPacifier() = 0;
	

								// Checks for and removes command line "+set var arg" constructs.
								// If match is NULL, all set commands will be executed, otherwise
								// only a set with the exact name.
	virtual void				StartupVariable( const char * match ) = 0;

								// Begins redirection of console output to the given buffer.
	virtual void				BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) = 0;

								// Stops redirection of console output.
	virtual void				EndRedirect() = 0;

								// Update the screen with every message printed.
	virtual void				SetRefreshOnPrint( bool set ) = 0;

								// Prints message to the console, which may cause a screen update if com_refreshOnPrint is set.
	virtual void				Printf( VERIFY_FORMAT_STRING const char *fmt, ... ) = 0;

								// Same as Printf, with a more usable API - Printf pipes to this.
	virtual void				VPrintf( const char *fmt, va_list arg ) = 0;

								// Prints message that only shows up if the "developer" cvar is set,
								// and NEVER forces a screen update, which could cause reentrancy problems.
	virtual void				DPrintf( VERIFY_FORMAT_STRING const char *fmt, ... ) = 0;

								// Prints WARNING %s message and adds the warning message to a queue for printing later on.
	virtual void				Warning( VERIFY_FORMAT_STRING const char *fmt, ... ) = 0;

								// Prints WARNING %s message in yellow that only shows up if the "developer" cvar is set.
	virtual void				DWarning( VERIFY_FORMAT_STRING const char *fmt, ...) = 0;

								// Prints all queued warnings.
	virtual void				PrintWarnings() = 0;

								// Removes all queued warnings.
	virtual void				ClearWarnings( const char *reason ) = 0;

								// Issues a C++ throw. Normal errors just abort to the game loop,
								// which is appropriate for media or dynamic logic errors.
	virtual void				Error( VERIFY_FORMAT_STRING const char *fmt, ... ) = 0;

								// Fatal errors quit all the way to a system dialog box, which is appropriate for
								// static internal errors or cases where the system may be corrupted.
	virtual void				FatalError( VERIFY_FORMAT_STRING const char *fmt, ... ) = 0;

								// Returns key bound to the command
	virtual const char *		KeysFromBinding( const char *bind ) = 0;

								// Returns the binding bound to the key
	virtual const char *		BindingFromKey( const char *key ) = 0; 

								// Directly sample a button.
	virtual int					ButtonState( int key ) = 0;

								// Directly sample a keystate.
	virtual int					KeyState( int key ) = 0;

	// Returns true if a multiplayer game is running.
	// CVars and commands are checked differently in multiplayer mode.
	virtual bool				IsMultiplayer() = 0;
	virtual bool				IsServer() = 0;
	virtual bool				IsClient() = 0;

	// Returns true if the player has ever enabled the console
	virtual bool				GetConsoleUsed() = 0;

	// Returns the rate (in ms between snaps) that we want to generate snapshots
	virtual int					GetSnapRate() = 0;

	virtual void				NetReceiveReliable( int peer, int type, hfBitMsg & msg ) = 0;
	virtual void				NetReceiveSnapshot( class hfSnapShot & ss ) = 0;
	virtual void				NetReceiveUsercmds( int peer, hfBitMsg & msg ) = 0;

	// Processes the given event.
	virtual	bool				ProcessEvent( const sysEvent_t * event ) = 0;

	virtual bool				LoadGame( const char * saveName ) = 0;
	virtual bool				SaveGame( const char * saveName ) = 0;

	virtual hfDemoFile *		ReadDemo() = 0;
	virtual hfDemoFile *		WriteDemo() = 0;

	virtual hfGame *			Game() = 0;
	virtual hfRenderWorld *		RW() = 0;
	virtual hfSoundWorld *		SW() = 0;
	virtual hfSoundWorld *		MenuSW() = 0;
	virtual hfSession *			Session() = 0;
	virtual hfCommonDialog &	Dialog() = 0;

	virtual void				OnSaveCompleted( hfSaveLoadParms & parms ) = 0;
	virtual void				OnLoadCompleted( hfSaveLoadParms & parms ) = 0;
	virtual void				OnLoadFilesCompleted( hfSaveLoadParms & parms ) = 0;
	virtual void				OnEnumerationCompleted( hfSaveLoadParms & parms ) = 0;
	virtual void				OnDeleteCompleted( hfSaveLoadParms & parms ) = 0;
	virtual void				TriggerScreenWipe( const char * _wipeMaterial, bool hold ) = 0;

	virtual void				OnStartHosting( hfMatchParameters & parms ) = 0;

	virtual int					GetGameFrame() = 0;

	virtual void				LaunchExternalTitle( int titleIndex, int device, const lobbyConnectInfo_t * const connectInfo ) = 0;

	virtual void				InitializeMPMapsModes() = 0;
	virtual const hfStrList &			GetModeList() const = 0;
	virtual const hfStrList &			GetModeDisplayList() const = 0;
	virtual const hfList<mpMap_t> &		GetMapList() const = 0;

	virtual void				ResetPlayerInput( int playerIndex ) = 0;

	virtual bool				JapaneseCensorship() const = 0;

	virtual void				QueueShowShell() = 0;		// Will activate the shell on the next frame.

	virtual currentGame_t		GetCurrentGame() const = 0;
	virtual void				SwitchToGame( currentGame_t newGame ) = 0;
};

extern hfCommon *		common;

#endif /* !__COMMON_H__ */

