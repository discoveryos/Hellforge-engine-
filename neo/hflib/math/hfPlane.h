#ifndef __MATH_PLANE_H__
#define __MATH_PLANE_H__

/*
===============================================================================

	3D plane with equation: a * x + b * y + c * z + d = 0

===============================================================================
*/


class hfVec3;
class hfMat3;

#define	ON_EPSILON					0.1f
#define DEGENERATE_DIST_EPSILON		1e-4f

#define	SIDE_FRONT					0
#define	SIDE_BACK					1
#define	SIDE_ON						2
#define	SIDE_CROSS					3

// plane sides
#define PLANESIDE_FRONT				0
#define PLANESIDE_BACK				1
#define PLANESIDE_ON				2
#define PLANESIDE_CROSS				3

// plane types
#define PLANETYPE_X					0
#define PLANETYPE_Y					1
#define PLANETYPE_Z					2
#define PLANETYPE_NEGX				3
#define PLANETYPE_NEGY				4
#define PLANETYPE_NEGZ				5
#define PLANETYPE_TRUEAXIAL			6	// all types < 6 are true axial planes
#define PLANETYPE_ZEROX				6
#define PLANETYPE_ZEROY				7
#define PLANETYPE_ZEROZ				8
#define PLANETYPE_NONAXIAL			9

class hfPlane {
public:
					hfPlane();
					explicit hfPlane( float a, float b, float c, float d );
					explicit hfPlane( const hfVec3 &normal, const float dist );
					explicit hfPlane( const hfVec3 & v0, const hfVec3 & v1, const hfVec3 & v2, bool fixDegenerate = false );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	hfPlane			operator-() const;						// flips plane
	hfPlane &		operator=( const hfVec3 &v );			// sets normal and sets hfPlane::d to zero
	hfPlane			operator+( const hfPlane &p ) const;	// add plane equations
	hfPlane			operator-( const hfPlane &p ) const;	// subtract plane equations
	hfPlane			operator*( const float s ) const;		// scale plane
	hfPlane &		operator*=( const hfMat3 &m );			// Normal() *= m

	bool			Compare( const hfPlane &p ) const;						// exact compare, no epsilon
	bool			Compare( const hfPlane &p, const float epsilon ) const;	// compare with epsilon
	bool			Compare( const hfPlane &p, const float normalEps, const float distEps ) const;	// compare with epsilon
	bool			operator==(	const hfPlane &p ) const;					// exact compare, no epsilon
	bool			operator!=(	const hfPlane &p ) const;					// exact compare, no epsilon

	void			Zero();							// zero plane
	void			SetNormal( const hfVec3 &normal );		// sets the normal
	const hfVec3 &	Normal() const;					// reference to const normal
	hfVec3 &		Normal();							// reference to normal
	float			Normalize( bool fixDegenerate = true );	// only normalizes the plane normal, does not adjust d
	bool			FixDegenerateNormal();			// fix degenerate normal
	bool			FixDegeneracies( float distEpsilon );	// fix degenerate normal and dist
	float			Dist() const;						// returns: -d
	void			SetDist( const float dist );			// sets: d = -dist
	int				Type() const;						// returns plane type

	bool			FromPoints( const hfVec3 &p1, const hfVec3 &p2, const hfVec3 &p3, bool fixDegenerate = true );
	bool			FromVecs( const hfVec3 &dir1, const hfVec3 &dir2, const hfVec3 &p, bool fixDegenerate = true );
	void			FitThroughPoint( const hfVec3 &p );	// assumes normal is valid
	bool			HeightFit( const hfVec3 *points, const int numPoints );
	hfPlane			Translate( const hfVec3 &translation ) const;
	hfPlane &		TranslateSelf( const hfVec3 &translation );
	hfPlane			Rotate( const hfVec3 &origin, const hfMat3 &axis ) const;
	hfPlane &		RotateSelf( const hfVec3 &origin, const hfMat3 &axis );

	float			Distance( const hfVec3 &v ) const;
	int				Side( const hfVec3 &v, const float epsilon = 0.0f ) const;

	bool			LineIntersection( const hfVec3 &start, const hfVec3 &end ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const hfVec3 &start, const hfVec3 &dir, float &scale ) const;
	bool			PlaneIntersection( const hfPlane &plane, hfVec3 &start, hfVec3 &dir ) const;

	int				GetDimension() const;

	const hfVec4 &	ToVec4() const;
	hfVec4 &		ToVec4();
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	float			a;
	float			b;
	float			c;
	float			d;
};

extern hfPlane plane_origin;
#define plane_zero plane_origin

HF_INLINE hfPlane::hfPlane() {
}

HF_INLINE hfPlane::hfPlane( float a, float b, float c, float d ) {
	this->a = a;
	this->b = b;
	this->c = c;
	this->d = d;
}

HF_INLINE hfPlane::hfPlane( const hfVec3 &normal, const float dist ) {
	this->a = normal.x;
	this->b = normal.y;
	this->c = normal.z;
	this->d = -dist;
}

HF_INLINE hfPlane::hfPlane( const hfVec3 & v0, const hfVec3 & v1, const hfVec3 & v2, bool fixDegenerate ) {
	FromPoints( v0, v1, v2, fixDegenerate );
}

HF_INLINE float hfPlane::operator[]( int index ) const {
	return ( &a )[ index ];
}

HF_INLINE float& hfPlane::operator[]( int index ) {
	return ( &a )[ index ];
}

HF_INLINE hfPlane hfPlane::operator-() const {
	return hfPlane( -a, -b, -c, -d );
}

HF_INLINE hfPlane &hfPlane::operator=( const hfVec3 &v ) { 
	a = v.x;
	b = v.y;
	c = v.z;
	d = 0;
	return *this;
}

HF_INLINE hfPlane hfPlane::operator+( const hfPlane &p ) const {
	return hfPlane( a + p.a, b + p.b, c + p.c, d + p.d );
}

HF_INLINE hfPlane hfPlane::operator-( const hfPlane &p ) const {
	return hfPlane( a - p.a, b - p.b, c - p.c, d - p.d );
}

HF_INLINE hfPlane hfPlane::operator*( const float s ) const {
	return hfPlane( a * s, b * s, c * s, d * s );
}

HF_INLINE hfPlane &hfPlane::operator*=( const hfMat3 &m ) {
	Normal() *= m;
	return *this;
}

HF_INLINE bool hfPlane::Compare( const hfPlane &p ) const {
	return ( a == p.a && b == p.b && c == p.c && d == p.d );
}

HF_INLINE bool hfPlane::Compare( const hfPlane &p, const float epsilon ) const {
	if ( hfMath::Fabs( a - p.a ) > epsilon ) {
		return false;
	}
			
	if ( hfMath::Fabs( b - p.b ) > epsilon ) {
		return false;
	}

	if ( hfMath::Fabs( c - p.c ) > epsilon ) {
		return false;
	}

	if ( hfMath::Fabs( d - p.d ) > epsilon ) {
		return false;
	}

	return true;
}

HF_INLINE bool hfPlane::Compare( const hfPlane &p, const float normalEps, const float distEps ) const {
	if ( hfMath::Fabs( d - p.d ) > distEps ) {
		return false;
	}
	if ( !Normal().Compare( p.Normal(), normalEps ) ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfPlane::operator==( const hfPlane &p ) const {
	return Compare( p );
}

HF_INLINE bool hfPlane::operator!=( const hfPlane &p ) const {
	return !Compare( p );
}

HF_INLINE void hfPlane::Zero() {
	a = b = c = d = 0.0f;
}

HF_INLINE void hfPlane::SetNormal( const hfVec3 &normal ) {
	a = normal.x;
	b = normal.y;
	c = normal.z;
}

HF_INLINE const hfVec3 &hfPlane::Normal() const {
	return *reinterpret_cast<const hfVec3 *>(&a);
}

HF_INLINE hfVec3 &hfPlane::Normal() {
	return *reinterpret_cast<hfVec3 *>(&a);
}

HF_INLINE float hfPlane::Normalize( bool fixDegenerate ) {
	float length = reinterpret_cast<hfVec3 *>(&a)->Normalize();

	if ( fixDegenerate ) {
		FixDegenerateNormal();
	}
	return length;
}

HF_INLINE bool hfPlane::FixDegenerateNormal() {
	return Normal().FixDegenerateNormal();
}

HF_INLINE bool hfPlane::FixDegeneracies( float distEpsilon ) {
	bool fixedNormal = FixDegenerateNormal();
	// only fix dist if the normal was degenerate
	if ( fixedNormal ) {
		if ( hfMath::Fabs( d - hfMath::Rint( d ) ) < distEpsilon ) {
			d = hfMath::Rint( d );
		}
	}
	return fixedNormal;
}

HF_INLINE float hfPlane::Dist() const {
	return -d;
}

HF_INLINE void hfPlane::SetDist( const float dist ) {
	d = -dist;
}

HF_INLINE bool hfPlane::FromPoints( const hfVec3 &p1, const hfVec3 &p2, const hfVec3 &p3, bool fixDegenerate ) {
	Normal() = (p1 - p2).Cross( p3 - p2 );
	if ( Normalize( fixDegenerate ) == 0.0f ) {
		return false;
	}
	d = -( Normal() * p2 );
	return true;
}

HF_INLINE bool hfPlane::FromVecs( const hfVec3 &dir1, const hfVec3 &dir2, const hfVec3 &p, bool fixDegenerate ) {
	Normal() = dir1.Cross( dir2 );
	if ( Normalize( fixDegenerate ) == 0.0f ) {
		return false;
	}
	d = -( Normal() * p );
	return true;
}

HF_INLINE void hfPlane::FitThroughPoint( const hfVec3 &p ) {
	d = -( Normal() * p );
}

HF_INLINE hfPlane hfPlane::Translate( const hfVec3 &translation ) const {
	return hfPlane( a, b, c, d - translation * Normal() );
}

HF_INLINE hfPlane &hfPlane::TranslateSelf( const hfVec3 &translation ) {
	d -= translation * Normal();
	return *this;
}

HF_INLINE hfPlane hfPlane::Rotate( const hfVec3 &origin, const hfMat3 &axis ) const {
	hfPlane p;
	p.Normal() = Normal() * axis;
	p.d = d + origin * Normal() - origin * p.Normal();
	return p;
}

HF_INLINE hfPlane &hfPlane::RotateSelf( const hfVec3 &origin, const hfMat3 &axis ) {
	d += origin * Normal();
	Normal() *= axis;
	d -= origin * Normal();
	return *this;
}

HF_INLINE float hfPlane::Distance( const hfVec3 &v ) const {
	return a * v.x + b * v.y + c * v.z + d;
}

HF_INLINE int hfPlane::Side( const hfVec3 &v, const float epsilon ) const {
	float dist = Distance( v );
	if ( dist > epsilon ) {
		return PLANESIDE_FRONT;
	}
	else if ( dist < -epsilon ) {
		return PLANESIDE_BACK;
	}
	else {
		return PLANESIDE_ON;
	}
}

HF_INLINE bool hfPlane::LineIntersection( const hfVec3 &start, const hfVec3 &end ) const {
	float d1, d2, fraction;

	d1 = Normal() * start + d;
	d2 = Normal() * end + d;
	if ( d1 == d2 ) {
		return false;
	}
	if ( d1 > 0.0f && d2 > 0.0f ) {
		return false;
	}
	if ( d1 < 0.0f && d2 < 0.0f ) {
		return false;
	}
	fraction = ( d1 / ( d1 - d2 ) );
	return ( fraction >= 0.0f && fraction <= 1.0f );
}

HF_INLINE bool hfPlane::RayIntersection( const hfVec3 &start, const hfVec3 &dir, float &scale ) const {
	float d1, d2;

	d1 = Normal() * start + d;
	d2 = Normal() * dir;
	if ( d2 == 0.0f ) {
		return false;
	}
	scale = -( d1 / d2 );
	return true;
}

HF_INLINE int hfPlane::GetDimension() const {
	return 4;
}

HF_INLINE const hfVec4 &hfPlane::ToVec4() const {
	return *reinterpret_cast<const hfVec4 *>(&a);
}

HF_INLINE hfVec4 &hfPlane::ToVec4() {
	return *reinterpret_cast<hfVec4 *>(&a);
}

HF_INLINE const float *hfPlane::ToFloatPtr() const {
	return reinterpret_cast<const float *>(&a);
}

HF_INLINE float *hfPlane::ToFloatPtr() {
	return reinterpret_cast<float *>(&a);
}

#endif /* !__MATH_PLANE_H__ */

