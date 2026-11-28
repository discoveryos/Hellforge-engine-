#ifndef __HF_ARENA_STUB_GAME_H__
#define __HF_ARENA_STUB_GAME_H__

#include "hfGameModule.h"
#include "../hflib/game/hfArenaGame.h"

// Minimal built-in hfGame for the arena prototype.  It satisfies the engine
// session/map path so ui_startMap can load a real .proc through ExecuteMapChange
// while the Hellforge Arena simulation remains engine-owned and deterministic.
class hfArenaStubGame : public hfGame {
public:
	hfArenaStubGame() : inGame( false ), serverTime( 0 ), ssStart( 0 ), ssEnd( 0 ) {}

	virtual void Init() {}
	virtual void Shutdown() {}
	virtual void SetServerInfo( const hfDict &info ) { serverInfo = info; }
	virtual const hfDict & GetServerInfo() { return serverInfo; }
	virtual void SetServerGameTimeMs( const int time ) { serverTime = time; }
	virtual int GetServerGameTimeMs() const { return serverTime; }
	virtual int GetSSEndTime() const { return ssEnd; }
	virtual int GetSSStartTime() const { return ssStart; }
	virtual const hfDict & GetPersistentPlayerInfo( int ) { return persistent; }
	virtual void SetPersistentPlayerInfo( int, const hfDict &info ) { persistent = info; }
	virtual void InitFromNewMap( const char *mapName, hfRenderWorld *, hfSoundWorld *, int, int randseed ) {
		serverInfo.Set( "si_map", mapName ? mapName : "" );
		HfArenaGame::Start( randseed != 0 ? randseed : 1 );
		inGame = true;
	}
	virtual bool InitFromSaveGame( const char *, hfRenderWorld *, hfSoundWorld *, hfFile *, hfFile *, int ) { return false; }
	virtual void SaveGame( hfFile *, hfFile * ) {}
	virtual void GetSaveGameDetails( hfSaveGameDetails & ) {}
	virtual void MapShutdown() { inGame = false; }
	virtual void CacheDictionaryMedia( const hfDict * ) {}
	virtual void Preload( const hfPreloadManifest & ) {}
	virtual void RunFrame( hfUserCmdMgr &, gameReturn_t &ret ) {
		ret.sessionCommand[ 0 ] = '\0';
		ret.syncNextGameFrame = false;
		if ( inGame ) { HfArenaGame::StepAuto(); }
	}
	virtual bool Draw( int ) { return true; }
	virtual bool HandlePlayerGuiEvent( const sysEvent_t * ) { return false; }
	virtual void ServerWriteSnapshot( hfSnapShot & ) {}
	virtual void ProcessReliableMessage( int, int, const hfBitMsg & ) {}
	virtual void SetInterpolation( const float, const int, const int start, const int end ) { ssStart = start; ssEnd = end; }
	virtual void ClientReadSnapshot( const hfSnapShot & ) {}
	virtual void ClientRunFrame( hfUserCmdMgr &, bool, gameReturn_t &ret ) { ret.sessionCommand[ 0 ] = '\0'; }
	virtual int GetTimeGroupTime( int ) { return serverTime; }
	virtual int GetMPGameModes( const char *** modes, const char *** display ) {
		static const char *m[] = { "arena" };
		static const char *d[] = { "Arena" };
		if ( modes ) { *modes = m; }
		if ( display ) { *display = d; }
		return 1;
	}
	virtual void GetClientStats( int, char *data, const int len ) { if ( data && len > 0 ) { data[ 0 ] = '\0'; } }
	virtual bool IsInGame() const { return inGame; }
	virtual int MapPeerToClient( int ) const { return 0; }
	virtual int GetLocalClientNum() const { return 0; }
	virtual void GetAimAssistAngles( hfAngles & ) {}
	virtual float GetAimAssistSensitivity() { return 1.0f; }
	virtual bool IsPDAOpen() const { return false; }
	virtual bool IsPlayerChatting() const { return false; }
	virtual void Leaderboards_Init() {}
	virtual void Leaderboards_Shutdown() {}
	virtual bool InhibitControls() { return false; }
	virtual void Shell_Init( const char *, hfSoundWorld * ) {}
	virtual void Shell_Cleanup() {}
	virtual void Shell_CreateMenu( bool ) {}
	virtual void Shell_ClosePause() {}
	virtual void Shell_Show( bool ) {}
	virtual bool Shell_IsActive() const { return false; }
	virtual bool Shell_HandleGuiEvent( const sysEvent_t * ) { return false; }
	virtual void Shell_Render() {}
	virtual void Shell_ResetMenu() {}
	virtual void Shell_SyncWithSession() {}
	virtual void Shell_UpdateSavedGames() {}
	virtual void Shell_SetCanContinue( bool ) {}
	virtual void Shell_UpdateClientCountdown( int ) {}
	virtual void Shell_UpdateLeaderboard( const hfLeaderboardCallback * ) {}
	virtual void Shell_SetGameComplete() {}

private:
	bool inGame;
	int serverTime;
	int ssStart;
	int ssEnd;
	hfDict serverInfo;
	hfDict persistent;
};

inline hfArenaStubGame &HfArenaStubGameInstance() {
	static hfArenaStubGame instance;
	return instance;
}

#endif
