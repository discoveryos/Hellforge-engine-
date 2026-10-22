#ifndef __HF_NET_REPLICATION_SYSTEM_H__
#define __HF_NET_REPLICATION_SYSTEM_H__

/*
===============================================================================

	Phase 10 — hfReplicationSystem.

	Moving the deterministic sim across a link. Four pieces, separated so the
	harness can test each without a socket:

	hfNetTransport        "ENet-style transport behind an interface". The
	                      interface is what the rest of the code sees; the
	                      loopback implementation delivers packets in process
	                      and can inject duplicates/reordering so the sequenced
	                      channel is exercised.

	hfNetChannel          sequence validation: dedup plus an age window, the
	                      part of a reliable/sequenced protocol that decides
	                      whether an arriving packet is usable at all.

	hfNetReplicator       snapshot serialization of transform / physics state
	                      and input forwarding, built on hfBitMsg (bit-level,
	                      alignment-safe, byte-order aware).

	hfNetLagCompensator   the lag-compensation hook: a ring of past snapshots
	                      keyed by step, so a peer's acknowledged step can be
	                      rewound to exactly.

	hfReplicationSystem   the scene system. As authority it serializes every
	                      hfNetComponent's transform and sends; as a remote it
	                      receives and applies. Delta encoding means an
	                      unchanged entity costs one bit.

Because the sim is fixed-rate and deterministic, replication is a copy problem
rather than a reconciliation problem: replaying the same input log anywhere
yields the same bits, which is what Test_Net proves.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "hfNetComponent.h"
#include "../hfBitMsg.h"
#include "../scene/hfSystem.h"
#include "../server/hfSimulationServer.h"

namespace HF_Net {

// sanity bound on how many entities one STATE packet may carry
static const int HF_NET_MAX_STATE_ENTITIES = 4096;

// 'HFN1'
static const int HF_NET_MAGIC = 0x48464E31;

enum hfNetMsgType {
	HF_NET_MSG_STATE = 0,	// hfReplicationSystem transform snapshot
	HF_NET_MSG_SIM,			// HF_Server::hfSimSnapshot (physics state)
	HF_NET_MSG_INPUTS,		// forwarded input log (client -> server)
	HF_NET_MSG_ACK,			// client acknowledges a snapshot step
	HF_NET_MSG_TYPE_COUNT
};

// --- packet on the wire ---------------------------------------------------

struct hfNetEnvelope {
	unsigned int	fromPeer;
	unsigned int	channel;
	unsigned int sequence;
	int				len;
	byte			data[ HF_NET_MAX_PACKET ];
};

// --- transport interface ---------------------------------------------------

class hfNetTransport {
public:
	virtual				~hfNetTransport() {}
	virtual bool		Send( unsigned int from, unsigned int to, unsigned int channel,
							  const void *data, int len ) = 0;
	virtual int			Receive( unsigned int peer, hfNetEnvelope *out, int maxOut ) = 0;
	virtual void		Reset() = 0;
};

// In-process transport: one queue per peer. Fault injection is off by default
// so tests get deterministic delivery; the flags exist so the sequenced
// channel can be proven to reject duplicates and out-of-order arrivals.
class hfNetLoopbackTransport : public hfNetTransport {
public:
	static const int	QUEUE = 32;

	hfNetLoopbackTransport() : m_reorder( false ), m_duplicate( false ) {
		for ( int p = 0; p < HF_NET_MAX_PEERS; p++ ) {
			m_queue[ p ] = new hfNetEnvelope[ QUEUE ];
			m_head[ p ] = 0;
			m_count[ p ] = 0;
			for ( int c = 0; c < HF_NET_CHANNEL_COUNT; c++ ) { m_sendSeq[ p ][ c ] = 1; }
		}
	}
	virtual ~hfNetLoopbackTransport() {
		for ( int p = 0; p < HF_NET_MAX_PEERS; p++ ) { delete[] m_queue[ p ]; }
	}

	void				SetReorder( bool on ) { m_reorder = on; }
	void				SetDuplicate( bool on ) { m_duplicate = on; }

	virtual bool		Send( unsigned int from, unsigned int to, unsigned int channel,
							  const void *data, int len ) {
		int ch = channel < ( unsigned int )HF_NET_CHANNEL_COUNT ? ( int )channel : 0;
		return SendRaw( from, to, channel, m_sendSeq[ from ][ ch ]++, data, len );
	}

	// explicit sequence, so a test can replay or forge a packet
	bool				SendRaw( unsigned int from, unsigned int to, unsigned int channel,
								 unsigned int sequence, const void *data, int len ) {
		if ( to >= ( unsigned int )HF_NET_MAX_PEERS ) { return false; }
		if ( len < 0 || len > HF_NET_MAX_PACKET ) { return false; }
		if ( !Append( from, to, channel, sequence, data, len ) ) { return false; }
		if ( m_duplicate ) {
			if ( !Append( from, to, channel, sequence, data, len ) ) { return false; }
		}
		if ( m_reorder ) {
			// move the newest packet to the front of the queue
			int head = m_head[ to ];
			int last = ( head + m_count[ to ] - 1 ) % QUEUE;
			hfNetEnvelope tmp = m_queue[ to ][ last ];
			for ( int i = m_count[ to ] - 1; i > 0; i-- ) {
				int a = ( head + i ) % QUEUE;
				int b = ( head + i - 1 ) % QUEUE;
				m_queue[ to ][ a ] = m_queue[ to ][ b ];
			}
			m_queue[ to ][ head ] = tmp;
		}
		return true;
	}

	virtual int			Receive( unsigned int peer, hfNetEnvelope *out, int maxOut ) {
		if ( peer >= ( unsigned int )HF_NET_MAX_PEERS || !out || maxOut <= 0 ) { return 0; }
		int n = 0;
		while ( n < maxOut && m_count[ peer ] > 0 ) {
			out[ n ] = m_queue[ peer ][ m_head[ peer ] ];
			m_head[ peer ] = ( m_head[ peer ] + 1 ) % QUEUE;
			m_count[ peer ]--;
			n++;
		}
		return n;
	}

	virtual void		Reset() {
		for ( int p = 0; p < HF_NET_MAX_PEERS; p++ ) {
			m_head[ p ] = 0;
			m_count[ p ] = 0;
			for ( int c = 0; c < HF_NET_CHANNEL_COUNT; c++ ) { m_sendSeq[ p ][ c ] = 1; }
		}
	}

	int					Pending( unsigned int peer ) const {
		if ( peer >= ( unsigned int )HF_NET_MAX_PEERS ) { return 0; }
		return m_count[ peer ];
	}

private:
	bool				Append( unsigned int from, unsigned int to, unsigned int channel,
								unsigned int sequence, const void *data, int len ) {
		if ( m_count[ to ] >= QUEUE ) { return false; }
		int slot = ( m_head[ to ] + m_count[ to ] ) % QUEUE;
		hfNetEnvelope &e = m_queue[ to ][ slot ];
		e.fromPeer = from;
		e.channel = channel;
		e.sequence = sequence;
		e.len = len;
		if ( len > 0 ) { memcpy( e.data, data, ( size_t )len ); }
		m_count[ to ]++;
		return true;
	}

	hfNetEnvelope *		m_queue[ HF_NET_MAX_PEERS ];
	int					m_head[ HF_NET_MAX_PEERS ];
	int					m_count[ HF_NET_MAX_PEERS ];
	unsigned int		m_sendSeq[ HF_NET_MAX_PEERS ][ HF_NET_CHANNEL_COUNT ];
	bool				m_reorder;
	bool				m_duplicate;
};

// Sequence validation for an arriving stream: drop anything already seen, and
// anything older than the window. Out-of-order arrivals inside the window are
// accepted (the unreliable path); full ordered delivery for the reliable
// channel is layered on top by the caller and is a known gap here.
class hfNetChannel {
public:
	static const unsigned int WINDOW = 256;

	hfNetChannel() { Reset(); }

	void				Reset() {
		for ( unsigned int i = 0; i < WINDOW; i++ ) { m_seen[ i ] = 0; }
		m_recvTop = 0;
		m_accepted = 0;
		m_duplicates = 0;
		m_tooOld = 0;
		m_nextSend = 1;
	}

	unsigned int		NextSend() { return m_nextSend++; }

	bool				Accept( unsigned int seq ) {
		if ( seq == 0 ) { m_duplicates++; return false; }			// 0 is never a valid sequence
		if ( seq + WINDOW <= m_recvTop ) { m_tooOld++; return false; }
		unsigned int slot = seq % WINDOW;
		if ( m_seen[ slot ] == seq ) { m_duplicates++; return false; }
		m_seen[ slot ] = seq;
		if ( seq >= m_recvTop ) { m_recvTop = seq + 1; }
		m_accepted++;
		return true;
	}

	unsigned int		GetAccepted() const { return m_accepted; }
	unsigned int		GetDuplicates() const { return m_duplicates; }
	unsigned int		GetTooOld() const { return m_tooOld; }
	unsigned int		GetRecvTop() const { return m_recvTop; }

private:
	unsigned int		m_seen[ WINDOW ];
	unsigned int		m_recvTop;
	unsigned int		m_accepted;
	unsigned int		m_duplicates;
	unsigned int		m_tooOld;
	unsigned int		m_nextSend;
};

// --- serialization ---------------------------------------------------------

class hfNetReplicator {
public:
	// --- physics state (HF_Server::hfSimSnapshot) ---
	static int			WriteSim( const HF_Server::hfSimSnapshot &snap,
								  byte *out, int outSize, unsigned int sequence );
	static bool			ReadSim( const byte *in, int inLen, HF_Server::hfSimSnapshot *out,
								 unsigned int *sequenceOut );

	// delta against a baseline: a body that did not move costs one bit.
	// `base` is also the decode scratch (the reader fills the baseline then
	// patches the changed bodies), so it must be the state you are patching.
	static int			WriteSimDelta( const HF_Server::hfSimSnapshot &snap,
									   const HF_Server::hfSimSnapshot *base,
									   byte *out, int outSize, unsigned int sequence );
	static bool			ReadSimDelta( const byte *in, int inLen,
									  HF_Server::hfSimSnapshot *base,
									  HF_Server::hfSimSnapshot *out,
									  unsigned int *sequenceOut );

	// --- input forwarding ---
	static int			WriteInputs( const HF_Server::hfSimInput *inputs, int count,
									 byte *out, int outSize, unsigned int sequence );
	static int			ReadInputs( const byte *in, int inLen,
									HF_Server::hfSimInput *out, int maxOut,
									unsigned int *sequenceOut );
};

HF_INLINE int hfNetReplicator::WriteSim( const HF_Server::hfSimSnapshot &snap,
										 byte *out, int outSize, unsigned int sequence ) {
	if ( !out || outSize <= 0 ) { return 0; }
	hfBitMsg msg( out, outSize );
	msg.BeginWriting();
	msg.WriteLong( HF_NET_MAGIC );
	msg.WriteLong( HF_NET_MSG_SIM );
	msg.WriteLong( ( int )sequence );
	msg.WriteLong( ( int )snap.step );
	msg.WriteLong( ( int )snap.checksum );
	msg.WriteLong( ( int )snap.sequence );
	msg.WriteFloat( snap.simTime );
	msg.WriteLong( ( int )snap.seed );
	msg.WriteLong( ( int )snap.inputsApplied );
	msg.WriteLong( snap.bodyCount );
	for ( int i = 0; i < snap.bodyCount; i++ ) {
		const HF_Server::hfSimBody &b = snap.bodies[ i ];
		msg.WriteLong( ( int )b.entity );
		msg.WriteFloat( b.position.x );
		msg.WriteFloat( b.position.y );
		msg.WriteFloat( b.position.z );
		msg.WriteFloat( b.rotation.x );
		msg.WriteFloat( b.rotation.y );
		msg.WriteFloat( b.rotation.z );
		msg.WriteFloat( b.rotation.w );
		msg.WriteFloat( b.velocity.x );
		msg.WriteFloat( b.velocity.y );
		msg.WriteFloat( b.velocity.z );
		msg.WriteBool( b.asleep );
	}
	if ( msg.IsOverflowed() ) { return 0; }
	msg.WriteByteAlign();
	return msg.GetSize();
}

HF_INLINE bool hfNetReplicator::ReadSim( const byte *in, int inLen,
										 HF_Server::hfSimSnapshot *out,
										 unsigned int *sequenceOut ) {
	if ( !in || !out ) { return false; }
	hfBitMsg msg( in, inLen );
	msg.BeginReading();
	if ( msg.ReadLong() != HF_NET_MAGIC ) { return false; }
	if ( msg.ReadLong() != HF_NET_MSG_SIM ) { return false; }
	{
		unsigned int packetSeq = ( unsigned int )msg.ReadLong();
		if ( sequenceOut ) { *sequenceOut = packetSeq; }
	}
	out->step = ( unsigned int )msg.ReadLong();
	unsigned int checksum = ( unsigned int )msg.ReadLong();
	out->checksum = checksum;
	out->sequence = ( unsigned int )msg.ReadLong();
	out->simTime = msg.ReadFloat();
	out->seed = ( unsigned int )msg.ReadLong();
	out->inputsApplied = ( unsigned int )msg.ReadLong();
	int count = msg.ReadLong();
	if ( count < 0 || count > HF_Server::HF_SIM_MAX_BODIES ) { return false; }
	out->bodyCount = count;
	for ( int i = 0; i < count; i++ ) {
		HF_Server::hfSimBody &b = out->bodies[ i ];
		b.entity = ( HF_Ecs::Entity )msg.ReadLong();
		b.position.x = msg.ReadFloat();
		b.position.y = msg.ReadFloat();
		b.position.z = msg.ReadFloat();
		b.rotation.x = msg.ReadFloat();
		b.rotation.y = msg.ReadFloat();
		b.rotation.z = msg.ReadFloat();
		b.rotation.w = msg.ReadFloat();
		b.velocity.x = msg.ReadFloat();
		b.velocity.y = msg.ReadFloat();
		b.velocity.z = msg.ReadFloat();
		b.asleep = msg.ReadBool();
	}
	return true;
}

HF_INLINE int hfNetReplicator::WriteSimDelta( const HF_Server::hfSimSnapshot &snap,
											  const HF_Server::hfSimSnapshot *base,
											  byte *out, int outSize,
											  unsigned int sequence ) {
	if ( !out || outSize <= 0 ) { return 0; }
	hfBitMsg msg( out, outSize );
	msg.BeginWriting();
	msg.WriteLong( HF_NET_MAGIC );
	msg.WriteLong( HF_NET_MSG_SIM );
	msg.WriteLong( ( int )sequence );
	msg.WriteLong( ( int )snap.step );
	msg.WriteLong( ( int )snap.checksum );
	msg.WriteLong( ( int )snap.sequence );
	msg.WriteFloat( snap.simTime );
	msg.WriteLong( ( int )snap.seed );
	msg.WriteLong( ( int )snap.inputsApplied );
	msg.WriteLong( snap.bodyCount );

	const bool haveBase = ( base != NULL ) && ( base->bodyCount == snap.bodyCount );
	for ( int i = 0; i < snap.bodyCount; i++ ) {
		const HF_Server::hfSimBody &b = snap.bodies[ i ];
		const HF_Server::hfSimBody *o = haveBase ? &base->bodies[ i ] : NULL;
		bool changed = !o ||
			b.entity != o->entity ||
			b.position.x != o->position.x || b.position.y != o->position.y || b.position.z != o->position.z ||
			b.rotation.x != o->rotation.x || b.rotation.y != o->rotation.y ||
			b.rotation.z != o->rotation.z || b.rotation.w != o->rotation.w ||
			b.velocity.x != o->velocity.x || b.velocity.y != o->velocity.y || b.velocity.z != o->velocity.z ||
			b.asleep != o->asleep;
		msg.WriteBool( changed );
		if ( !changed ) { continue; }
		msg.WriteLong( ( int )b.entity );
		msg.WriteFloat( b.position.x );
		msg.WriteFloat( b.position.y );
		msg.WriteFloat( b.position.z );
		msg.WriteFloat( b.rotation.x );
		msg.WriteFloat( b.rotation.y );
		msg.WriteFloat( b.rotation.z );
		msg.WriteFloat( b.rotation.w );
		msg.WriteFloat( b.velocity.x );
		msg.WriteFloat( b.velocity.y );
		msg.WriteFloat( b.velocity.z );
		msg.WriteBool( b.asleep );
	}
	if ( msg.IsOverflowed() ) { return 0; }
	msg.WriteByteAlign();
	return msg.GetSize();
}

HF_INLINE bool hfNetReplicator::ReadSimDelta( const byte *in, int inLen,
											  HF_Server::hfSimSnapshot *base,
											  HF_Server::hfSimSnapshot *out,
											  unsigned int *sequenceOut ) {
	if ( !in || !base || !out ) { return false; }
	hfBitMsg msg( in, inLen );
	msg.BeginReading();
	if ( msg.ReadLong() != HF_NET_MAGIC ) { return false; }
	if ( msg.ReadLong() != HF_NET_MSG_SIM ) { return false; }
	{
		unsigned int packetSeq = ( unsigned int )msg.ReadLong();
		if ( sequenceOut ) { *sequenceOut = packetSeq; }
	}
	unsigned int step = ( unsigned int )msg.ReadLong();
	unsigned int checksum = ( unsigned int )msg.ReadLong();
	unsigned int sequence = ( unsigned int )msg.ReadLong();
	float simTime = msg.ReadFloat();
	unsigned int seed = ( unsigned int )msg.ReadLong();
	unsigned int inputsApplied = ( unsigned int )msg.ReadLong();
	int count = msg.ReadLong();
	if ( count < 0 || count > HF_Server::HF_SIM_MAX_BODIES ) { return false; }

	// start from the baseline so bodies the packet left untouched keep their
	// previous values, then patch the ones flagged as changed
	*out = *base;
	out->step = step;
	out->checksum = checksum;
	out->sequence = sequence;
	out->simTime = simTime;
	out->seed = seed;
	out->inputsApplied = inputsApplied;
	out->bodyCount = count;
	for ( int i = 0; i < count; i++ ) {
		bool changed = msg.ReadBool();
		if ( !changed ) { continue; }
		HF_Server::hfSimBody &b = out->bodies[ i ];
		b.entity = ( HF_Ecs::Entity )msg.ReadLong();
		b.position.x = msg.ReadFloat();
		b.position.y = msg.ReadFloat();
		b.position.z = msg.ReadFloat();
		b.rotation.x = msg.ReadFloat();
		b.rotation.y = msg.ReadFloat();
		b.rotation.z = msg.ReadFloat();
		b.rotation.w = msg.ReadFloat();
		b.velocity.x = msg.ReadFloat();
		b.velocity.y = msg.ReadFloat();
		b.velocity.z = msg.ReadFloat();
		b.asleep = msg.ReadBool();
	}
	return true;
}

HF_INLINE int hfNetReplicator::WriteInputs( const HF_Server::hfSimInput *inputs, int count,
											 byte *out, int outSize, unsigned int sequence ) {
	if ( !out || outSize <= 0 || count < 0 ) { return 0; }
	hfBitMsg msg( out, outSize );
	msg.BeginWriting();
	msg.WriteLong( HF_NET_MAGIC );
	msg.WriteLong( HF_NET_MSG_INPUTS );
	msg.WriteLong( ( int )sequence );
	msg.WriteLong( count );
	for ( int i = 0; i < count; i++ ) {
		msg.WriteLong( ( int )inputs[ i ].step );
		msg.WriteLong( ( int )inputs[ i ].entity );
		msg.WriteFloat( inputs[ i ].impulse.x );
		msg.WriteFloat( inputs[ i ].impulse.y );
		msg.WriteFloat( inputs[ i ].impulse.z );
	}
	if ( msg.IsOverflowed() ) { return 0; }
	msg.WriteByteAlign();
	return msg.GetSize();
}

HF_INLINE int hfNetReplicator::ReadInputs( const byte *in, int inLen,
											HF_Server::hfSimInput *out, int maxOut,
											unsigned int *sequenceOut ) {
	if ( !in || !out || maxOut <= 0 ) { return 0; }
	hfBitMsg msg( in, inLen );
	msg.BeginReading();
	if ( msg.ReadLong() != HF_NET_MAGIC ) { return 0; }
	if ( msg.ReadLong() != HF_NET_MSG_INPUTS ) { return 0; }
	{
		unsigned int packetSeq = ( unsigned int )msg.ReadLong();
		if ( sequenceOut ) { *sequenceOut = packetSeq; }
	}
	int count = msg.ReadLong();
	if ( count < 0 ) { return 0; }
	int n = count < maxOut ? count : maxOut;
	for ( int i = 0; i < n; i++ ) {
		out[ i ].step = ( unsigned int )msg.ReadLong();
		out[ i ].entity = ( HF_Ecs::Entity )msg.ReadLong();
		out[ i ].impulse.x = msg.ReadFloat();
		out[ i ].impulse.y = msg.ReadFloat();
		out[ i ].impulse.z = msg.ReadFloat();
	}
	return n;
}

// --- lag compensation ------------------------------------------------------

struct hfNetHistoryEntry {
	unsigned int		step;
	HF_Server::hfSimSnapshot	snapshot;
};

class hfNetLagCompensator {
public:
	hfNetLagCompensator() : m_entries( NULL ), m_capacity( 0 ), m_count( 0 ), m_head( 0 ) {
		SetCapacity( 16 );
	}
	~hfNetLagCompensator() { delete[] m_entries; }

	void				SetCapacity( int n ) {
		if ( n < 1 ) { n = 1; }
		delete[] m_entries;
		m_entries = new hfNetHistoryEntry[ n ];
		m_capacity = n;
		m_count = 0;
		m_head = 0;
	}

	void				Reset() { m_count = 0; m_head = 0; }

	// record the authoritative state for its step; a repeated step replaces
	void				Record( const HF_Server::hfSimSnapshot &snap ) {
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_entries[ i ].step == snap.step ) {
				m_entries[ i ].snapshot = snap;
				return;
			}
		}
		if ( m_count < m_capacity ) {
			m_entries[ m_count ].step = snap.step;
			m_entries[ m_count ].snapshot = snap;
			m_count++;
		} else {
			m_entries[ m_head ].step = snap.step;
			m_entries[ m_head ].snapshot = snap;
			m_head = ( m_head + 1 ) % m_capacity;
		}
	}

	// exact step only — a missing step is a miss, never an approximation
	const HF_Server::hfSimSnapshot *	Sample( unsigned int step ) const {
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_entries[ i ].step == step ) { return &m_entries[ i ].snapshot; }
		}
		return NULL;
	}

	unsigned int		NewestStep() const {
		unsigned int best = 0;
		for ( int i = 0; i < m_count; i++ ) {
			if ( i == 0 || m_entries[ i ].step > best ) { best = m_entries[ i ].step; }
		}
		return best;
	}

	int					GetCount() const { return m_count; }
	int					GetCapacity() const { return m_capacity; }

private:
	hfNetHistoryEntry *	m_entries;
	int					m_capacity;
	int					m_count;
	int					m_head;
};

// --- the scene system ------------------------------------------------------

class hfReplicationSystem : public HF_Scene::hfSystem {
public:
	hfReplicationSystem()
		: m_transport( NULL ), m_self( HF_NET_PEER_SERVER ), m_peer( HF_NET_PEER_SERVER ),
		  m_authority( true ), m_step( 0 ), m_seq( 1 ),
		  m_lastPacketBytes( 0 ), m_stateSendCount( 0 ), m_stateRecvCount( 0 ) {
		m_peerStats.Reset();
	}

	virtual const char *	GetName() const { return "ReplicationSystem"; }
	virtual int				GetPriority() const { return 95; }
	virtual void			OnRegister( HF_Ecs::Registry &registry ) { ( void )registry; }

	void					SetTransport( hfNetTransport *t ) { m_transport = t; m_channel.Reset(); }
	void					SetPeer( unsigned int self, unsigned int authority ) {
		m_self = self; m_peer = authority; m_peerStats.peerId = authority;
	}
	void					SetAuthority( bool on ) { m_authority = on; }
	void					SetStep( unsigned int step ) { m_step = step; }
	unsigned int			GetStep() const { return m_step; }
	bool					IsAuthority() const { return m_authority; }

	// send our replicated transforms (authority only)
	bool					SendState( HF_Ecs::Registry &registry ) {
		if ( !m_transport || !m_authority ) { return false; }

		int count = 0;
		registry.Each< hfNetComponent >(
			[ & ]( HF_Ecs::Entity, hfNetComponent & ) { count++; } );
		if ( count == 0 ) { return false; }

		byte buf[ HF_NET_MAX_PACKET ];
		hfBitMsg msg( buf, ( int )sizeof( buf ) );
		msg.BeginWriting();
		msg.WriteLong( HF_NET_MAGIC );
		msg.WriteLong( HF_NET_MSG_STATE );
		msg.WriteLong( ( int )m_seq );
		msg.WriteLong( ( int )m_step );
		msg.WriteLong( count );

		registry.Each< hfNetComponent >(
			[ & ]( HF_Ecs::Entity ent, hfNetComponent &c ) {
				HF_Scene::hfTransformComponent *t = registry.GetComponent< HF_Scene::hfTransformComponent >( ent );
				if ( !t ) { return; }
				bool changed = !c.hasPrev ||
					t->position != c.prevPosition ||
					t->rotation.x != c.prevRotation.x || t->rotation.y != c.prevRotation.y ||
					t->rotation.z != c.prevRotation.z || t->rotation.w != c.prevRotation.w;
				msg.WriteLong( c.netId.index );
				msg.WriteBool( changed );
				if ( changed ) {
					msg.WriteFloat( t->position.x );
					msg.WriteFloat( t->position.y );
					msg.WriteFloat( t->position.z );
					msg.WriteFloat( t->rotation.x );
					msg.WriteFloat( t->rotation.y );
					msg.WriteFloat( t->rotation.z );
					msg.WriteFloat( t->rotation.w );
				}
				c.prevPosition = t->position;
				c.prevRotation = t->rotation;
				c.hasPrev = true;
				c.dirty = false;
				c.replicatedStep = m_step;
			} );

		if ( msg.IsOverflowed() ) { return false; }
		msg.WriteByteAlign();
		int len = msg.GetSize();
		if ( !m_transport->Send( m_self, m_peer, HF_NET_CHANNEL_RELIABLE, buf, len ) ) { return false; }
		m_lastPacketBytes = len;
		m_seq++;
		m_stateSendCount++;
		m_peerStats.lastSentStep = m_step;
		m_peerStats.packetsOut++;
		return true;
	}

	// pull and apply (remote only)
	bool					ReceiveState( HF_Ecs::Registry &registry ) {
		if ( !m_transport || m_authority ) { return false; }

		hfNetEnvelope env[ 4 ];
		int got = m_transport->Receive( m_self, env, 4 );
		bool applied = false;
		for ( int i = 0; i < got; i++ ) {
			m_peerStats.packetsIn++;
			if ( !m_channel.Accept( env[ i ].sequence ) ) {
				m_peerStats.duplicatesDropped++;
				continue;
			}
			// const byte* selects hfBitMsg's read constructor (InitRead).
			// The byte* constructor is InitWrite, which leaves curSize at 0
			// and makes every Read come back as -1.
			hfBitMsg msg( ( const byte * )env[ i ].data, env[ i ].len );
			msg.BeginReading();
			if ( msg.ReadLong() != HF_NET_MAGIC ) { continue; }
			int type = msg.ReadLong();
			if ( type != HF_NET_MSG_STATE ) { continue; }
			msg.ReadLong();						// sender's sequence
			unsigned int step = ( unsigned int )msg.ReadLong();
			int count = msg.ReadLong();
			if ( count < 0 || count > HF_NET_MAX_STATE_ENTITIES ) { continue; }
			for ( int k = 0; k < count; k++ ) {
				int netId = msg.ReadLong();
				bool changed = msg.ReadBool();
				if ( !changed ) { continue; }
				float px = msg.ReadFloat();
				float py = msg.ReadFloat();
				float pz = msg.ReadFloat();
				float qx = msg.ReadFloat();
				float qy = msg.ReadFloat();
				float qz = msg.ReadFloat();
				float qw = msg.ReadFloat();

				HF_Ecs::Entity found = HF_Ecs::ENTITY_NULL;
				registry.Each< hfNetComponent >(
					[ & ]( HF_Ecs::Entity ent, hfNetComponent &c ) {
						if ( found == HF_Ecs::ENTITY_NULL && c.netId.index == netId ) { found = ent; }
					} );
				if ( found == HF_Ecs::ENTITY_NULL ) { continue; }
				HF_Scene::hfTransformComponent *t = registry.GetComponent< HF_Scene::hfTransformComponent >( found );
				HF_Net::hfNetComponent *c = registry.GetComponent< hfNetComponent >( found );
				if ( !t ) { continue; }
				t->position.Set( px, py, pz );
				t->rotation.x = qx; t->rotation.y = qy;
				t->rotation.z = qz; t->rotation.w = qw;
				t->dirty = true;
				if ( c ) {
					c->prevPosition = t->position;
					c->prevRotation = t->rotation;
					c->hasPrev = true;
					c->dirty = false;
					c->replicatedStep = step;
					c->lastAckStep = step;
				}
			}
			m_lastPacketBytes = env[ i ].len;
			m_stateRecvCount++;
			m_peerStats.lastAckedStep = step;
			applied = true;
		}
		return applied;
	}

	// the client tells the authority what it has, which is what the lag
	// compensator rewinds to
	void					Acknowledge( unsigned int step ) {
		if ( step > m_peerStats.lastAckedStep ) { m_peerStats.lastAckedStep = step; }
	}

	virtual void			Update( HF_Ecs::Registry &registry, float dt ) {
		( void )dt;
		if ( !m_transport ) { return; }
		if ( m_authority ) { SendState( registry ); }
		else { ReceiveState( registry ); }
	}

	int						GetLastPacketBytes() const { return m_lastPacketBytes; }
	unsigned int			GetSendSequence() const { return m_seq; }
	int						GetStateSendCount() const { return m_stateSendCount; }
	int						GetStateRecvCount() const { return m_stateRecvCount; }
	const hfNetPeerStats &	GetPeerStats() const { return m_peerStats; }
	hfNetChannel &			GetChannel() { return m_channel; }

private:
	hfNetTransport *		m_transport;
	hfNetChannel			m_channel;
	hfNetPeerStats			m_peerStats;
	unsigned int			m_self;
	unsigned int			m_peer;
	bool					m_authority;
	unsigned int			m_step;
	unsigned int			m_seq;
	int						m_lastPacketBytes;
	int						m_stateSendCount;
	int						m_stateRecvCount;
};

} // namespace HF_Net

#endif