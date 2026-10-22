#ifndef __HF_PHYSICS_BROADPHASE_H__
#define __HF_PHYSICS_BROADPHASE_H__

#include "../hfEcs.h"
#include "../bv/hfBounds.h"

namespace HF_Physics {

// Sort-and-sweep broadphase on the X axis. World AABBs in, overlapping
// candidate pairs out, self-deduplicating by construction (pairs are
// emitted as i < j in sweep order, so no pair hashing is needed). The
// classic >n-probability of a single-axis sweep is fine for game scenes;
// a uniform grid or BVH can slot in behind the same interface later.

static const int MAX_BROADPHASE_ENTRIES	= 1024;
static const int MAX_BROADPHASE_PAIRS	= 4096;

struct hfBroadphaseEntry {
	HF_Ecs::Entity	entity;
	hfBounds		box;
};

struct hfBroadphasePair {
	int				a;
	int				b;
};

class hfBroadphase {
public:
	HF_INLINE hfBroadphase() : m_count( 0 ), m_pairCount( 0 ) {}

	HF_INLINE void Reset() { m_count = 0; m_pairCount = 0; }

	// Adds an entry; returns its id (index) for pair reporting.
	HF_INLINE int Add( const hfBroadphaseEntry &e ) {
		if ( m_count >= MAX_BROADPHASE_ENTRIES ) { return -1; }
		m_entries[m_count] = e;
		return m_count++;
	}

	// Sort by mins.x then sweep; returns the pair count.
	HF_INLINE int Build() {
		m_pairCount = 0;
		for ( int i = 0; i < m_count; i++ ) { m_order[i] = i; }
		// insertion sort (small scenes; swap for IntroSort on large ones)
		for ( int i = 1; i < m_count; i++ ) {
			int key = m_order[i];
			int j = i - 1;
			while ( j >= 0 && m_entries[m_order[j]].box[0].x > m_entries[key].box[0].x ) {
				m_order[j + 1] = m_order[j];
				j--;
			}
			m_order[j + 1] = key;
		}
		for ( int i = 0; i < m_count; i++ ) {
			const hfBounds &A = m_entries[m_order[i]].box;
			for ( int k = i + 1; k < m_count; k++ ) {
				const hfBounds &B = m_entries[m_order[k]].box;
				if ( B[0].x > A[1].x ) { break; }	// sorted by mins.x
				if ( m_pairCount >= MAX_BROADPHASE_PAIRS ) { return m_pairCount; }
				if ( A.IntersectsBounds( B ) ) {
					m_pairs[m_pairCount].a = m_order[i];
					m_pairs[m_pairCount].b = m_order[k];
					m_pairCount++;
				}
			}
		}
		return m_pairCount;
	}

	HF_INLINE int GetPairCount() const { return m_pairCount; }
	HF_INLINE const hfBroadphasePair &GetPair( int i ) const { return m_pairs[i]; }
	HF_INLINE void GetPair( int i, int &a, int &b ) const { a = m_pairs[i].a; b = m_pairs[i].b; }

private:
	hfBroadphaseEntry	m_entries[ MAX_BROADPHASE_ENTRIES ];
	hfBroadphasePair	m_pairs[ MAX_BROADPHASE_PAIRS ];
	int					m_order[ MAX_BROADPHASE_ENTRIES ];
	int					m_count;
	int					m_pairCount;
};

} // namespace HF_Physics

#endif