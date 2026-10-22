#ifndef	HF_ANIMATION_CLIP_H
#define	HF_ANIMATION_CLIP_H
#include "../../hflib/hfEcs.h"
#include "hfSkeletonComponent.h"

const int	HF_ANIM_MAX_KEYS	= 64;
const int	HF_ANIM_MAX_TRACKS	= 4;
const int	HF_ANIM_MAX_BONES	= 8;

struct hfAnimRKey	{ float t; hfQuat value; };
struct hfAnimTKey	{ float t; hfVec3 value; };

struct hfAnimTrack {
	HF_INLINE hfAnimTrack() : bone( 0 ), rCount( 0 ), tCount( 0 ) {}
	HF_INLINE int	AddR( float t, const hfQuat &v ) {
		if ( rCount >= HF_ANIM_MAX_KEYS ) return -1;
		hfAnimRKey &k = rkeys[ rCount++ ];
		k.t = t; k.value = v;
		return rCount - 1;
	}
	HF_INLINE int	AddT( float t, const hfVec3 &v ) {
		if ( tCount >= HF_ANIM_MAX_KEYS ) return -1;
		hfAnimTKey &k = tkeys[ tCount++ ];
		k.t = t; k.value = v;
		return tCount - 1;
	}
	int		bone;
	int		rCount;
	int		tCount;
	hfAnimRKey	rkeys[ HF_ANIM_MAX_KEYS ];
	hfAnimTKey	tkeys[ HF_ANIM_MAX_KEYS ];
};

struct hfAnimEvent	{ float t; const char *name; };

struct hfAnimationClip {
	HF_INLINE hfAnimationClip() : duration( 1.0f ), trackCount( 0 ), eventCount( 0 ) {}
	HF_INLINE int	AddTrack( int bone ) {
		if ( trackCount >= HF_ANIM_MAX_TRACKS ) return -1;
		hfAnimTrack &t = tracks[ trackCount++ ];
		t.bone = bone;
		return trackCount - 1;
	}
	HF_INLINE int	AddEvent( float t, const char *n ) {
		if ( eventCount >= HF_ANIM_MAX_TRACKS ) return -1;
		hfAnimEvent &e = events[ eventCount++ ];
		e.t = t; e.name = n;
		return eventCount - 1;
	}
	HF_INLINE int	FindEvent( const char *n ) const {
		int i;
		for ( i = 0; i < eventCount; i++ )
			if ( hfStr::Cmp( events[ i ].name, n ) == 0 ) return i;
		return -1;
	}
	HF_INLINE float	EndTime() const { return duration; }
	HF_INLINE float	NormalizeTime( float t ) const {
		float d = duration > 0.0f ? duration : 1.0f;
		float x = (float)fmod( (double)t, (double)d );
		if ( x < 0.0f ) x += d;
		return x;
	}
	HF_INLINE void	Sample( float t, hfBonePose *pose, int maxBones ) const {
		float	nt = NormalizeTime( t );
		int	i;
		for ( i = 0; i < trackCount; i++ ) {
			const hfAnimTrack &tr = tracks[ i ];
			int	b = tr.bone;
			if ( b < 0 || b >= maxBones ) continue;
			hfBonePose &p = pose[ b ];
			if ( tr.rCount > 0 ) {
				int	ri = 0;
				while ( ri + 1 < tr.rCount && tr.rkeys[ ri + 1 ].t <= nt ) ri++;
				if ( ri + 1 < tr.rCount ) {
					const hfAnimRKey &a = tr.rkeys[ ri ];
					const hfAnimRKey &c = tr.rkeys[ ri + 1 ];
					float	f = ( c.t > a.t ) ? ( nt - a.t ) / ( c.t - a.t ) : 0.0f;
					p.rotation = Slerp( a.value, c.value, f );
				} else p.rotation = tr.rkeys[ ri ].value;
			}
			if ( tr.tCount > 0 ) {
				int	ti = 0;
				while ( ti + 1 < tr.tCount && tr.tkeys[ ti + 1 ].t <= nt ) ti++;
				if ( ti + 1 < tr.tCount ) {
					const hfAnimTKey &a = tr.tkeys[ ti ];
					const hfAnimTKey &c = tr.tkeys[ ti + 1 ];
					float	f = ( c.t > a.t ) ? ( nt - a.t ) / ( c.t - a.t ) : 0.0f;
					p.position = a.value + ( c.value - a.value ) * f;
				} else p.position = tr.tkeys[ ti ].value;
			}
		}
	}
	// Fire every event whose time lies in (lo, hi]. The caller is responsible
	// for splitting a wrapped interval into in-range segments, so this stays a
	// plain range scan with no wrap arithmetic of its own.
	template< typename Fn >
	HF_INLINE int	EventsInRange( float lo, float hi, const Fn &cb ) const {
		int	i, fired = 0;
		for ( i = 0; i < eventCount; i++ ) {
			if ( events[ i ].t > lo && events[ i ].t <= hi ) {
				cb( events[ i ].name, events[ i ].t );
				fired++;
			}
		}
		return fired;
	}

	float		duration;
	int		trackCount;
	int		eventCount;
	hfAnimTrack	tracks[ HF_ANIM_MAX_TRACKS ];
	hfAnimEvent	events[ HF_ANIM_MAX_TRACKS ];
};
#endif
