#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfCommon_local.h"

hfCVar net_clientMaxPrediction( "net_clientMaxPrediction", "5000", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "maximum number of milliseconds a client can predict ahead of server." );
hfCVar net_snapRate( "net_snapRate", "100", CVAR_SYSTEM | CVAR_INTEGER, "How many milliseconds between sending snapshots" );
hfCVar net_ucmdRate( "net_ucmdRate", "40", CVAR_SYSTEM | CVAR_INTEGER, "How many milliseconds between sending usercmds" );

hfCVar net_debug_snapShotTime( "net_debug_snapShotTime", "0", CVAR_BOOL | CVAR_ARCHIVE, "" );
hfCVar com_forceLatestSnap( "com_forceLatestSnap", "0", CVAR_BOOL, "" );

// Enables effective snap rate: dynamically adjust the client snap rate based on:
//	-client FPS
//	-server FPS (interpolated game time received / interval it was received over)
//  -local buffered time (leave a cushion to absorb spikes, slow down when infront of it, speed up when behind it) ie: net_minBufferedSnapPCT_Static
hfCVar net_effectiveSnapRateEnable( "net_effectiveSnapRateEnable", "1", CVAR_BOOL, "Dynamically adjust client snaprate");
hfCVar net_effectiveSnapRateDebug( "net_effectiveSnapRateDebug", "0", CVAR_BOOL, "Debug");

// Min buffered snapshot time to keep as a percentage of the effective snaprate
//	-ie we want to keep 50% of the amount of time difference between last two snaps.
//	-we need to scale this because we may get throttled at the snaprate may change
//  -Acts as a buffer to absorb spikes
hfCVar net_minBufferedSnapPCT_Static( "net_minBufferedSnapPCT_Static", "1.0", CVAR_FLOAT, "Min amount of snapshot buffer time we want need to buffer");
hfCVar net_maxBufferedSnapMS( "net_maxBufferedSnapMS", "336", CVAR_INTEGER, "Max time to allow for interpolation cushion");
hfCVar net_minBufferedSnapWinPCT_Static( "net_minBufferedSnapWinPCT_Static", "1.0", CVAR_FLOAT, "Min amount of snapshot buffer time we want need to buffer");

// Factor at which we catch speed up interpolation if we fall behind our optimal interpolation window
//  -This is a static factor. We may experiment with a dynamic one that would be faster the farther you are from the ideal window
hfCVar net_interpolationCatchupRate( "net_interpolationCatchupRate", "1.3", CVAR_FLOAT, "Scale interpolationg rate when we fall behind");
hfCVar net_interpolationFallbackRate( "net_interpolationFallbackRate", "0.95", CVAR_FLOAT, "Scale interpolationg rate when we fall behind");
hfCVar net_interpolationBaseRate( "net_interpolationBaseRate", "1.0", CVAR_FLOAT, "Scale interpolationg rate when we fall behind");

// Enabled a dynamic ideal snap buffer window: we will scale the distance and size 
hfCVar net_optimalDynamic( "net_optimalDynamic", "1", CVAR_BOOL, "How fast to add to our optimal time buffer when we are playing snapshots faster than server is feeding them to us");

// These values are used instead if net_optimalDynamic is 0 (don't scale by actual snap rate/interval)
hfCVar net_optimalSnapWindow( "net_optimalSnapWindow", "112", CVAR_FLOAT, "");
hfCVar net_optimalSnapTime( "net_optimalSnapTime", "112", CVAR_FLOAT, "How fast to add to our optimal time buffer when we are playing snapshots faster than server is feeding them to us");

// this is at what percentage of being ahead of the interpolation buffer that we start slowing down (we ramp down from 1.0 to 0.0 starting here)
// this is a percentage of the total cushion time.
hfCVar net_interpolationSlowdownStart( "net_interpolationSlowdownStart", "0.5", CVAR_FLOAT, "Scale interpolation rate when we fall behind");


// Extrapolation is now disabled
hfCVar net_maxExtrapolationInMS( "net_maxExtrapolationInMS", "0", CVAR_INTEGER, "Max time in MS that extrapolation is allowed to occur.");

static const int SNAP_USERCMDS = 8192;

/*
===============
hfCommonLocal::IsMultiplayer
===============
*/
bool hfCommonLocal::IsMultiplayer() {
	hfLobbyBase & lobby = session->GetPartyLobbyBase();
	return ( ( ( lobby.GetMatchParms().matchFlags & MATCH_ONLINE ) != 0 ) && ( session->GetState() > hfSession::IDLE ) );
}

/*
===============
hfCommonLocal::IsServer
===============
*/
bool hfCommonLocal::IsServer() {
	return IsMultiplayer() && session->GetActingGameStateLobbyBase().IsHost();
}

/*
===============
hfCommonLocal::IsClient
===============
*/
bool hfCommonLocal::IsClient() {
	return IsMultiplayer() && session->GetActingGameStateLobbyBase().IsPeer();
}

/*
===============
hfCommonLocal::SendSnapshots
===============
*/
int hfCommonLocal::GetSnapRate() {
	return net_snapRate.GetInteger();
}

/*
===============
hfCommonLocal::SendSnapshots
===============
*/
void hfCommonLocal::SendSnapshots() {
	if ( !mapSpawned ) {
		return;
	}
	int currentTime = Sys_Milliseconds();
	if ( currentTime < nextSnapshotSendTime ) {
		return;
	}
	hfLobbyBase & lobby = session->GetActingGameStateLobbyBase();
	if ( !lobby.IsHost() ) {
		return;
	}
	if ( !lobby.HasActivePeers() ) {
		return;
	}
	hfSnapShot ss;
	game->ServerWriteSnapshot( ss );

	session->SendSnapshot( ss );
	nextSnapshotSendTime = MSEC_ALIGN_TO_FRAME( currentTime + net_snapRate.GetInteger() );
}

/*
===============
hfCommonLocal::NetReceiveSnapshot
===============
*/
void hfCommonLocal::NetReceiveSnapshot( class hfSnapShot & ss ) {
	ss.SetRecvTime( Sys_Milliseconds() );
	// If we are about to overwrite the oldest snap, then force a read, which will cause a pop on screen, but we have to do this.
	if ( writeSnapshotIndex - readSnapshotIndex >= RECEIVE_SNAPSHOT_BUFFER_SIZE ) {
		hfLib::Printf( "Overwritting oldest snapshot %d with new snapshot %d\n", readSnapshotIndex, writeSnapshotIndex );
		assert( writeSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE == readSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE );
		ProcessNextSnapshot();
	}

	receivedSnaps[ writeSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE ] = ss;
	writeSnapshotIndex++;

	// Force read the very first 2 snapshots
	if ( readSnapshotIndex < 2 ) {
		ProcessNextSnapshot();
	}
}

/*
===============
hfCommonLocal::SendUsercmd
===============
*/
void hfCommonLocal::SendUsercmds( int localClientNum ) {
	if ( !mapSpawned ) {
		return;
	}
	int currentTime = Sys_Milliseconds();
	if ( currentTime < nextUsercmdSendTime ) {
		return;
	}
	hfLobbyBase & lobby = session->GetActingGameStateLobbyBase();
	if ( lobby.IsHost() ) {
		return;
	}
	// We always send the last NUM_USERCMD_SEND usercmds
	// Which may result in duplicate usercmds being sent in the case of a low net_ucmdRate
	// But the LZW compressor means the extra usercmds are not large and the redundancy can smooth packet loss
	byte buffer[hfPacketProcessor::MAX_FINAL_PACKET_SIZE];
	hfBitMsg msg( buffer, sizeof( buffer ) );
	hfSerializer ser( msg, true );
	usercmd_t empty;
	usercmd_t * last = &empty;
	
	usercmd_t * cmdBuffer[NUM_USERCMD_SEND];
	const int numCmds = userCmdMgr.GetPlayerCmds( localClientNum, cmdBuffer, NUM_USERCMD_SEND );
	msg.WriteByte( numCmds );
	for ( int i = 0; i < numCmds; i++ ) {
		cmdBuffer[i]->Serialize( ser, *last );
		
		last = cmdBuffer[i];
	}
	session->SendUsercmds( msg );

	nextUsercmdSendTime = MSEC_ALIGN_TO_FRAME( currentTime + net_ucmdRate.GetInteger() );
}

/*
===============
hfCommonLocal::NetReceiveUsercmds
===============
*/
void hfCommonLocal::NetReceiveUsercmds( int peer, hfBitMsg & msg ) {
	int clientNum = Game()->MapPeerToClient( peer );
	if ( clientNum == -1 ) {
		hfLib::Warning( "NetReceiveUsercmds: Could not find client for peer %d", peer );
		return;
	}

	NetReadUsercmds( clientNum, msg );
}

/*
===============
hfCommonLocal::NetReceiveReliable
===============
*/
void hfCommonLocal::NetReceiveReliable( int peer, int type, hfBitMsg & msg ) {
	int clientNum = Game()->MapPeerToClient( peer );
	// Only servers care about the client num. Band-aid for problems related to the host's peerIndex being -1 on clients.
	if ( common->IsServer() && clientNum == -1 ) {
		hfLib::Warning( "NetReceiveReliable: Could not find client for peer %d", peer );
		return;
	}

	const byte * msgData = msg.GetReadData() + msg.GetReadCount();
	int msgSize = msg.GetRemainingData();
	reliableMsg_t & reliable = reliableQueue.Alloc();
	reliable.client = clientNum;
	reliable.type = type;
	reliable.dataSize = msgSize;
	reliable.data = (byte *)Mem_Alloc( msgSize, TAG_NETWORKING );
	memcpy( reliable.data, msgData, msgSize );
}

/*
========================
hfCommonLocal::ProcessSnapshot
========================
*/
void hfCommonLocal::ProcessSnapshot( hfSnapShot & ss ) {
	int time = Sys_Milliseconds();

	snapTime = time;
	snapPrevious			= snapCurrent;
	snapCurrent.serverTime	= ss.GetTime();
	snapRate = snapCurrent.serverTime - snapPrevious.serverTime;


	static int lastReceivedLocalTime = 0;
	int timeSinceLastSnap = ( time - lastReceivedLocalTime );
	if ( net_debug_snapShotTime.GetBool() ) {
		hfLib::Printf( "^2ProcessSnapshot. delta serverTime: %d  delta localTime: %d \n", ( snapCurrent.serverTime-snapPrevious.serverTime ), timeSinceLastSnap );
	}
	lastReceivedLocalTime = time;

	/* JAF ?
	for ( int i = 0; i < MAX_PLAYERS; i++ ) {
		hfBitMsg msg;
		if ( ss.GetObjectMsgByID( hfSession::SS_PLAYER + i, msg ) ) {
			if ( msg.GetSize() == 0 ) {
				snapCurrent.players[ i ].valid = false;
				continue;
			}

			hfSerializer ser( msg, false );
			SerializePlayer( ser, snapCurrent.players[ i ] );
			snapCurrent.players[ i ].valid = true;

			extern hfCVar com_drawSnapshots;
			if ( com_drawSnapshots.GetInteger() == 3 ) {
				console->AddSnapObject( "players", msg.GetSize(), ss.CompareObject( &oldss, hfSession::SS_PLAYER + i ) );
			}
		}
	}
	*/

	// Read usercmds from other players
	for ( int p = 0; p < MAX_PLAYERS; p++ ) {
		if ( p == game->GetLocalClientNum() ) {
			continue;
		}
		hfBitMsg msg;
		if ( ss.GetObjectMsgByID( SNAP_USERCMDS + p, msg ) ) {
			NetReadUsercmds( p, msg );
		}
	}




	// Set server game time here so that it accurately reflects the time when this frame was saved out, in case any serialize function needs it.
	int oldTime = Game()->GetServerGameTimeMs();		
	Game()->SetServerGameTimeMs( snapCurrent.serverTime );

	Game()->ClientReadSnapshot( ss ); //, &oldss );

	// Restore server game time
	Game()->SetServerGameTimeMs( oldTime );

	snapTimeDelta = ss.GetRecvTime() - oldss.GetRecvTime();
	oldss = ss;
}

/*
========================
hfCommonLocal::NetReadUsercmds
========================
*/
void hfCommonLocal::NetReadUsercmds( int clientNum, hfBitMsg & msg ) {
	if ( clientNum == -1 ) {
		hfLib::Warning( "NetReadUsercmds: Trying to read commands from invalid clientNum %d", clientNum );
		return;
	}
	
	// TODO: This shouldn't actually happen. Figure out why it does.
	// Seen on clients when another client leaves a match.
	if ( msg.GetReadData() == NULL ) {
		return;
	}

	hfSerializer ser( msg, false );
	
	usercmd_t fakeCmd;
	usercmd_t * base = &fakeCmd;

	usercmd_t lastCmd;

	bool										gotNewCmd = false;
	hfStaticList< usercmd_t, NUM_USERCMD_RELAY >	newCmdBuffer;

	usercmd_t baseCmd = userCmdMgr.NewestUserCmdForPlayer( clientNum );
	int curMilliseconds = baseCmd.clientGameMilliseconds;

	const int numCmds = msg.ReadByte();

	for ( int i = 0; i < numCmds; i++ ) {
		usercmd_t newCmd;
		newCmd.Serialize( ser, *base );
		
		lastCmd = newCmd;
		base = &lastCmd;

		int newMilliseconds = newCmd.clientGameMilliseconds;

		if ( newMilliseconds > curMilliseconds ) {
			if ( verify( i < NUM_USERCMD_RELAY ) ) {
				newCmdBuffer.Append( newCmd );
				gotNewCmd = true;
				curMilliseconds = newMilliseconds;
			}
		}
	}
	
	// Push the commands into the buffer.
	for ( int i = 0; i < newCmdBuffer.Num(); ++i ) {
		userCmdMgr.PutUserCmdForPlayer( clientNum, newCmdBuffer[i] );
	}
}

/*
========================
hfCommonLocal::ProcessNextSnapshot
========================
*/
void hfCommonLocal::ProcessNextSnapshot() {
	if ( readSnapshotIndex == writeSnapshotIndex ) {
		hfLib::Printf("No snapshots to process.\n");
		return;		// No snaps to process
	}
	ProcessSnapshot( receivedSnaps[ readSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE ] );		
	readSnapshotIndex++;
}

/*
========================
hfCommonLocal::CalcSnapTimeBuffered
Return the amount of game time left of buffered snapshots
totalBufferedTime - total amount of snapshot time (includng what we've already past in current interpolate)
totalRecvTime - total real time (sys_milliseconds) all of totalBufferedTime was received over
========================
*/
int hfCommonLocal::CalcSnapTimeBuffered( int & totalBufferedTime, int & totalRecvTime ) {

	totalBufferedTime = snapRate;
	totalRecvTime = snapTimeDelta;

	// oldSS = last ss we deserialized
	int lastBuffTime = oldss.GetTime();		
	int lastRecvTime = oldss.GetRecvTime();

	// receivedSnaps[readSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE] = next buffered snapshot we haven't processed yet (might not exist)
	for ( int i = readSnapshotIndex; i < writeSnapshotIndex; i++ ) {
		int buffTime = receivedSnaps[i % RECEIVE_SNAPSHOT_BUFFER_SIZE].GetTime();
		int recvTime = receivedSnaps[i % RECEIVE_SNAPSHOT_BUFFER_SIZE].GetRecvTime();

		totalBufferedTime += buffTime - lastBuffTime;
		totalRecvTime += recvTime - lastRecvTime;

		lastRecvTime = recvTime;
		lastBuffTime = buffTime;
	}

	totalRecvTime = Max( 1, totalRecvTime );
	totalRecvTime = static_cast<float>( initialBaseTicksPerSec ) * static_cast<float>( totalRecvTime / 1000.0f ); // convert realMS to gameMS

	// remove time we've already interpolated over
	int timeLeft = totalBufferedTime - Min< int >( snapRate, snapCurrentTime ); 

	//hfLib::Printf( "CalcSnapTimeBuffered. timeLeft: %d totalRecvTime: %d, totalTimeBuffered: %d\n", timeLeft, totalRecvTime, totalBufferedTime );
	return timeLeft;
}

/*
========================
hfCommonLocal::InterpolateSnapshot
========================
*/
void hfCommonLocal::InterpolateSnapshot( netTimes_t & prev, netTimes_t & next, float fraction, bool predict ) {

	int serverTime = Lerp( prev.serverTime, next.serverTime, fraction );

	Game()->SetServerGameTimeMs( serverTime );		// Set the global server time to the interpolated time of the server
	Game()->SetInterpolation( fraction, serverTime, prev.serverTime, next.serverTime );

	//Game()->RunFrame( &userCmdMgr, &ret, true );
	
}

/*
========================
hfCommonLocal::RunNetworkSnapshotFrame
========================
*/
void hfCommonLocal::RunNetworkSnapshotFrame() {

	// Process any reliable messages we've received
	for ( int i = 0; i < reliableQueue.Num(); i++ ) {
		game->ProcessReliableMessage( reliableQueue[i].client, reliableQueue[i].type, hfBitMsg( (const byte *)reliableQueue[i].data, reliableQueue[i].dataSize ) );
		Mem_Free( reliableQueue[i].data );
	}
	reliableQueue.Clear();

	// abuse the game timing to time presentable thinking on clients
	time_gameFrame = Sys_Microseconds();
	time_maxGameFrame = 0;
	count_numGameFrames = 0;

	if ( snapPrevious.serverTime >= 0 ) {

		int	msec_interval = 1 + hfMath::Ftoi( (float)initialBaseTicksPerSec  );

		static int clientTimeResidual = 0;
		static int lastTime = Sys_Milliseconds();
		int currentTime = Sys_Milliseconds();
		int deltaFrameTime = hfMath::ClampInt( 1, 33, currentTime - lastTime );

		clientTimeResidual += hfMath::ClampInt( 0, 50, currentTime - lastTime );
		lastTime = currentTime;

		extern hfCVar com_fixedTic;
		if ( com_fixedTic.GetBool() ) {
			clientTimeResidual = 0;
		}

		do {
			// If we are extrapolating and have fresher snapshots, then use the freshest one
			while ( ( snapCurrentTime >= snapRate || com_forceLatestSnap.GetBool() ) && readSnapshotIndex < writeSnapshotIndex ) {
				snapCurrentTime -= snapRate;
				ProcessNextSnapshot();
			}

			// this only matters when running < 60 fps
			// JAF Game()->GetRenderWorld()->UpdateDeferredPositions();

			// Clamp the current time so that it doesn't fall outside of our extrapolation bounds
			snapCurrentTime = hfMath::ClampInt( 0, snapRate + Min( (int)snapRate, (int)net_maxExtrapolationInMS.GetInteger() ), snapCurrentTime );

			if ( snapRate <= 0 ) {
				hfLib::Warning("snapRate <= 0. Resetting to 100");
				snapRate = 100;
			}

			float fraction = (float)snapCurrentTime / (float)snapRate;		
			if ( !IsValid( fraction ) ) {
				hfLib::Warning("Interpolation Fraction invalid: snapCurrentTime %d / snapRate %d", (int)snapCurrentTime, (int)snapRate );
				fraction = 0.0f;
			}
			
			InterpolateSnapshot( snapPrevious, snapCurrent, fraction, true );

			// Default to a snap scale of 1
			float snapRateScale = net_interpolationBaseRate.GetFloat();

			snapTimeBuffered = CalcSnapTimeBuffered( totalBufferedTime, totalRecvTime );
			effectiveSnapRate = static_cast< float > ( totalBufferedTime ) / static_cast< float > ( totalRecvTime );

			if ( net_minBufferedSnapPCT_Static.GetFloat() > 0.0f ) {
				optimalPCTBuffer = session->GetTitleStorageFloat( "net_minBufferedSnapPCT_Static", net_minBufferedSnapPCT_Static.GetFloat() );
			}

			// Calculate optimal amount of buffered time we want
			if ( net_optimalDynamic.GetBool() ) {
				optimalTimeBuffered = hfMath::ClampInt( 0, net_maxBufferedSnapMS.GetInteger(), snapRate * optimalPCTBuffer );
				optimalTimeBufferedWindow = snapRate * net_minBufferedSnapWinPCT_Static.GetFloat();
			} else {
				optimalTimeBuffered = net_optimalSnapTime.GetFloat();
				optimalTimeBufferedWindow = net_optimalSnapWindow.GetFloat();
			}

			// Scale snapRate based on where we are in the buffer
			if ( snapTimeBuffered <= optimalTimeBuffered ) {
				if ( snapTimeBuffered <= hfMath::FLT_SMALLEST_NON_DENORMAL ) {
					snapRateScale = 0;
				} else {
					snapRateScale = net_interpolationFallbackRate.GetFloat();
					// When we interpolate past our cushion of buffered snapshot, we want to slow smoothly slow the
					// rate of interpolation. frac will go from 1.0 to 0.0 (if snapshots stop coming in).
					float startSlowdown = ( net_interpolationSlowdownStart.GetFloat() * optimalTimeBuffered );
					if ( startSlowdown > 0 && snapTimeBuffered < startSlowdown ) {
						float frac = hfMath::ClampFloat( 0.0f, 1.0f, snapTimeBuffered / startSlowdown );
						if ( !IsValid( frac ) ) {
							frac = 0.0f;
						}
						snapRateScale = Square( frac ) * snapRateScale;
						if ( !IsValid( snapRateScale ) ) {
							snapRateScale = 0.0f;
						}
					}
				}


			} else if ( snapTimeBuffered > optimalTimeBuffered + optimalTimeBufferedWindow ) {
				// Go faster
				snapRateScale = net_interpolationCatchupRate.GetFloat();

			}

			float delta_interpolate = (float)initialBaseTicksPerSec * snapRateScale;
			if ( net_effectiveSnapRateEnable.GetBool() ) {

				float deltaFrameGameMS = static_cast<float>( initialBaseTicksPerSec ) * static_cast<float>( deltaFrameTime / 1000.0f );
				delta_interpolate = ( deltaFrameGameMS * snapRateScale * effectiveSnapRate ) + snapCurrentResidual;
				if ( !IsValid( delta_interpolate ) ) {
					delta_interpolate = 0.0f;
				}

				snapCurrentResidual = hfMath::Frac( delta_interpolate ); // fixme: snapCurrentTime should just be a float, but would require changes in d4 too
				if ( !IsValid( snapCurrentResidual ) ) {
					snapCurrentResidual = 0.0f;
				}

				if ( net_effectiveSnapRateDebug.GetBool() ) {
					hfLib::Printf("%d/%.2f snapRateScale: %.2f effectiveSR: %.2f d.interp: %.2f snapTimeBuffered: %.2f res: %.2f\n", deltaFrameTime, deltaFrameGameMS, snapRateScale, effectiveSnapRate, delta_interpolate, snapTimeBuffered, snapCurrentResidual );
				}
			}

			assert( IsValid( delta_interpolate ) );
			int interpolate_interval = hfMath::Ftoi( delta_interpolate );

			snapCurrentTime += interpolate_interval;	// advance interpolation time by the scaled interpolate_interval
			clientTimeResidual -= msec_interval;		// advance local client residual time (fixed step)

		} while ( clientTimeResidual >= msec_interval );

		if ( clientTimeResidual < 0 ) {
			clientTimeResidual = 0;
		}
	}

	time_gameFrame = Sys_Microseconds() - time_gameFrame;
}

/*
========================
hfCommonLocal::ExecuteReliableMessages
========================
*/
void hfCommonLocal::ExecuteReliableMessages() {

	// Process any reliable messages we've received
	for ( int i = 0; i < reliableQueue.Num(); i++ ) {
		reliableMsg_t & reliable = reliableQueue[i];
		game->ProcessReliableMessage( reliable.client, reliable.type, hfBitMsg( (const byte *)reliable.data, reliable.dataSize ) );
		Mem_Free( reliable.data );
	}
	reliableQueue.Clear();

}

/*
========================
hfCommonLocal::ResetNetworkingState
========================
*/
void hfCommonLocal::ResetNetworkingState() {	
	snapTime		= 0;
	snapTimeWrite	= 0;
	snapCurrentTime	= 0;
	snapCurrentResidual = 0.0f;

	snapTimeBuffered	= 0.0f;
	effectiveSnapRate	= 0.0f;
	totalBufferedTime	= 0;
	totalRecvTime		= 0;

	readSnapshotIndex	= 0;
	writeSnapshotIndex	= 0;
	snapRate			= 100000;
	optimalTimeBuffered	= 0.0f;
	optimalPCTBuffer	= 0.5f;
	optimalTimeBufferedWindow = 0.0;
	
	// Clear snapshot queue
	for ( int i = 0; i < RECEIVE_SNAPSHOT_BUFFER_SIZE; i++ ) {
		receivedSnaps[i].Clear();
	}

	userCmdMgr.SetDefaults();
	
	snapCurrent.localTime	= -1;
	snapPrevious.localTime	= -1;
	snapCurrent.serverTime	= -1;
	snapPrevious.serverTime = -1;

	// Make sure our current snap state is cleared so state from last game doesn't carry over into new game
	oldss.Clear();

	gameFrame = 0;
	clientPrediction = 0;
	nextUsercmdSendTime = 0;
	nextSnapshotSendTime = 0;
}

