#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfsys_lobby.h"
#include "hfsys_voicechat.h"




/*
========================
hfLobby::SaveDisconnectedUser
========================
*/
void hfLobby::SaveDisconnectedUser( const lobbyUser_t & user ) {
	bool found = false;
	for ( int i = 0; i < disconnectedUsers.Num(); i++ ) {
		if ( user.lobbyUserID.CompareIgnoreLobbyType( disconnectedUsers[i].lobbyUserID ) ) {
			found = true;
			memcpy( disconnectedUsers[i].gamertag, user.gamertag, sizeof( user.gamertag ) );
		}
	}
	if ( !found ) {
		disconnectedUser_t & du = disconnectedUsers.Alloc();
		du.lobbyUserID = user.lobbyUserID;
		memcpy( du.gamertag, user.gamertag, sizeof( user.gamertag ) );
	}
}

/*
========================
hfLobby::AllocUser
========================
*/
lobbyUser_t * hfLobby::AllocUser( const lobbyUser_t & defaults ) {
	if ( !verify( freeUsers.Num() > 0 ) ) {
		hfLib::Error( "Out of session users" );		// This shouldn't be possible
	}

	lobbyUser_t * user = freeUsers[freeUsers.Num() - 1];
	freeUsers.SetNum( freeUsers.Num() - 1 );

	// Set defaults
	*user = defaults;

	userList.Append( user );

	assert( userList.Num() == userPool.Max() - freeUsers.Num() );

	return user;
}

/*
========================
hfLobby::FreeUser
========================
*/
void hfLobby::FreeUser( lobbyUser_t * user ) {
	if ( !verify( user != NULL ) ) {
		return;
	}

	if ( !VerifyUser( user ) ) {
		return;
	}

	SaveDisconnectedUser( *user );

	verify( userList.Remove( user ) );

	freeUsers.Append( user );
}

/*
========================
hfLobby::FreeAllUsers
========================
*/
void hfLobby::FreeAllUsers() {
	for ( int i = userList.Num() - 1; i >= 0; i-- ) {
		FreeUser( userList[i] );
	}

	assert( userList.Num() == 0 );
	assert( freeUsers.Num() == userPool.Max() );
}

/*
========================
hfLobby::RegisterUser
========================
*/
void hfLobby::RegisterUser( lobbyUser_t * lobbyUser ) {
	if ( lobbyUser->isBot ) {
		return;
	}

	// Register the user with the various managers
	bool isLocal = IsSessionUserLocal( lobbyUser );

	if ( lobbyBackend != NULL ) {
		lobbyBackend->RegisterUser( lobbyUser, isLocal );
	}

	sessionCB->GetVoiceChat()->RegisterTalker( lobbyUser, lobbyType, isLocal );
}

/*
========================
hfLobby::UnregisterUser
========================
*/
void hfLobby::UnregisterUser( lobbyUser_t * lobbyUser ) {
	if ( lobbyUser->isBot ) {
		return;
	}

	if ( lobbyUser->IsDisconnected() ) {
		return;
	}

	bool isLocal = IsSessionUserLocal( lobbyUser );

	if ( lobbyBackend != NULL ) {
		lobbyBackend->UnregisterUser( lobbyUser, isLocal );
	}

	sessionCB->GetVoiceChat()->UnregisterTalker( lobbyUser, lobbyType, isLocal );
}

/*
========================
hfLobby::VerifyUser
========================
*/
bool hfLobby::VerifyUser( const lobbyUser_t * lobbyUser ) const {
	if ( !verify( userList.FindIndex( const_cast<lobbyUser_t *>( lobbyUser ) ) != -1 ) ) {
		return false;
	}

	return true;
}

/*
========================
hfLobby::IsSessionUserLocal
========================
*/
bool hfLobby::IsSessionUserLocal( const lobbyUser_t * lobbyUser ) const {
	if ( !verify( lobbyUser != NULL ) ) {
		return false;
	}

	if ( !VerifyUser( lobbyUser ) ) {
		return false;
	}

	// This user is local if the peerIndex matches what our peerIndex is on the host
	return ( lobbyUser->peerIndex == peerIndexOnHost );
}

/*
========================
hfLobby::IsSessionUserIndexLocal
========================
*/
bool hfLobby::IsSessionUserIndexLocal( int i ) const {
	return IsSessionUserLocal( GetLobbyUser( i ) );
}

/*
========================
hfLobby::GetLobbyUserIndexByID
========================
*/
int hfLobby::GetLobbyUserIndexByID( lobbyUserID_t lobbyUserId, bool ignoreLobbyType ) const {
	if ( !lobbyUserId.IsValid() ) {
		return -1;
	}
	assert( lobbyUserId.GetLobbyType() == lobbyType || ignoreLobbyType );

	for ( int i = 0; i < GetNumLobbyUsers(); ++ i ) {
		if ( ignoreLobbyType ) {
			if ( GetLobbyUser( i )->lobbyUserID.CompareIgnoreLobbyType( lobbyUserId ) ) {
				return i;
			}
			continue;
		}
		if ( GetLobbyUser( i )->lobbyUserID == lobbyUserId ) {
			return i;
		}
	}
	return -1;
}

/*
========================
hfLobby::GetLobbyUserByID
========================
*/
lobbyUser_t	* hfLobby::GetLobbyUserByID( lobbyUserID_t lobbyUserID, bool ignoreLobbyType ) {
	int index = GetLobbyUserIndexByID( lobbyUserID, ignoreLobbyType );

	if ( index == -1 ) {
		return NULL;
	}

	return GetLobbyUser( index );
}


/*
========================
hfLobby::CreateLobbyUserFromLocalUser
This functions just defaults the session users to the signin manager local users
========================
*/
lobbyUser_t hfLobby::CreateLobbyUserFromLocalUser( const hfLocalUser * localUser ) {
		
	lobbyUser_t lobbyUser;
	hfStr::Copynz( lobbyUser.gamertag, localUser->GetGamerTag(), sizeof( lobbyUser.gamertag ) );
	lobbyUser.peerIndex			= -1;
	lobbyUser.lobbyUserID		= lobbyUserID_t( localUser->GetLocalUserHandle(), lobbyType );	// Generate the lobby using a combination of local user id, and lobby type
	lobbyUser.disconnecting		= false;

	// If we are in a game lobby (or dedicated game state lobby), and we have a party lobby running, assume we can grab the party token from our equivalent user in the party.
	if ( ( lobbyType == TYPE_GAME || lobbyType == TYPE_GAME_STATE ) && sessionCB->GetPartyLobby().IsLobbyActive() ) {
		if ( ( sessionCB->GetPartyLobby().parms.matchFlags & MATCH_REQUIRE_PARTY_LOBBY ) && !( sessionCB->GetPartyLobby().parms.matchFlags & MATCH_PARTY_INVITE_PLACEHOLDER ) ) {
			// copy some things from my party user
			const int myPartyUserIndex = sessionCB->GetPartyLobby().GetLobbyUserIndexByLocalUserHandle( lobbyUser.lobbyUserID.GetLocalUserHandle() );

			if ( verify( myPartyUserIndex >= 0 ) ) {		// Just in case
				lobbyUser_t * myPartyUser = sessionCB->GetPartyLobby().GetLobbyUser( myPartyUserIndex );
				if ( myPartyUser != NULL ) {
					lobbyUser.partyToken = myPartyUser->partyToken;
				}
			}
		}
	}

	lobbyUser.UpdateClientMutableData( localUser );

	NET_VERBOSE_PRINT( "NET: CreateLobbyUserFromLocalUser: party %08x name %s (%s)\n", lobbyUser.partyToken, lobbyUser.gamertag, GetLobbyName() );

	return lobbyUser;
}

/*
========================
hfLobby::InitSessionUsersFromLocalUsers
This functions just defaults the session users to the signin manager local users
========================
*/
void hfLobby::InitSessionUsersFromLocalUsers( bool onlineMatch ) {
	assert( lobbyBackend != NULL );

	// First, clear all session users of this session type
	FreeAllUsers();

	// Copy all local users from sign in mgr to the session user list
	for ( int i = 0; i < sessionCB->GetSignInManager().GetNumLocalUsers(); i++ ) {		
		hfLocalUser * localUser = sessionCB->GetSignInManager().GetLocalUserByIndex( i );
		
		// Make sure this user can join lobbies
		if ( onlineMatch && !localUser->CanPlayOnline() ) {
			continue;
		}

		lobbyUser_t lobbyUser = CreateLobbyUserFromLocalUser( localUser );

		// Append this new session user to the session user list
		lobbyUser_t * createdUser = AllocUser( lobbyUser );

		// Get the migration game data if this is a migrated hosting
		if ( verify( createdUser != NULL ) && migrationInfo.persistUntilGameEndsData.wasMigratedHost ) {
			createdUser->migrationGameData = migrationInfo.persistUntilGameEndsData.ourGameData;
			NET_VERBOSE_PRINT( "NET: Migration game data set for local user %s at index %d \n", createdUser->gamertag, migrationInfo.persistUntilGameEndsData.ourGameData );
		}		
	}
}

/*
========================
hfLobby::GetLobbyUserIndexByLocalUserHandle
Takes a local user handle, and converts to a session user
========================
*/
int hfLobby::GetLobbyUserIndexByLocalUserHandle( const localUserHandle_t localUserHandle ) const {
	// Find the session user that uses this input device index
	for ( int i = 0; i < GetNumLobbyUsers(); i++ ) {
		if ( !IsSessionUserIndexLocal( i ) ) {
			continue;	// We only want local users
		}
		if ( GetLobbyUser( i )->lobbyUserID.GetLocalUserHandle() == localUserHandle ) {
			return i;		// Found it
		}
	}
	
	return -1;
}

/*
========================
hfLobby::GetLocalUserFromLobbyUserIndex
This takes a session user, and converts to a local user
========================
*/
hfLocalUser * hfLobby::GetLocalUserFromLobbyUserIndex( int lobbyUserIndex ) {
	if ( lobbyUserIndex < 0 || lobbyUserIndex >= GetNumLobbyUsers() ) {
		return NULL;
	}

	if ( !IsSessionUserIndexLocal( lobbyUserIndex ) ) {
		return NULL;
	}

	lobbyUser_t * lobbyUser = GetLobbyUser( lobbyUserIndex );

	if ( lobbyUser == NULL ) {
		return NULL;
	}
	
	return sessionCB->GetSignInManager().GetLocalUserByHandle( lobbyUser->lobbyUserID.GetLocalUserHandle() );
}

/*
========================
hfLobby::GetSessionUserFromLocalUser
Takes a local user, and converts to a session user
========================
*/
lobbyUser_t * hfLobby::GetSessionUserFromLocalUser( const hfLocalUser * localUser ) {
	if ( localUser == NULL ) {
		return NULL;
	}

	int sessionUserIndex = GetLobbyUserIndexByLocalUserHandle( localUser->GetLocalUserHandle() );
	
	if ( sessionUserIndex != -1 ) {
		assert( IsSessionUserIndexLocal( sessionUserIndex ) );
		return GetLobbyUser( sessionUserIndex );
	}
	
	return NULL;
}

/*
========================
hfLobby::RemoveUsersWithDisconnectedPeers
Go through each user, and remove the ones that have a peer marked as disconnected
NOTE - This should only be called from the host.  The host will call RemoveSessionUsersByIDList, 
which will forward the action to the connected peers.
========================
*/
void hfLobby::RemoveUsersWithDisconnectedPeers() {
	if ( !verify( IsHost() ) ) {	
		// We're not allowed to do this unless we are the host of this session type
		// If we are the host, RemoveSessionUsersByIDList will forward the call to peers.
		return;
	}

	hfList< lobbyUserID_t > removeList;
	for ( int u = 0; u < GetNumLobbyUsers(); u++ ) {
		lobbyUser_t * user = GetLobbyUser( u );
		if ( !verify( user != NULL ) ) {
			continue;
		}
		if ( IsSessionUserIndexLocal( u ) || user->IsDisconnected() ) {
			continue;
		}

		if ( user->peerIndex == -1 ) {
			// Wanting to know if this actually happens.  
			// If this is a user on the hosts machine, IsSessionUserIndexLocal should catch it above.
			assert( false );		
			// The user is on the host.
			// The host's peer is disconnected via other mechanisms that I don't have a firm
			// grasp on yet.
			continue;
		}

		if ( user->peerIndex >= peers.Num() ) {
			// TTimo - I am hitting this in ~12 client games for some reason?
			// only throwing an assertion in debug, with no crashing, so adding a warning verbose
			hfLib::Warning( "hfLobby::RemoveUsersWithDisconnectedPeers: user %d %s is out of range in the peers list (%d elements)", u, user->gamertag, peers.Num() );
			continue;
		}
		peer_t & peer = peers[ user->peerIndex ];
		if ( peer.GetConnectionState() != CONNECTION_ESTABLISHED ) {
			removeList.Append( user->lobbyUserID );
		}
	}

	RemoveSessionUsersByIDList( removeList );
}

/*
========================
hfLobby::RemoveSessionUsersByIDList
This is the choke point for removing users from a session.
It will handle all the housekeeping of removing from various platform lists (xsession user tracking, etc).
Called from both host and client.
========================
*/
void hfLobby::RemoveSessionUsersByIDList( hfList< lobbyUserID_t > & usersToRemoveByID ) {
	assert( lobbyBackend != NULL || usersToRemoveByID.Num() == 0 );

	for ( int i = 0; i < usersToRemoveByID.Num(); i++ ) {
		for ( int u = 0; u < GetNumLobbyUsers(); u++ ) {
			lobbyUser_t * user = GetLobbyUser( u );
			
			if ( user->IsDisconnected() ) {
				// User already disconnected from session  but not removed from the list.
				// This will happen when users are removed during the game.
				continue;
			}
			
			if ( user->lobbyUserID == usersToRemoveByID[i] ) {
				if ( lobbyType == TYPE_GAME ) {
					hfLib::Printf( "NET: %s left the game.\n", user->gamertag );
				} else if ( lobbyType == TYPE_PARTY ) {
					hfLib::Printf( "NET: %s left the party.\n", user->gamertag );
				}

				UnregisterUser( user );

				// Save the user so we can still get his gamertag, which may be needed for
				// a disconnection HUD message.
				SaveDisconnectedUser( *user );
				FreeUser( user );
				
				break;
			}
		}
	}

	if ( usersToRemoveByID.Num() > 0 && IsHost() ) {
		if ( lobbyBackend != NULL ) {
			lobbyBackend->UpdateLobbySkill( GetAverageSessionLevel() );
		}

		// If we are the host, send a message to all peers with a list of users who have disconnected
		byte buffer[ hfPacketProcessor::MAX_MSG_SIZE ];
		hfBitMsg msg( buffer, sizeof( buffer ) );
		msg.WriteByte( usersToRemoveByID.Num() );

		for ( int i = 0; i < usersToRemoveByID.Num(); i++ ) {
			usersToRemoveByID[i].WriteToMsg( msg );
		}
		for ( int p = 0; p < peers.Num(); p++ ) {
			QueueReliableMessage( p, RELIABLE_USER_DISCONNECTED, msg.GetReadData(), msg.GetSize() );
		}
	}
}

/*
========================
hfLobby::SendPeersMicStatusToNewUsers
Sends each current user mic status to the newly added peer.
========================
*/
void hfLobby::SendPeersMicStatusToNewUsers( int peerNumber ) {
	if ( !IsHost() ) {
		return;
	}

	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE ];
	hfBitMsg outmsg( buffer, sizeof( buffer ) );

	// Count up how many users will be in the msg
	int numUsersInMsg = 0;

	for ( int i = 0; i < GetNumLobbyUsers(); ++i ) {
		lobbyUser_t * user = GetLobbyUser( i );

		if ( user->isBot ) {
			continue;
		}

		if ( user->peerIndex == peerNumber ) {
			continue;
		}

		numUsersInMsg++;
	}

	if ( numUsersInMsg == 0 ) {
		return;		// Nothing to do
	}

	outmsg.WriteLong( numUsersInMsg );

	for ( int i = 0; i < GetNumLobbyUsers(); ++i ) {
		lobbyUser_t * user = GetLobbyUser( i );

		if ( user->isBot ) {
			continue;
		}

		if ( user->peerIndex == peerNumber ) {
			continue;
		}
		
		int talkerIndex = sessionCB->GetVoiceChat()->FindTalkerByUserId( user->lobbyUserID, lobbyType );
		bool state = sessionCB->GetVoiceChat()->GetHeadsetState( talkerIndex );
		
		hfLib::Printf( "Packing headset state %d for user %d %s\n", state, i, user->gamertag );
		user->lobbyUserID.WriteToMsg( outmsg );
		outmsg.WriteBool( state );
	}


	hfLib::Printf( "Sending headset states to new peer %d\n", peerNumber );
	QueueReliableMessage( peerNumber, RELIABLE_HEADSET_STATE, outmsg.GetReadData(), outmsg.GetSize() );
}

/*
========================
hfLobby::SendNewUsersToPeers
Sends a range of users to the current list of peers.
The host calls this when he receives new users, to forward the list to the other peers.
========================
*/
void hfLobby::SendNewUsersToPeers( int skipPeer, int userStart, int numUsers ) {
	if ( !IsHost() ) {
		return;
	}

	assert( GetNumLobbyUsers() - userStart == numUsers );
	
	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE ];
	hfBitMsg outmsg( buffer, sizeof( buffer ) );

	// Write number of users
	outmsg.WriteByte( numUsers );
	
	// Fill up the msg with all the users
	for ( int u = userStart; u < GetNumLobbyUsers(); u++ ) {
		GetLobbyUser( u )->WriteToMsg( outmsg );
	}
	
	// Send the msg to all peers (except the skipPeer, or peers not connected to this session type)
	for ( int p = 0; p < peers.Num(); p++ ) {
		if ( p == skipPeer || peers[p].GetConnectionState() != CONNECTION_ESTABLISHED ) {
			continue;	// If they are not connected in this session type, don't send anything to them.
		}
		QueueReliableMessage( p, RELIABLE_USER_CONNECTED, outmsg.GetReadData(), outmsg.GetSize() );
	}
}

/*
========================
hfLobby::AllocLobbyUserSlotForBot
========================
*/
lobbyUserID_t hfLobby::AllocLobbyUserSlotForBot( const char * botName ) {
	lobbyUser_t botSessionUser;
	botSessionUser.peerIndex = peerIndexOnHost;
	botSessionUser.isBot = true;
	botSessionUser.disconnecting = false;
	hfStr::Copynz( botSessionUser.gamertag, botName, sizeof( botSessionUser.gamertag ) );

	localUserHandle_t localUserHandle( session->GetSignInManager().GetUniqueLocalUserHandle( botSessionUser.gamertag ) );
	botSessionUser.lobbyUserID = lobbyUserID_t( localUserHandle, lobbyType );

	lobbyUser_t * botUser = NULL;

	int sessionUserIndex = -1;

	// First, try to replace a disconnected user
	for ( int i = 0; i < GetNumLobbyUsers(); ++i ) {
		if ( IsLobbyUserDisconnected( i ) ) {
			lobbyUser_t * user = GetLobbyUser( i );
			if ( verify( user != NULL ) ) {
				*user = botSessionUser;
				botUser = user;
				sessionUserIndex = i;
				break;
			}
		}
	}

	if ( botUser == NULL ) {
		if ( freeUsers.Num() == 0 ) {
			hfLib::Warning( "NET: Out Of Session Users - Can't Add Bot %s!", botName );
			return lobbyUserID_t();
		}
		botUser = AllocUser( botSessionUser );
		sessionUserIndex = userList.Num() - 1;
	}

	if ( !verify( botUser != NULL ) ) {
		hfLib::Warning( "NET: Can't Find Session Slot For Bot!" );
		return lobbyUserID_t();
	} else {
		NET_VERBOSE_PRINT( "NET: Created Bot %s At Index %d \n", botUser->gamertag, sessionUserIndex );
	}

	SendNewUsersToPeers( peerIndexOnHost, userList.Num() - 1, 1 ); // bot has been added to the lobby user list - update the peers so that they can see the bot too.

	return GetLobbyUser( sessionUserIndex )->lobbyUserID;
}

/*
========================
hfLobby::RemoveBotFromLobbyUserList
========================
*/
void hfLobby::RemoveBotFromLobbyUserList( lobbyUserID_t lobbyUserID ) {
	const int index = GetLobbyUserIndexByID( lobbyUserID );

	lobbyUser_t * botUser = GetLobbyUser( index );
	if ( botUser == NULL ) {
		assert( false );
		hfLib::Warning( "RemoveBotFromLobbyUserList: Invalid User Index!" );
		return;
	}

	if ( !botUser->isBot ) {
		hfLib::Warning( "RemoveBotFromLobbyUserList: User Index Is Not A Bot!" ); // don't accidentally disconnect a human player.
		return;
	}

	botUser->isBot = false;
	botUser->lobbyUserID = lobbyUserID_t();

	FreeUser( botUser );
}

/*
========================
hfLobby::GetLobbyUserIsBot
========================
*/
bool hfLobby::GetLobbyUserIsBot( lobbyUserID_t lobbyUserID ) const {
	const int index = GetLobbyUserIndexByID( lobbyUserID );

	const lobbyUser_t * botLobbyUser = GetLobbyUser( index );
	if ( botLobbyUser == NULL ) {
		return false;
	}

	return botLobbyUser->isBot;
}

/*
========================
hfLobby::AddUsersFromMsg
Called on peer and host.
Simply parses a msg, and adds any new users from it to our own user list.
If we are the host, we will forward this to all peers except the peer that we just received it from.
========================
*/
void hfLobby::AddUsersFromMsg( hfBitMsg & msg, int fromPeer ) {
	int userStart	= GetNumLobbyUsers();
	int numNewUsers = msg.ReadByte();
	
	assert( lobbyBackend != NULL );

	// Add the new users to our own list
	for ( int u = 0; u < numNewUsers; u++ ) {
		lobbyUser_t newUser;
		
		// Read in the new user
		newUser.ReadFromMsg( msg );

		// Initialize their peerIndex and userID if we are the host
		// (we'll send these back to them in the initial connect)
		if ( IsHost() ) {
			if ( fromPeer != -1 ) {		// -1 means this is the host adding his own users, and this stuff is already computed
				// local users will already have this information filled out.
				newUser.address		= peers[ fromPeer ].address; 
				newUser.peerIndex	= fromPeer;
				if ( lobbyType == TYPE_PARTY ) {
					newUser.partyToken = GetPartyTokenAsHost();
				}
			}
		} else {
			assert( fromPeer == host );
			// The host sends us all user addresses, except his local users, so we compute that here
			if ( newUser.peerIndex == -1 ) {
				newUser.address	= peers[ fromPeer ].address;		
			}
		}

		hfLib::Printf( "NET: %s joined (%s) [partyToken = %08x].\n", newUser.gamertag, GetLobbyName(), newUser.partyToken );

		lobbyUser_t * appendedUser = NULL;

		// First, try to replace a disconnected user
		for ( int i = 0; i < GetNumLobbyUsers(); i++ ) {
			lobbyUser_t * user = GetLobbyUser( i );

			if ( user->IsDisconnected() ) {
				userStart = i;
				*user = newUser;
				appendedUser = user;
				break;
			}
		}

		// Add them to our list
		if ( appendedUser == NULL ) {
			appendedUser = AllocUser( newUser );
		}

		// Run platform-specific handler after adding
		assert( appendedUser->peerIndex == newUser.peerIndex );		// paranoia
		assert( appendedUser->lobbyUserID == newUser.lobbyUserID );	// paranoia
		RegisterUser( appendedUser );
	}
	
	// Forward list of the new users to all other peers
	if ( IsHost() ) {
		SendNewUsersToPeers( fromPeer, userStart, numNewUsers );
		
		// Set the lobbies skill level
		lobbyBackend->UpdateLobbySkill( GetAverageSessionLevel() );
	}

	hfLib::Printf( "---------------- %s --------------------\n", GetLobbyName() );
	for( int userIndex = 0; userIndex < GetNumLobbyUsers(); ++userIndex ) {
		lobbyUser_t * user = GetLobbyUser( userIndex );
		hfLib::Printf( "party %08x user %s\n", user->partyToken, user->gamertag );
	}
	hfLib::Printf( "---------------- %s --------------------\n", GetLobbyName() );
}

/*
========================
hfLobby::UpdateSessionUserOnPeers
========================
*/
void hfLobby::UpdateSessionUserOnPeers( hfBitMsg & msg ) {
	for ( int p = 0; p < peers.Num(); p++ ) {
		QueueReliableMessage( p, RELIABLE_UPDATE_SESSION_USER, msg.GetReadData() + msg.GetReadCount(), msg.GetSize() - msg.GetReadCount() );
	}

	HandleUpdateSessionUser( msg );
}

/*
========================
hfLobby::HandleHeadsetStateChange
========================
*/
void hfLobby::HandleHeadsetStateChange( int fromPeer, hfBitMsg & msg ) {
	int userCount = msg.ReadLong();
	
	for ( int i = 0; i < userCount; ++i ) {
		lobbyUserID_t lobbyUserID;
		lobbyUserID.ReadFromMsg( msg );
		bool state = msg.ReadBool();
		
		int talkerIndex = sessionCB->GetVoiceChat()->FindTalkerByUserId( lobbyUserID, lobbyType );
		sessionCB->GetVoiceChat()->SetHeadsetState( talkerIndex, state );

		hfLib::Printf( "User %d headset status: %d\n", talkerIndex, state );

		// If we are the host, let the other clients know about the headset state of this peer
		if ( IsHost() ) {

			// We should not be receiving a message with a user count > 1 if we are the host
			assert( userCount == 1 );

			byte buffer[ hfPacketProcessor::MAX_MSG_SIZE ];
			hfBitMsg outMsg( buffer, sizeof( buffer ) );
			outMsg.WriteLong( 1 );
			lobbyUserID.WriteToMsg( outMsg );
			outMsg.WriteBool( state );

			for ( int j = 0; j < peers.Num(); ++j ) {
				// Don't send this to the player that we just received the message from
				if ( !peers[ j ].IsConnected() || j == fromPeer ) {
					continue;
				}

				QueueReliableMessage( j, RELIABLE_HEADSET_STATE, outMsg.GetReadData(), outMsg.GetSize() );
			}
		}
	}
}

/*
========================
hfLobby::HandleUpdateSessionUser
========================
*/
void hfLobby::HandleUpdateSessionUser( hfBitMsg & msg ) {
	// FIXME: Use a user id here
	int sessionUserIndex = msg.ReadByte();

	lobbyUser_t * user = GetLobbyUser( sessionUserIndex );

	if ( verify( user != NULL ) ) {
		user->ReadClientMutableData( msg );
	}
}

/*
========================
hfLobby::CreateUserUpdateMessage
========================
*/
void hfLobby::CreateUserUpdateMessage( int userIndex, hfBitMsg & msg ) {
	lobbyUser_t * user = GetLobbyUser( userIndex );

	if ( verify( user != NULL ) ) {
		msg.WriteByte( userIndex );
		user->WriteClientMutableData( msg );
	}
}

/*
========================
hfLobby::UpdateLocalSessionUsers
======================== 
*/
void hfLobby::UpdateLocalSessionUsers() {
	for ( int i = 0; i < GetNumLobbyUsers(); i++ ) {
		hfLocalUser * localUser = GetLocalUserFromLobbyUserIndex( i );
		lobbyUser_t * lobbyUser = GetLobbyUser( i );

		if ( localUser == NULL || lobbyUser == NULL ) {
			continue;
		}
		if ( !lobbyUser->UpdateClientMutableData( localUser ) ) {
			continue;
		}

		byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE - 2 ];
		hfBitMsg msg( buffer, sizeof( buffer ) );

		CreateUserUpdateMessage( i, msg );

		if ( IsHost() ) {
			UpdateSessionUserOnPeers( msg );
		} else if ( IsPeer() ) {
			QueueReliableMessage( host, RELIABLE_SESSION_USER_MODIFIED, msg.GetReadData(), msg.GetSize() );
		}
	}
}

/*
========================
hfLobby::PeerIndexForSessionUserIndex
========================
*/
int hfLobby::PeerIndexForSessionUserIndex( int sessionUserIndex ) const {
	const lobbyUser_t * user = GetLobbyUser( sessionUserIndex );

	if ( !verify( user != NULL ) ) {
		return -1;
	}

	return user->peerIndex;
}

/*
========================
hfLobby::HandleUserConnectFailure
========================
*/
void hfLobby::HandleUserConnectFailure( int p, hfBitMsg & inMsg, int reliableType ) {
	// Read user to get handle so we can send it back
	inMsg.ReadByte();		// Num users
	lobbyUser_t user;
	user.ReadFromMsg( inMsg );
	
	// Not enough room, send failure ack
	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE ];
	hfBitMsg msg( buffer, sizeof( buffer ) );
	user.lobbyUserID.GetLocalUserHandle().WriteToMsg( msg );		// Let peer know which user failed to connect
	
	// Send it
	QueueReliableMessage( p, reliableType, msg.GetReadData(), msg.GetSize() );
}

/*
========================
hfLobby::ProcessUserDisconnectMsg
========================
*/
void hfLobby::ProcessUserDisconnectMsg( hfBitMsg & msg ) {

	hfList< lobbyUserID_t > removeList;

	// Convert the msg into a list of id's
	const int numUsers			= msg.ReadByte();
	
	for ( int u = 0; u < numUsers; u++ ) {
		lobbyUserID_t lobbyUserID;
		lobbyUserID.ReadFromMsg( msg );
		removeList.Append( lobbyUserID );
	}
	
	RemoveSessionUsersByIDList( removeList );
}

/*
========================
hfLobby::CompactDisconnectedUsers
Pack the user list by removing disconnected users
We need to do this, since when in a game, we aren't allowed to remove users from the game session.
========================
*/
void hfLobby::CompactDisconnectedUsers() {
	for ( int i = GetNumLobbyUsers() - 1; i >= 0; i-- ) {
		lobbyUser_t * user = GetLobbyUser( i );
		if ( user->IsDisconnected() ) {
			FreeUser( user );
		}
	}
}

/*
========================
hfLobby::RequestLocalUserJoin
Sends a request to the host to join a local user to a session.
If we are the host, we will do it immediately.
========================
*/
void hfLobby::RequestLocalUserJoin( hfLocalUser * localUser ) {
	assert( IsRunningAsHostOrPeer() );

	// Construct a msg that contains the user connect request
	lobbyUser_t lobbyUser = CreateLobbyUserFromLocalUser( localUser );
	
	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE ];
	hfBitMsg msg( buffer, sizeof( buffer ) );
	
	msg.WriteByte( 1 );					// 1 user
	lobbyUser.WriteToMsg( msg );		// Write user

	if ( IsHost() ) {
		AddUsersFromMsg( msg, -1 );
		localUser->SetJoiningLobby( lobbyType, false );
	} else {
		// Send request to host to add user
		QueueReliableMessage( host, RELIABLE_USER_CONNECT_REQUEST, msg.GetReadData(), msg.GetSize() );
	}
}

/*
========================
hfLobby::RequestSessionUserDisconnect
Sends a request to the host to remove a session user from the session.
If we are the host, we will do it immediately.
========================
*/
void hfLobby::RequestSessionUserDisconnect( int sessionUserIndex ) {

	if ( !IsRunningAsHostOrPeer() ) {
		// If we are not in an actual running session, just remove it.
		// This is so we accurately reflect the local user list through the session users in the menus, etc.
		// FIXME:
		// This is a total hack, and we should really look at better separation of local users/session users
		// and not rely on session users while in the menus. 
		FreeUser( GetLobbyUser( sessionUserIndex ) );
		return;
	}
	
	lobbyUser_t * lobbyUser = GetLobbyUser( sessionUserIndex );

	if ( !verify( lobbyUser != NULL ) ) {
		return;
	}
	
	if ( lobbyUser->disconnecting == true ) {
		return;		// Already disconnecting
	}

	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE ];
	hfBitMsg msg( buffer, sizeof( buffer ) );
	
	msg.WriteByte( 1 );	// 1 user
	lobbyUser->lobbyUserID.WriteToMsg( msg );
	
	if ( IsHost() ) {
		hfBitMsg readMsg;
		readMsg.InitRead( msg.GetReadData(), msg.GetSize() );

		// As the host, just disconnect immediately (we'll still send the notification to all peers though)
		ProcessUserDisconnectMsg( readMsg );
	} else {
		// Send the message
		QueueReliableMessage( host, RELIABLE_USER_DISCONNECT_REQUEST, msg.GetReadData(), msg.GetSize() );
		
		// Mark user as disconnecting to make sure we don't keep sending the request
		lobbyUser->disconnecting = true;
	}
}

/*
========================
hfLobby::SyncLobbyUsersWithLocalUsers
This function will simply try and keep session[lobbyType].users sync'd up with local users.
As local users come and go, this function will detect that, and send msg's to the host that will
add/remove the users from the session.
========================
*/
void hfLobby::SyncLobbyUsersWithLocalUsers( bool allowLocalJoins, bool onlineMatch ) {		
	
	if ( lobbyBackend == NULL ) {
		return;
	}

	if ( !IsRunningAsHostOrPeer() ) {
		return;
	}

	if ( allowLocalJoins ) {
		// If we are allowed to do so, allow local users to join the session user list
		for ( int i = 0; i < sessionCB->GetSignInManager().GetNumLocalUsers(); i++ ) {
			hfLocalUser * localUser = sessionCB->GetSignInManager().GetLocalUserByIndex( i );
						
			if ( GetSessionUserFromLocalUser( localUser ) != NULL ) {
				continue;		// Already in the lobby
			}

			if ( localUser->IsJoiningLobby( lobbyType ) ) {
				continue;		// Already joining lobby if this type
			}
			
			if ( onlineMatch && !localUser->CanPlayOnline() ) {
				continue;		// Not allowed to join this type of lobby
			}
			
			localUser->SetJoiningLobby( lobbyType, true );		// We'll reset this when we get the ack for this request
			RequestLocalUserJoin( localUser );
		}	
	}

	// Find session users that are no longer allowed to be in the list
	for ( int i = GetNumLobbyUsers() - 1; i >= 0; i-- ) {
		if ( !IsSessionUserIndexLocal( i ) ) {
			continue;
		}

		lobbyUser_t * lobbyUser = GetLobbyUser( i );
		if ( lobbyUser != NULL && lobbyUser->isBot ) {
			continue;
		}

		hfLocalUser * localUser = GetLocalUserFromLobbyUserIndex( i );
		
		if ( localUser == NULL || ( onlineMatch && !localUser->CanPlayOnline() ) ) {
			// Either the session user is no longer in the local user list,
			//	or not allowed to join online lobbies.
			RequestSessionUserDisconnect( i );
		} else {
			localUser->SetJoiningLobby( lobbyType, false );		// Remove joining lobby flag if we are in the lobby
		}
	}
}

/*
========================
hfLobby::IsLobbyUserDisconnected
========================
*/
bool hfLobby::IsLobbyUserDisconnected( int userIndex ) const {
	const lobbyUser_t * user = GetLobbyUser( userIndex );
	if ( user == NULL ) {
		return true;
	}

	if ( user->isBot ) {
		return false;
	}

	if ( user->IsDisconnected() ) {
		return true;
	}

	return false;
}

/*
========================
hfLobby::IsLobbyUserValid
========================
*/
bool hfLobby::IsLobbyUserValid( lobbyUserID_t lobbyUserID ) const {
	if ( !lobbyUserID.IsValid() ) {
		return false;
	}

	if ( GetLobbyUserIndexByID( lobbyUserID ) == -1 ) {
		return false;
	}

	return true;
}

/*
========================
hfLobby::ValidateConnectedUser
========================
*/
bool hfLobby::ValidateConnectedUser( const lobbyUser_t * user ) const {
	if ( user == NULL ) {
		return false;
	}

	if ( user->IsDisconnected() ) {
		return false;
	}

	if ( user->peerIndex == -1 ) {
		return true;		// Host
	}

	if ( IsHost() ) {
		if ( user->peerIndex < 0 || user->peerIndex >= peers.Num() ) {
			return false;
		}

		if ( !peers[user->peerIndex].IsConnected() ) {
			return false;
		}
	}
	
	return true;
}

/*
========================
hfLobby::IsLobbyUserLoaded
========================
*/
bool hfLobby::IsLobbyUserLoaded( lobbyUserID_t lobbyUserID ) const {
	assert( lobbyType == GetActingGameStateLobbyType() );

	int userIndex = GetLobbyUserIndexByID( lobbyUserID );

	if ( !verify( userIndex != -1 ) ) {
		return false;
	}

	const lobbyUser_t * user = GetLobbyUser( userIndex );

	if ( user == NULL ) {
		return false;
	}

	if ( user->isBot ) {
		return true;
	}

	if ( !ValidateConnectedUser( user ) ) {
		return false;
	}

	if ( IsSessionUserLocal( user ) ) {
		return loaded;		// If this is a local user, check the local loaded flag
	}

	if ( !verify( user->peerIndex >= 0 && user->peerIndex < peers.Num() ) ) {
		return false;
	}

	return peers[user->peerIndex].loaded;
}

/*
========================
hfLobby::LobbyUserHasFirstFullSnap
========================
*/
bool hfLobby::LobbyUserHasFirstFullSnap( lobbyUserID_t lobbyUserID ) const {
	assert( lobbyType == GetActingGameStateLobbyType() );

	int userIndex = GetLobbyUserIndexByID( lobbyUserID );

	const lobbyUser_t * user = GetLobbyUser( userIndex );

	if ( !ValidateConnectedUser( user ) ) {
		return false;
	}

	if ( user->peerIndex == -1 ) {
		return false;
	}

	if ( peers[user->peerIndex].snapProc->GetFullSnapBaseSequence() < hfSnapshotProcessor::INITIAL_SNAP_SEQUENCE ) {
		return false;
	}

	return true;
}

/*
========================
hfLobby::GetLobbyUserIdByOrdinal
========================
*/
lobbyUserID_t hfLobby::GetLobbyUserIdByOrdinal( int userIndex ) const {
	const lobbyUser_t * user = GetLobbyUser( userIndex );
	if ( user == NULL ) {
		return lobbyUserID_t();
	}

	if ( user->isBot ) {
		return user->lobbyUserID;
	}

	if ( !ValidateConnectedUser( user ) ) {
		return lobbyUserID_t();
	}

	return user->lobbyUserID;
}

/*
========================
hfLobby::GetLobbyUserIndexFromLobbyUserID
========================
*/
int hfLobby::GetLobbyUserIndexFromLobbyUserID( lobbyUserID_t lobbyUserID ) const {
	return GetLobbyUserIndexByID( lobbyUserID );
}

/*
========================
hfLobby::EnableSnapshotsForLobbyUser
========================
*/
void hfLobby::EnableSnapshotsForLobbyUser( lobbyUserID_t lobbyUserID ) {
	assert( lobbyType == GetActingGameStateLobbyType() );

	int userIndex = GetLobbyUserIndexByID( lobbyUserID );

	const lobbyUser_t * user = GetLobbyUser( userIndex );

	if ( !ValidateConnectedUser( user ) ) {
		return;
	}

	if ( user->peerIndex == -1 ) {
		return;
	}

	peers[user->peerIndex].pauseSnapshots = false;
}

/*
========================
hfLobby::GetAverageSessionLevel
========================
*/
float hfLobby::GetAverageSessionLevel() {
	float	level				= 0.0f;
	int		numActiveMembers	= 0;

	for ( int i = 0; i < GetNumLobbyUsers(); i++ ) {
		const lobbyUser_t * user = GetLobbyUser( i );

		if ( user->IsDisconnected() ) {
			continue;
		}

		level += user->level;
		numActiveMembers++;
	}

	if ( numActiveMembers > 0 ) {
		level /= (float)numActiveMembers;
	}

	float ret = Max( level, 1.0f );
	NET_VERBOSE_PRINT( "NET: GetAverageSessionLevel %g\n", ret );
	return ret;
}

/*
========================
hfLobby::GetAverageLocalUserLevel
========================
*/
float hfLobby::GetAverageLocalUserLevel( bool onlineOnly ) {
	float	level				= 0.0f;
	int		numActiveMembers	= 0;

	for ( int i = 0; i < sessionCB->GetSignInManager().GetNumLocalUsers(); ++i ) {
		const hfLocalUser * localUser = sessionCB->GetSignInManager().GetLocalUserByIndex( i );

		if ( onlineOnly && !localUser->CanPlayOnline() ) {
			continue;
		}

		const hfPlayerProfile * profile = localUser->GetProfile();

		if ( profile == NULL ) {
			continue;
		}

		level += profile->GetLevel();
		numActiveMembers++;
	}

	if ( numActiveMembers > 0 ) {
		level /= (float)numActiveMembers;
	}

	return Max( level, 1.0f );
}

/*
========================
hfLobby::QueueReliablePlayerToPlayerMessage
========================
*/
void hfLobby::QueueReliablePlayerToPlayerMessage( int fromSessionUserIndex, int toSessionUserIndex, reliablePlayerToPlayer_t type, const byte * data, int dataLen ) {
	
	reliablePlayerToPlayerHeader_t info;
	info.fromSessionUserIndex = fromSessionUserIndex;
	info.toSessionUserIndex = toSessionUserIndex;

	// Some kind of pool allocator for packet buffers would be nice.

	byte buffer[ hfPacketProcessor::MAX_PACKET_SIZE ];
	hfBitMsg outmsg( buffer, sizeof( buffer ) );
	if ( !info.Write( this, outmsg ) ) {
		hfLib::Warning( "NET: Can't queue invalid reliable player to player msg" );
		return;
	}
	outmsg.WriteData( data, dataLen );

	const lobbyUser_t * targetUser = GetLobbyUser( toSessionUserIndex );

	if ( !verify( targetUser != NULL ) ) {
		return;
	}

	const int sendToPeer = IsHost() ? targetUser->peerIndex : host;

	QueueReliableMessage( sendToPeer, RELIABLE_PLAYER_TO_PLAYER_BEGIN + (int) type, outmsg.GetReadData(), outmsg.GetSize() );
}

/*
========================
hfLobby::KickLobbyUser
========================
*/
void hfLobby::KickLobbyUser( lobbyUserID_t lobbyUserID ) {
	if ( !IsHost() ) {
		return;
	}

	const int lobbyUserIndex = GetLobbyUserIndexByID( lobbyUserID );

	lobbyUser_t * user = GetLobbyUser( lobbyUserIndex );

	if ( user != NULL && !IsSessionUserLocal( user ) ) {
		// Send an explicit kick msg, so they know why they were removed
		if ( user->peerIndex >= 0 && user->peerIndex < peers.Num() ) {
			byte buffer[ hfPacketProcessor::MAX_MSG_SIZE ];
			hfBitMsg msg( buffer, sizeof( buffer ) );
			msg.WriteByte( lobbyUserIndex );
			QueueReliableMessage( user->peerIndex, RELIABLE_KICK_PLAYER, msg.GetReadData(), msg.GetSize() );
		}
	}
}

/*
========================
hfLobby::GetNumConnectedUsers
========================
*/
int hfLobby::GetNumConnectedUsers() const {
	int numConnectectUsers = 0;

	for ( int i = 0; i < GetNumLobbyUsers(); i++ ) {
		const lobbyUser_t * user = GetLobbyUser( i );

		if ( user->IsDisconnected() ) {
			continue;
		}

		numConnectectUsers++;
	}

	return numConnectectUsers;
}
