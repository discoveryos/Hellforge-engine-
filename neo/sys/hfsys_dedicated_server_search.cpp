#pragma hdrstop

#include "../hflib/hfprecompiled.h"
#include "hfsys_lobby_backend.h"
#include "hfsys_dedicated_server_search.h"

/*
========================
hfDedicatedServerSearch::hfDedicatedServerSearch
========================
*/
hfDedicatedServerSearch::hfDedicatedServerSearch() :
	callback( NULL ) {
}

/*
========================
hfDedicatedServerSearch::~hfDedicatedServerSearch
========================
*/
hfDedicatedServerSearch::~hfDedicatedServerSearch() {
	if ( callback != NULL ) {
		delete callback;
	}
}



/*
========================
hfDedicatedServerSearch::StartSearch
========================
*/
void hfDedicatedServerSearch::StartSearch( const hfCallback & cb ) {
	Clear();
	callback = cb.Clone();
}

/*
========================
hfDedicatedServerSearch::Clear
========================
*/
void hfDedicatedServerSearch::Clear() {
	if ( callback != NULL ) {
		delete callback;
		callback = NULL;
	}
	list.Clear();
}

/*
========================
hfDedicatedServerSearch::Clear
========================
*/
void hfDedicatedServerSearch::HandleQueryAck( lobbyAddress_t & addr, hfBitMsg & msg ) {
	bool found = false;
	// Find the server this ack belongs to
	for ( int i = 0; i < list.Num(); i++ ) {
		serverInfoDedicated_t & query = list[i];


		if ( query.addr.Compare( addr ) ) {
			// Found the server
			found = true;
				
			bool canJoin = msg.ReadBool();
				
			if ( !canJoin ) {
				// If we can't join this server, then remove it
				list.RemoveIndex( i-- );
				break;
			}
				
			query.serverInfo.Read( msg );
			query.connectedPlayers.Clear();
			for ( int i = 0; i < query.serverInfo.numPlayers; i++ ) {
				hfStr user;
				msg.ReadString( user );
				query.connectedPlayers.Append( user );
			}
			break;
		}
	}

	if ( !found ) {
		bool canJoin = msg.ReadBool();
		if ( canJoin ) {
			serverInfoDedicated_t newServer;
			newServer.addr = addr;
			newServer.serverInfo.Read( msg );
			if ( newServer.serverInfo.serverName.IsEmpty() ) {
				newServer.serverInfo.serverName = addr.ToString();
			}
			newServer.connectedPlayers.Clear();
			for ( int i = 0; i < newServer.serverInfo.numPlayers; i++ ) {
				hfStr user;
				msg.ReadString( user );
				newServer.connectedPlayers.Append( user );
			}
			list.Append( newServer );
		}
	}


	if ( callback != NULL ) {
		callback->Call();
	}
}

/*
========================
hfDedicatedServerSearch::GetAddrAtIndex
========================
*/
bool hfDedicatedServerSearch::GetAddrAtIndex( netadr_t & addr, int i ) {
	if ( i >= 0 && i < list.Num() ) {
		addr = list[i].addr.netAddr;
		return true;
	}
	return false;
}

/*
========================
hfDedicatedServerSearch::DescribeServerAtIndex
========================
*/
const serverInfo_t * hfDedicatedServerSearch::DescribeServerAtIndex( int i ) const {
	if ( i >= 0 && i < list.Num() ) {
		return &list[i].serverInfo;
	}
	return NULL;
}

/*
========================
hfDedicatedServerSearch::GetServerPlayersAtIndex
========================
*/
const hfList< hfStr > * hfDedicatedServerSearch::GetServerPlayersAtIndex( int i ) const {
	if ( i >= 0 && i < list.Num() ) {
		return &list[i].connectedPlayers;
	}
	return NULL;
}

/*
========================
hfDedicatedServerSearch::NumServers
========================
*/
int hfDedicatedServerSearch::NumServers() const {
	return list.Num();
}
