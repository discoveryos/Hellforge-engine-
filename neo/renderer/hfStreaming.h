#ifndef __HF_STREAMING_H__
#define __HF_STREAMING_H__

#include "hfStr.h"

// Texture streaming. Two layers:
//
// 1. HF_Stream — a PURE mip-budget planner (no engine types) that the
//    headless harness validates: distance-derived target mip per texture plus
//    a global resident-byte budget.
// 2. hfStreamingManager — the engine-facing wrapper with cvars, invoked once
//    per backend frame. It does not touch hfImage yet (fully resident), which
//    is the documented wiring point: lower a far/LRU texture's resident mip
//    count when the frame is GPU-bound and r_streamTextures is on.

namespace HF_Stream {

static const unsigned int MAX_TEXTURES	= 4096;
static const unsigned int MAX_MIPS		= 12;
static const unsigned int NO_BUDGET		= 0xFFFFFFFFu;

struct TextureEntry {
	unsigned int	id;				// opaque caller id
	unsigned int	lastUsedFrame;	// LRU-sort key, higher = more recent
	float			distance;		// viewer distance (world units), >= 0
	unsigned short	width;
	unsigned short	height;
	unsigned short	mipCount;		// how many mips the file has
	unsigned short	currentBaseMip;	// in/out: starts at 0
	unsigned char	formatBpp;		// bytes per texel
	unsigned char	reserved;
};

// Bytes consumed by a single mip level.
static inline unsigned int mipBytes( const TextureEntry &t, unsigned short mip ) {
	unsigned int w = t.width >> mip; if ( w < 1 ) { w = 1; }
	unsigned int h = t.height >> mip; if ( h < 1 ) { h = 1; }
	return w * h * t.formatBpp;
}

// Total resident bytes for an entry given a base mip.
static inline unsigned int residentBytes( const TextureEntry &t, unsigned short baseMip ) {
	unsigned int total = 0;
	for ( unsigned short m = baseMip; m < t.mipCount && m < MAX_MIPS; m++ ) {
		total += mipBytes( t, m );
	}
	return total;
}

// Distance-derived target base mip. bias < 0 keeps more resolution.
static inline unsigned short targetBaseMip( float distance, unsigned short mipCount, float bias ) {
	if ( mipCount <= 1 ) { return 0; }
	float d = distance;
	if ( d < 1.0f ) { d = 1.0f; }
	float m = 0.5f + ( float )log2f( ( float )( d * 0.02f ) ) + bias;
	if ( m < 0.0f ) { m = 0.0f; }
	if ( m > mipCount - 1 ) { m = mipCount - 1; }
	return ( unsigned short )m;
}

// Rebalance residency so the total stays within budgetBytes. Returns the
// number of textures degraded below their distance-derived target; the final
// resident byte count is stored in residentOut.
static inline unsigned int planResidency( TextureEntry *entries, int count,
											unsigned int budgetBytes, unsigned int &residentOut ) {
	unsigned int resident = 0;
	// phase 1: apply per-texture distance targets
	for ( int i = 0; i < count; i++ ) {
		TextureEntry &t = entries[i];
		unsigned short want = targetBaseMip( t.distance, t.mipCount, 0.0f );
		if ( t.currentBaseMip < want ) { t.currentBaseMip = want; }
		resident += residentBytes( t, t.currentBaseMip );
	}
	// phase 2: over budget -> drop the farthest / least-recently-used entry
	// one mip at a time
	int guard = 0;
	while ( resident > budgetBytes && guard < ( int )count * ( int )MAX_MIPS ) {
		int worst = -1;
		double worstScore = -1.0;
		for ( int i = 0; i < count; i++ ) {
			TextureEntry &t = entries[i];
			if ( t.currentBaseMip >= t.mipCount - 1 ) { continue; }
			double score = ( double )t.distance + ( double )( t.lastUsedFrame ) * 0.001;
			if ( score > worstScore ) { worstScore = score; worst = i; }
		}
		if ( worst < 0 ) { break; }
		TextureEntry &t = entries[worst];
		resident -= residentBytes( t, t.currentBaseMip );
		t.currentBaseMip++;
		resident += residentBytes( t, t.currentBaseMip );
		guard++;
	}
	unsigned int degraded = 0;
	for ( int i = 0; i < count; i++ ) {
		const TextureEntry &t = entries[i];
		unsigned short want = targetBaseMip( t.distance, t.mipCount, 0.0f );
		if ( t.currentBaseMip > want ) { degraded++; }
	}
	residentOut = resident;
	return degraded;
}

} // namespace HF_Stream

// -------------------------------------------------------------------------
// engine-facing manager
// -------------------------------------------------------------------------
class hfStreamingManager {
public:
					hfStreamingManager();

	// Called once per backend frame. Currently tracks budget stats only; the
	// hfImage hook that would drop mips lives behind the same cvars.
	void			Frame();

	bool			IsEnabled() const;
	unsigned int	GetBudgetBytes() const;
	int				GetLastDegraded() const { return m_lastDegraded; }
	const char *	GetStatus( hfStr &out ) const;

protected:
	bool			m_reported;
	unsigned int	m_lastDegraded;
};

extern hfStreamingManager streamingManager;

#endif