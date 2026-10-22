#ifndef	HF_ANIMATION_SYSTEM_H
#define	HF_ANIMATION_SYSTEM_H
#include "../../hflib/hfEcs.h"
#include "hfSkeletonComponent.h"
#include "hfAnimationClip.h"

const int	HF_ANIM_MAX_SLOTS	= 4;

struct hfAnimSlot	{ int clip; int playing; float time; float weight; };

// Plays a set of clips into one skeleton. Every playing slot contributes its
// sampled pose with its own weight; the weighted poses are folded together
// sequentially (rotation via Slerp, translation via Lerp) so a single slot at
// weight 1 reproduces the raw clip, and a pair at 0.5/0.5 yields the midpoint.
// This is the blend-tree foundation the state machine drives.
struct hfAnimationSystem {
	HF_INLINE hfAnimationSystem() : clips( 0 ), clipCount( 0 ), skeleton( 0 ),
		eventCb( 0 ), eventUser( 0 ) {
		int i;
		for ( i = 0; i < HF_ANIM_MAX_SLOTS; i++ ) {
			slots[ i ].clip = -1; slots[ i ].playing = 0;
			slots[ i ].time = 0.0f; slots[ i ].weight = 1.0f;
		}
	}
	HF_INLINE void	SetClips( hfAnimationClip *c, int n ) { clips = c; clipCount = n; }
	HF_INLINE void	SetSkeleton( hfSkeletonComponent *s ) { skeleton = s; }
	HF_INLINE void	SetEventCallback( void (*cb)( const char *, float, void * ), void *user ) {
		eventCb = cb; eventUser = user;
	}
	HF_INLINE int	Play( int clip, int slot ) { return PlayWeighted( clip, slot, 1.0f ); }
	HF_INLINE int	PlayWeighted( int clip, int slot, float weight ) {
		if ( slot < 0 || slot >= HF_ANIM_MAX_SLOTS ) return -1;
		if ( clip < 0 || clip >= clipCount ) return -1;
		slots[ slot ].clip = clip;
		slots[ slot ].playing = 1;
		slots[ slot ].time = 0.0f;
		slots[ slot ].weight = ( weight < 0.0f ) ? 0.0f : weight;
		return slot;
	}
	HF_INLINE void	Stop( int slot ) {
		if ( slot < 0 || slot >= HF_ANIM_MAX_SLOTS ) return;
		slots[ slot ].playing = 0;
		slots[ slot ].weight = 0.0f;
	}
	HF_INLINE void	SetWeight( int slot, float weight ) {
		if ( slot < 0 || slot >= HF_ANIM_MAX_SLOTS ) return;
		slots[ slot ].weight = ( weight < 0.0f ) ? 0.0f : weight;
	}
	HF_INLINE float	GetWeight( int slot ) const {
		if ( slot < 0 || slot >= HF_ANIM_MAX_SLOTS ) return 0.0f;
		return slots[ slot ].weight;
	}
	HF_INLINE float	Playhead( int slot ) const {
		if ( slot < 0 || slot >= HF_ANIM_MAX_SLOTS ) return -1.0f;
		return slots[ slot ].time;
	}

	// Advance every playing slot by dt, firing any clip events the playhead
	// crossed, then fold the weighted samples into the skeleton. A full lap
	// boundary is handled by walking the (prev, next] span lap by lap so an
	// event is delivered exactly once per crossing even across a loop.
	HF_INLINE void	Advance( float dt ) {
		if ( !clips || !skeleton ) return;
		hfBonePose	acc[ HF_SKELETON_MAX_BONES ];
		hfBonePose	sample[ HF_SKELETON_MAX_BONES ];
		int		have[ HF_SKELETON_MAX_BONES ];
		float		accW = 0.0f;
		int		i, b, any = 0;

		for ( b = 0; b < HF_SKELETON_MAX_BONES; b++ ) {
			acc[ b ].rotation = hfQuat( 0, 0, 0, 1 );
			acc[ b ].position = hfVec3( 0, 0, 0 );
			have[ b ] = 0;
		}
		for ( i = 0; i < HF_ANIM_MAX_SLOTS; i++ ) {
			hfAnimSlot &s = slots[ i ];
			if ( !s.playing || s.clip < 0 || s.clip >= clipCount ) continue;
			const hfAnimationClip &clip = clips[ s.clip ];
			float prev = s.time;
			float next = s.time + dt;
			if ( eventCb ) DispatchEvents( clip, prev, next );
			s.time = next;
			if ( s.weight <= 0.0f ) continue;
			for ( b = 0; b < HF_SKELETON_MAX_BONES; b++ ) {
				sample[ b ].rotation = hfQuat( 0, 0, 0, 1 );
				sample[ b ].position = hfVec3( 0, 0, 0 );
			}
			clip.Sample( s.time, sample, HF_SKELETON_MAX_BONES );
			any = 1;
			float nw = accW + s.weight;
			float f = ( nw > 0.0f ) ? ( s.weight / nw ) : 0.0f;
			for ( b = 0; b < HF_SKELETON_MAX_BONES; b++ ) {
				if ( !have[ b ] ) {
					acc[ b ] = sample[ b ];
				} else {
					acc[ b ].rotation = Slerp( acc[ b ].rotation, sample[ b ].rotation, f );
					acc[ b ].position = acc[ b ].position +
						( sample[ b ].position - acc[ b ].position ) * f;
				}
				have[ b ] = 1;
			}
			accW = nw;
		}
		if ( !any ) return;
		for ( b = 0; b < HF_SKELETON_MAX_BONES; b++ )
			skeleton->localPose[ b ] = acc[ b ];
	}

	// Split (prev, next] into at most one partial segment per lap and emit the
	// events on each. A generous guard keeps a zero/negative duration from
	// spinning forever.
	HF_INLINE void	DispatchEvents( const hfAnimationClip &clip, float prev, float next ) {
		float		d = clip.duration > 0.0f ? clip.duration : 1.0f;
		float		from = clip.NormalizeTime( prev );
		float		remaining = next - prev;
		int		guard = 0;
		while ( remaining > 0.0f && guard++ < 64 ) {
			float segEnd = from + remaining;
			if ( segEnd <= d ) {
				clip.EventsInRange( from, segEnd, [ this ]( const char *n, float t ) { eventCb( n, t, eventUser ); } );
				remaining = 0.0f;
			} else {
				clip.EventsInRange( from, d, [ this ]( const char *n, float t ) { eventCb( n, t, eventUser ); } );
				remaining -= ( d - from );
				from = 0.0f;
			}
		}
	}

	hfAnimationClip		*clips;
	int			clipCount;
	hfSkeletonComponent	*skeleton;
	void			(*eventCb)( const char *, float, void * );
	void			*eventUser;
	hfAnimSlot		slots[ HF_ANIM_MAX_SLOTS ];
};
#endif
