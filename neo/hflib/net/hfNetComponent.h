#ifndef __HF_NET_COMPONENT_H__
#define __HF_NET_COMPONENT_H__

/*
===============================================================================

	Phase 10 — network components.

	Server-authoritative replication over the deterministic sim. The fixed
	60 Hz snapshot the simulation server already publishes is the whole point:
	there is no need to reconcile two independent integrations, only to move
	the same numbers across a link.

	This header holds the data:

	  - hfNetComponent   per-entity replication state (who owns it, what was
	                     last replicated, the previous transform for deltas)
	  - hfNetInputEvent  one input wrapped for the wire (client -> server)
	  - hfNetPeerStats   ack / round-trip bookkeeping, which is where lag
	                     compensation hooks in: a peer's acknowledged step is
	                     the step the server rewinds to when resolving a shot

	IDs are hfNetRid, an index handle in the same shape as hfRid/hfSimRid.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"

namespace HF_Net {

// stable replication id, assigned by the authority and never reused while a
// peer still holds the old one
struct hfNetRid {
	int		index;
	HF_INLINE hfNetRid() : index( -1 ) {}
	HF_INLINE explicit hfNetRid( int i ) : index( i ) {}
	HF_INLINE bool Valid() const { return index >= 0; }
	HF_INLINE bool operator==( const hfNetRid &o ) const { return index == o.index; }
	HF_INLINE bool operator!=( const hfNetRid &o ) const { return index != o.index; }
};

enum hfNetRole {
	HF_NET_ROLE_AUTHORITY = 0,		// simulates the world and replicates it out
	HF_NET_ROLE_REMOTE				// mirrors what the authority sent
};

enum hfNetChannelId {
	HF_NET_CHANNEL_RELIABLE = 0,	// ordered, retransmitted (inputs, state)
	HF_NET_CHANNEL_UNRELIABLE = 1,	// sequenced, may drop (voice, cosmetic)
	HF_NET_CHANNEL_COUNT
};

static const unsigned int	HF_NET_PEER_SERVER = 0;
static const int			HF_NET_MAX_PEERS = 8;
static const int			HF_NET_MAX_PACKET = 8192;

// one entity's replication state
struct hfNetComponent {
	hfNetRid			netId;
	hfNetRole			role;
	unsigned int		ownerPeer;			// peer that drives this entity (0 = server)
	unsigned int		lastAckStep;		// newest snapshot step the peer has confirmed
	unsigned int		replicatedStep;		// step of the state we last sent/received
	bool				dirty;				// changed since the last serialization

	// previous replicated transform, so the writer can send a delta
	hfVec3				prevPosition;
	hfQuat				prevRotation;
	bool				hasPrev;

	HF_INLINE hfNetComponent() {
		netId = hfNetRid();
		role = HF_NET_ROLE_AUTHORITY;
		ownerPeer = HF_NET_PEER_SERVER;
		lastAckStep = 0;
		replicatedStep = 0;
		dirty = true;
		prevPosition = hfVec3( 0.0f, 0.0f, 0.0f );
		prevRotation = hfQuat( 0.0f, 0.0f, 0.0f, 1.0f );
		hasPrev = false;
	}
};

// one input wrapped for the wire. `seq` orders a client's inputs so the
// server can drop duplicates and reorder.
struct hfNetInputEvent {
	unsigned int		step;
	hfNetRid			target;
	hfVec3				impulse;
	unsigned int		seq;

	HF_INLINE hfNetInputEvent() {
		step = 0;
		target = hfNetRid();
		impulse = hfVec3( 0.0f, 0.0f, 0.0f );
		seq = 0;
	}
};

// Per-peer bookkeeping. `lastAckedStep` is the lag-compensation hook: the
// server rewinds to that step when resolving something a client reported, so
// the world the client saw is the world the server resolves against.
struct hfNetPeerStats {
	unsigned int		peerId;
	unsigned int		lastAckedStep;
	unsigned int		lastSentStep;
	unsigned int		rttSteps;			// round trip measured in sim steps
	unsigned int		packetsIn;
	unsigned int		packetsOut;
	unsigned int		duplicatesDropped;
	unsigned int		outOfOrderDropped;

	HF_INLINE hfNetPeerStats() { Reset(); }
	HF_INLINE void Reset() {
		peerId = 0;
		lastAckedStep = 0;
		lastSentStep = 0;
		rttSteps = 0;
		packetsIn = 0;
		packetsOut = 0;
		duplicatesDropped = 0;
		outOfOrderDropped = 0;
	}
};

} // namespace HF_Net

#endif