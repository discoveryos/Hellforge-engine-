#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfsys_lobby_backend.h"
#include "hfsys_lobby_backend_direct.h"

extern hfCVar net_port;

extern hfLobbyToSessionCB * lobbyToSessionCB;

/*
========================
hfLobbyBackendDirect::hfLobbyBackendWin
========================
*/
hfLobbyBackendDirect::hfLobbyBackendDirect() {
	state = STATE_INVALID;
}

/*
========================
hfLobbyBackendDirect::StartHosting
========================
*/
void hfLobbyBackendDirect::StartHosting( const hfMatchParameters & p, float skillLevel, lobbyBackendType_t type ) {
	NET_VERBOSE_PRINT( "hfLobbyBackendDirect::StartHosting\n" );

	isLocal = MatchTypeIsLocal( p.matchFlags );
	isHost	= true;

	state	= STATE_READY;
	isLocal = true;
}

/*
========================
hfLobbyBackendDirect::StartFinding
========================
*/
void hfLobbyBackendDirect::StartFinding( const hfMatchParameters & p, int numPartyUsers, float skillLevel ) {
	isLocal = MatchTypeIsLocal( p.matchFlags );
	isHost	= false;

	if ( lobbyToSessionCB->CanJoinLocalHost() ) {
		state = STATE_READY;
	} else {
		state = STATE_FAILED;
	}
}

/*
========================
hfLobbyBackendDirect::GetSearchResults
========================
*/
void hfLobbyBackendDirect::GetSearchResults( hfList< lobbyConnectInfo_t > & searchResults ) {
	lobbyConnectInfo_t fakeResult;
	searchResults.Clear();
	searchResults.Append( fakeResult );
}

/*
========================
hfLobbyBackendDirect::JoinFromConnectInfo
========================
*/
void hfLobbyBackendDirect::JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo ) {
	if ( lobbyToSessionCB->CanJoinLocalHost() ) {
		Sys_StringToNetAdr( "localhost", &address, true );
		address.port = net_port.GetInteger();
	} else {
		address = connectInfo.netAddr;
	}

	state		= STATE_READY;
	isLocal		= false;
	isHost		= false;
}

/*
========================
hfLobbyBackendDirect::Shutdown
========================
*/
void hfLobbyBackendDirect::Shutdown() {
	state = STATE_SHUTDOWN;
}

/*
========================
hfLobbyBackendDirect::BecomeHost
========================
*/
void hfLobbyBackendDirect::BecomeHost( int numInvites ) {
}

/*
========================
hfLobbyBackendDirect::FinishBecomeHost
========================
*/
void hfLobbyBackendDirect::FinishBecomeHost() {
	isHost = true;
}

/*
========================
hfLobbyBackendDirect::GetOwnerAddress
========================
*/
void hfLobbyBackendDirect::GetOwnerAddress( lobbyAddress_t & outAddr ) {
	outAddr.netAddr = address;
	state			= STATE_READY;
}

/*
========================
hfLobbyBackendDirect::SetIsJoinable
========================
*/
void hfLobbyBackendDirect::SetIsJoinable( bool joinable ) {
}

/*
========================
hfLobbyBackendDirect::GetConnectInfo
========================
*/
lobbyConnectInfo_t hfLobbyBackendDirect::GetConnectInfo() {
	lobbyConnectInfo_t connectInfo;

	// If we aren't the host, this lobby should have been joined through JoinFromConnectInfo
	if ( IsHost() ) {
		// If we are the host, give them our ip address
		const char * ip = Sys_GetLocalIP( 0 );
		Sys_StringToNetAdr( ip, &address, false );
		address.port = net_port.GetInteger();
	}

	connectInfo.netAddr = address;

	return connectInfo;
}

/*
========================
hfLobbyBackendDirect::IsOwnerOfConnectInfo
========================
*/
bool hfLobbyBackendDirect::IsOwnerOfConnectInfo( const lobbyConnectInfo_t & connectInfo ) const {
	return Sys_CompareNetAdrBase( address, connectInfo.netAddr );
}

/*
========================
hfLobbyBackendDirect::Pump
========================
*/
void hfLobbyBackendDirect::Pump() {
}

/*
========================
hfLobbyBackendDirect::UpdateMatchParms
========================
*/
void hfLobbyBackendDirect::UpdateMatchParms( const hfMatchParameters & p ) {
}

/*
========================
hfLobbyBackendDirect::UpdateLobbySkill
========================
*/
void hfLobbyBackendDirect::UpdateLobbySkill( float lobbySkill ) {
}

/*
========================
hfLobbyBackendDirect::SetInGame
========================
*/
void hfLobbyBackendDirect::SetInGame( bool value ) {
}

/*
========================
hfLobbyBackendDirect::RegisterUser
========================
*/
void hfLobbyBackendDirect::RegisterUser( lobbyUser_t * user, bool isLocal ) {
}

/*
========================
hfLobbyBackendDirect::UnregisterUser
========================
*/
void hfLobbyBackendDirect::UnregisterUser( lobbyUser_t * user, bool isLocal ) {
}

