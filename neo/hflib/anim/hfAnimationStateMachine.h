#ifndef	HF_ANIMATION_STATE_MACHINE_H
#define	HF_ANIMATION_STATE_MACHINE_H
#include "../../hflib/hfEcs.h"

const int	HF_ANIM_SM_MAX_STATES	= 8;
const int	HF_ANIM_SM_MAX_PARAMS	= 8;
const int	HF_ANIM_SM_MAX_TRANS	= 16;
const int	HF_ANIM_SM_NAME		= 24;

enum hfAnimCond {
	HF_ANIM_COND_BOOL = 0,		// param is true
	HF_ANIM_COND_BOOL_FALSE,	// param is false
	HF_ANIM_COND_FLOAT_GT,		// param > threshold
	HF_ANIM_COND_FLOAT_LT,		// param < threshold
	HF_ANIM_COND_TRIGGER		// param set, then consumed by taking it
};

struct hfAnimSmState		{ char name[ HF_ANIM_SM_NAME ]; int clip; };
struct hfAnimSmParam		{ char name[ HF_ANIM_SM_NAME ]; int type; float fval; int bval; };
struct hfAnimSmTransition	{ int from; int to; char param[ HF_ANIM_SM_NAME ]; int cond; float threshold; };

// A small play-state graph. Each state names a clip; transitions are guarded by
// named parameters. Update() takes the first transition leaving the current
// state whose guard holds (from == -1 means "from any state"), consumes a
// trigger if that is what fired, and returns the new state's clip index so the
// caller can hand it to hfAnimationSystem::Play. It never touches the blender
// itself — that separation is what keeps it testable in isolation.
struct hfAnimationStateMachine {
	enum { PARAM_BOOL = 0, PARAM_FLOAT, PARAM_TRIGGER };

	HF_INLINE hfAnimationStateMachine() : stateCount( 0 ), paramCount( 0 ),
		transCount( 0 ), current( -1 ), lastTransition( -1 ) {}

	HF_INLINE static void	CopyName( char *dst, const char *src ) {
		int i = 0;
		if ( !src ) { dst[ 0 ] = 0; return; }
		while ( src[ i ] && i < HF_ANIM_SM_NAME - 1 ) { dst[ i ] = src[ i ]; i++; }
		dst[ i ] = 0;
	}

	HF_INLINE int	FindParam( const char *n ) const {
		int i;
		for ( i = 0; i < paramCount; i++ )
			if ( hfStr::Cmp( params[ i ].name, n ) == 0 ) return i;
		return -1;
	}
	HF_INLINE int	AddBool( const char *n, bool v ) {
		int p = AddParam( n, PARAM_BOOL );
		if ( p >= 0 ) params[ p ].bval = v ? 1 : 0;
		return p;
	}
	HF_INLINE int	AddFloat( const char *n, float v ) {
		int p = AddParam( n, PARAM_FLOAT );
		if ( p >= 0 ) params[ p ].fval = v;
		return p;
	}
	HF_INLINE int	AddTrigger( const char *n ) {
		int p = AddParam( n, PARAM_TRIGGER );
		if ( p >= 0 ) params[ p ].bval = 0;
		return p;
	}
	HF_INLINE bool	SetBool( const char *n, bool v ) {
		int p = FindParam( n );
		if ( p < 0 ) return false;
		params[ p ].bval = v ? 1 : 0;
		return true;
	}
	HF_INLINE bool	SetFloat( const char *n, float v ) {
		int p = FindParam( n );
		if ( p < 0 ) return false;
		params[ p ].fval = v;
		return true;
	}
	HF_INLINE bool	SetTrigger( const char *n ) {
		int p = FindParam( n );
		if ( p < 0 ) return false;
		params[ p ].bval = 1;
		return true;
	}
	HF_INLINE bool	GetBool( const char *n ) const {
		int p = FindParam( n );
		return p >= 0 ? ( params[ p ].bval != 0 ) : false;
	}
	HF_INLINE float	GetFloat( const char *n ) const {
		int p = FindParam( n );
		return p >= 0 ? params[ p ].fval : 0.0f;
	}

	HF_INLINE int	FindState( const char *n ) const {
		int i;
		for ( i = 0; i < stateCount; i++ )
			if ( hfStr::Cmp( states[ i ].name, n ) == 0 ) return i;
		return -1;
	}
	HF_INLINE int	AddState( const char *n, int clip ) {
		if ( stateCount >= HF_ANIM_SM_MAX_STATES ) return -1;
		hfAnimSmState &s = states[ stateCount ];
		CopyName( s.name, n );
		s.clip = clip;
		if ( current < 0 ) current = stateCount;		// first added state is default
		return stateCount++;
	}
	HF_INLINE int	AddTransition( int from, int to, const char *param, int cond, float threshold ) {
		if ( to < 0 || to >= stateCount ) return -1;
		if ( transCount >= HF_ANIM_SM_MAX_TRANS ) return -1;
		hfAnimSmTransition &t = transitions[ transCount ];
		t.from = from; t.to = to; t.cond = cond; t.threshold = threshold;
		CopyName( t.param, param );
		return transCount++;
	}
	HF_INLINE void	SetState( int s ) { if ( s >= 0 && s < stateCount ) current = s; }
	HF_INLINE int	GetState() const { return current; }
	HF_INLINE int	GetStateClip() const {
		return ( current >= 0 && current < stateCount ) ? states[ current ].clip : -1;
	}
	HF_INLINE const char *GetStateName() const {
		return ( current >= 0 && current < stateCount ) ? states[ current ].name : "";
	}
	HF_INLINE int	GetLastTransition() const { return lastTransition; }

	HF_INLINE int	Update() {
		int i;
		lastTransition = -1;
		for ( i = 0; i < transCount; i++ ) {
			hfAnimSmTransition &tr = transitions[ i ];
			if ( tr.from >= 0 && tr.from != current ) continue;
			int p = FindParam( tr.param );
			if ( p < 0 ) continue;
			hfAnimSmParam &par = params[ p ];
			bool hit = false;
			switch ( tr.cond ) {
			case HF_ANIM_COND_BOOL:		hit = ( par.bval != 0 ); break;
			case HF_ANIM_COND_BOOL_FALSE:	hit = ( par.bval == 0 ); break;
			case HF_ANIM_COND_FLOAT_GT:	hit = ( par.fval > tr.threshold ); break;
			case HF_ANIM_COND_FLOAT_LT:	hit = ( par.fval < tr.threshold ); break;
			case HF_ANIM_COND_TRIGGER:	hit = ( par.bval != 0 ); break;
			}
			if ( !hit ) continue;
			if ( tr.cond == HF_ANIM_COND_TRIGGER ) par.bval = 0;
			lastTransition = i;
			current = tr.to;
			return GetStateClip();
		}
		return -1;
	}

	int	stateCount;
	int	paramCount;
	int	transCount;
	int	current;
	int	lastTransition;
	hfAnimSmState		states[ HF_ANIM_SM_MAX_STATES ];
	hfAnimSmParam		params[ HF_ANIM_SM_MAX_PARAMS ];
	hfAnimSmTransition	transitions[ HF_ANIM_SM_MAX_TRANS ];

private:
	HF_INLINE int	AddParam( const char *n, int type ) {
		if ( paramCount >= HF_ANIM_SM_MAX_PARAMS ) return -1;
		if ( FindParam( n ) >= 0 ) return -1;
		hfAnimSmParam &p = params[ paramCount ];
		CopyName( p.name, n );
		p.type = type;
		p.fval = 0.0f;
		p.bval = 0;
		return paramCount++;
	}
};
#endif
