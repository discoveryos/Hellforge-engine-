#ifndef __MATH_PLUECKER_H__
#define __MATH_PLUECKER_H__

/*
===============================================================================

	Pluecker coordinate

===============================================================================
*/

class hfPluecker {
public:	
					hfPluecker();
					explicit hfPluecker( const float *a );
					explicit hfPluecker( const hfVec3 &start, const hfVec3 &end );
					explicit hfPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	hfPluecker		operator-() const;											// flips the direction
	hfPluecker		operator*( const float a ) const;
	hfPluecker		operator/( const float a ) const;
	float			operator*( const hfPluecker &a ) const;						// permuted inner product
	hfPluecker		operator-( const hfPluecker &a ) const;
	hfPluecker		operator+( const hfPluecker &a ) const;
	hfPluecker &	operator*=( const float a );
	hfPluecker &	operator/=( const float a );
	hfPluecker &	operator+=( const hfPluecker &a );
	hfPluecker &	operator-=( const hfPluecker &a );

	bool			Compare( const hfPluecker &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfPluecker &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfPluecker &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const hfPluecker &a ) const;					// exact compare, no epsilon

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero();

	void			FromLine( const hfVec3 &start, const hfVec3 &end );			// pluecker from line
	void			FromRay( const hfVec3 &start, const hfVec3 &dir );			// pluecker from ray
	bool			FromPlanes( const hfPlane &p1, const hfPlane &p2 );			// pluecker from intersection of planes
	bool			ToLine( hfVec3 &start, hfVec3 &end ) const;					// pluecker to line
	bool			ToRay( hfVec3 &start, hfVec3 &dir ) const;					// pluecker to ray
	void			ToDir( hfVec3 &dir ) const;									// pluecker to direction
	float			PermutedInnerProduct( const hfPluecker &a ) const;			// pluecker permuted inner product
	float			Distance3DSqr( const hfPluecker &a ) const;					// pluecker line distance

	float			Length() const;										// pluecker length
	float			LengthSqr() const;									// pluecker squared length
	hfPluecker		Normalize() const;									// pluecker normalize
	float			NormalizeSelf();										// pluecker normalize

	int				GetDimension() const;

	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern hfPluecker pluecker_origin;
#define pluecker_zero pluecker_origin

HF_INLINE hfPluecker::hfPluecker() {
}

HF_INLINE hfPluecker::hfPluecker( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

HF_INLINE hfPluecker::hfPluecker( const hfVec3 &start, const hfVec3 &end ) {
	FromLine( start, end );
}

HF_INLINE hfPluecker::hfPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

HF_INLINE hfPluecker hfPluecker::operator-() const {
	return hfPluecker( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

HF_INLINE float hfPluecker::operator[]( const int index ) const {
	return p[index];
}

HF_INLINE float &hfPluecker::operator[]( const int index ) {
	return p[index];
}

HF_INLINE hfPluecker hfPluecker::operator*( const float a ) const {
	return hfPluecker( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

HF_INLINE float hfPluecker::operator*( const hfPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

HF_INLINE hfPluecker hfPluecker::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return hfPluecker( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

HF_INLINE hfPluecker hfPluecker::operator+( const hfPluecker &a ) const {
	return hfPluecker( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

HF_INLINE hfPluecker hfPluecker::operator-( const hfPluecker &a ) const {
	return hfPluecker( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

HF_INLINE hfPluecker &hfPluecker::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

HF_INLINE hfPluecker &hfPluecker::operator/=( const float a ) {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	p[0] *= inva;
	p[1] *= inva;
	p[2] *= inva;
	p[3] *= inva;
	p[4] *= inva;
	p[5] *= inva;
	return *this;
}

HF_INLINE hfPluecker &hfPluecker::operator+=( const hfPluecker &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

HF_INLINE hfPluecker &hfPluecker::operator-=( const hfPluecker &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

HF_INLINE bool hfPluecker::Compare( const hfPluecker &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

HF_INLINE bool hfPluecker::Compare( const hfPluecker &a, const float epsilon ) const {
	if ( hfMath::Fabs( p[0] - a[0] ) > epsilon ) {
		return false;
	}
			
	if ( hfMath::Fabs( p[1] - a[1] ) > epsilon ) {
		return false;
	}

	if ( hfMath::Fabs( p[2] - a[2] ) > epsilon ) {
		return false;
	}

	if ( hfMath::Fabs( p[3] - a[3] ) > epsilon ) {
		return false;
	}

	if ( hfMath::Fabs( p[4] - a[4] ) > epsilon ) {
		return false;
	}

	if ( hfMath::Fabs( p[5] - a[5] ) > epsilon ) {
		return false;
	}

	return true;
}

HF_INLINE bool hfPluecker::operator==( const hfPluecker &a ) const {
	return Compare( a );
}

HF_INLINE bool hfPluecker::operator!=( const hfPluecker &a ) const {
	return !Compare( a );
}

HF_INLINE void hfPluecker::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

HF_INLINE void hfPluecker::Zero() {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

HF_INLINE void hfPluecker::FromLine( const hfVec3 &start, const hfVec3 &end ) {
	p[0] = start[0] * end[1] - end[0] * start[1];
	p[1] = start[0] * end[2] - end[0] * start[2];
	p[2] = start[0] - end[0];
	p[3] = start[1] * end[2] - end[1] * start[2];
	p[4] = start[2] - end[2];
	p[5] = end[1] - start[1];
}

HF_INLINE void hfPluecker::FromRay( const hfVec3 &start, const hfVec3 &dir ) {
	p[0] = start[0] * dir[1] - dir[0] * start[1];
	p[1] = start[0] * dir[2] - dir[0] * start[2];
	p[2] = -dir[0];
	p[3] = start[1] * dir[2] - dir[1] * start[2];
	p[4] = -dir[2];
	p[5] = dir[1];
}

HF_INLINE bool hfPluecker::ToLine( hfVec3 &start, hfVec3 &end ) const {
	hfVec3 dir1, dir2;
	float d;

	dir1[0] = p[3];
	dir1[1] = -p[1];
	dir1[2] = p[0];

	dir2[0] = -p[2];
	dir2[1] = p[5];
	dir2[2] = -p[4];

	d = dir2 * dir2;
	if ( d == 0.0f ) {
		return false; // pluecker coordinate does not represent a line
	}

	start = dir2.Cross(dir1) * (1.0f / d);
	end = start + dir2;
	return true;
}

HF_INLINE bool hfPluecker::ToRay( hfVec3 &start, hfVec3 &dir ) const {
	hfVec3 dir1;
	float d;

	dir1[0] = p[3];
	dir1[1] = -p[1];
	dir1[2] = p[0];

	dir[0] = -p[2];
	dir[1] = p[5];
	dir[2] = -p[4];

	d = dir * dir;
	if ( d == 0.0f ) {
		return false; // pluecker coordinate does not represent a line
	}

	start = dir.Cross(dir1) * (1.0f / d);
	return true;
}

HF_INLINE void hfPluecker::ToDir( hfVec3 &dir ) const {
	dir[0] = -p[2];
	dir[1] = p[5];
	dir[2] = -p[4];
}

HF_INLINE float hfPluecker::PermutedInnerProduct( const hfPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

HF_INLINE float hfPluecker::Length() const {
	return ( float )hfMath::Sqrt( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

HF_INLINE float hfPluecker::LengthSqr() const {
	return ( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

HF_INLINE float hfPluecker::NormalizeSelf() {
	float l, d;

	l = LengthSqr();
	if ( l == 0.0f ) {
		return l; // pluecker coordinate does not represent a line
	}
	d = hfMath::InvSqrt( l );
	p[0] *= d;
	p[1] *= d;
	p[2] *= d;
	p[3] *= d;
	p[4] *= d;
	p[5] *= d;
	return d * l;
}

HF_INLINE hfPluecker hfPluecker::Normalize() const {
	float d;

	d = LengthSqr();
	if ( d == 0.0f ) {
		return *this; // pluecker coordinate does not represent a line
	}
	d = hfMath::InvSqrt( d );
	return hfPluecker( p[0]*d, p[1]*d, p[2]*d, p[3]*d, p[4]*d, p[5]*d );
}

HF_INLINE int hfPluecker::GetDimension() const {
	return 6;
}

HF_INLINE const float *hfPluecker::ToFloatPtr() const {
	return p;
}

HF_INLINE float *hfPluecker::ToFloatPtr() {
	return p;
}

#endif /* !__MATH_PLUECKER_H__ */

