#ifndef __MATH_QUAT_H__
#define __MATH_QUAT_H__

/*
===============================================================================

	Quaternion

===============================================================================
*/

#include "../containers/hfArray.h"	// for hfTupleSize

class hfVec3;
class hfAngles;
class hfRotation;
class hfMat3;
class hfMat4;
class hfCQuat;

class hfQuat {
public:
	float			x;
	float			y;
	float			z;
	float			w;

					hfQuat();
					hfQuat( float x, float y, float z, float w );

	void 			Set( float x, float y, float z, float w );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	hfQuat			operator-() const;
	hfQuat &		operator=( const hfQuat &a );
	hfQuat			operator+( const hfQuat &a ) const;
	hfQuat &		operator+=( const hfQuat &a );
	hfQuat			operator-( const hfQuat &a ) const;
	hfQuat &		operator-=( const hfQuat &a );
	hfQuat			operator*( const hfQuat &a ) const;
	hfVec3			operator*( const hfVec3 &a ) const;
	hfQuat			operator*( float a ) const;
	hfQuat &		operator*=( const hfQuat &a );
	hfQuat &		operator*=( float a );

	friend hfQuat	operator*( const float a, const hfQuat &b );
	friend hfVec3	operator*( const hfVec3 &a, const hfQuat &b );

	bool			Compare( const hfQuat &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfQuat &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfQuat &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const hfQuat &a ) const;					// exact compare, no epsilon

	hfQuat			Inverse() const;
	float			Length() const;
	hfQuat &		Normalize();

	float			CalcW() const;
	int				GetDimension() const;

	hfAngles		ToAngles() const;
	hfRotation		ToRotation() const;
	hfMat3			ToMat3() const;
	hfMat4			ToMat4() const;
	hfCQuat			ToCQuat() const;
	hfVec3			ToAngularVelocity() const;
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

	hfQuat &		Slerp( const hfQuat &from, const hfQuat &to, float t );
	hfQuat &		Lerp( const hfQuat &from, const hfQuat &to, const float t );
};

// A non-member slerp function allows constructing a const hfQuat object with the result of a slerp,
// but without having to explicity create a temporary hfQuat object.
hfQuat Slerp( const hfQuat & from, const hfQuat & to, const float t );

HF_INLINE hfQuat::hfQuat() {
}

HF_INLINE hfQuat::hfQuat( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

HF_INLINE float hfQuat::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

HF_INLINE float& hfQuat::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

HF_INLINE hfQuat hfQuat::operator-() const {
	return hfQuat( -x, -y, -z, -w );
}

HF_INLINE hfQuat &hfQuat::operator=( const hfQuat &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	w = a.w;

	return *this;
}

HF_INLINE hfQuat hfQuat::operator+( const hfQuat &a ) const {
	return hfQuat( x + a.x, y + a.y, z + a.z, w + a.w );
}

HF_INLINE hfQuat& hfQuat::operator+=( const hfQuat &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

HF_INLINE hfQuat hfQuat::operator-( const hfQuat &a ) const {
	return hfQuat( x - a.x, y - a.y, z - a.z, w - a.w );
}

HF_INLINE hfQuat& hfQuat::operator-=( const hfQuat &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

HF_INLINE hfQuat hfQuat::operator*( const hfQuat &a ) const {
	return hfQuat(	w*a.x + x*a.w + y*a.z - z*a.y,
					w*a.y + y*a.w + z*a.x - x*a.z,
					w*a.z + z*a.w + x*a.y - y*a.x,
					w*a.w - x*a.x - y*a.y - z*a.z );
}

HF_INLINE hfVec3 hfQuat::operator*( const hfVec3 &a ) const {
#if 0
	// it's faster to do the conversion to a 3x3 matrix and multiply the vector by this 3x3 matrix
	return ( ToMat3() * a );
#else
	// result = this->Inverse() * hfQuat( a.x, a.y, a.z, 0.0f ) * (*this)
	float xxzz = x*x - z*z;
	float wwyy = w*w - y*y;

	float xw2 = x*w*2.0f;
	float xy2 = x*y*2.0f;
	float xz2 = x*z*2.0f;
	float yw2 = y*w*2.0f;
	float yz2 = y*z*2.0f;
	float zw2 = z*w*2.0f;

	return hfVec3(
		(xxzz + wwyy)*a.x		+ (xy2 + zw2)*a.y		+ (xz2 - yw2)*a.z,
		(xy2 - zw2)*a.x			+ (y*y+w*w-x*x-z*z)*a.y	+ (yz2 + xw2)*a.z,
		(xz2 + yw2)*a.x			+ (yz2 - xw2)*a.y		+ (wwyy - xxzz)*a.z
	);
#endif
}

HF_INLINE hfQuat hfQuat::operator*( float a ) const {
	return hfQuat( x * a, y * a, z * a, w * a );
}

HF_INLINE hfQuat operator*( const float a, const hfQuat &b ) {
	return b * a;
}

HF_INLINE hfVec3 operator*( const hfVec3 &a, const hfQuat &b ) {
	return b * a;
}

HF_INLINE hfQuat& hfQuat::operator*=( const hfQuat &a ) {
	*this = *this * a;

	return *this;
}

HF_INLINE hfQuat& hfQuat::operator*=( float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

HF_INLINE bool hfQuat::Compare( const hfQuat &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && ( w == a.w ) );
}

HF_INLINE bool hfQuat::Compare( const hfQuat &a, const float epsilon ) const {
	if ( hfMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( w - a.w ) > epsilon ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfQuat::operator==( const hfQuat &a ) const {
	return Compare( a );
}

HF_INLINE bool hfQuat::operator!=( const hfQuat &a ) const {
	return !Compare( a );
}

HF_INLINE void hfQuat::Set( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

HF_INLINE hfQuat hfQuat::Inverse() const {
	return hfQuat( -x, -y, -z, w );
}

HF_INLINE float hfQuat::Length() const {
	float len;

	len = x * x + y * y + z * z + w * w;
	return hfMath::Sqrt( len );
}

HF_INLINE hfQuat& hfQuat::Normalize() {
	float len;
	float ilength;

	len = this->Length();
	if ( len ) {
		ilength = 1 / len;
		x *= ilength;
		y *= ilength;
		z *= ilength;
		w *= ilength;
	}
	return *this;
}

HF_INLINE float hfQuat::CalcW() const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) );
}

HF_INLINE int hfQuat::GetDimension() const {
	return 4;
}

HF_INLINE const float *hfQuat::ToFloatPtr() const {
	return &x;
}

HF_INLINE float *hfQuat::ToFloatPtr() {
	return &x;
}

/*
===============================================================================

	Specialization to get size of an hfQuat generically.

===============================================================================
*/
template<>
struct hfTupleSize< hfQuat > {
	enum { value = 4 };
};

/*
===============================================================================

	Compressed quaternion

===============================================================================
*/

class hfCQuat {
public:
	float			x;
	float			y;
	float			z;

					hfCQuat();
					hfCQuat( float x, float y, float z );

	void 			Set( float x, float y, float z );

	float			operator[]( int index ) const;
	float &			operator[]( int index );

	bool			Compare( const hfCQuat &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfCQuat &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfCQuat &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const hfCQuat &a ) const;					// exact compare, no epsilon

	int				GetDimension() const;

	hfAngles		ToAngles() const;
	hfRotation		ToRotation() const;
	hfMat3			ToMat3() const;
	hfMat4			ToMat4() const;
	hfQuat			ToQuat() const;
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;
};

HF_INLINE hfCQuat::hfCQuat() {
}

HF_INLINE hfCQuat::hfCQuat( float x, float y, float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

HF_INLINE void hfCQuat::Set( float x, float y, float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

HF_INLINE float hfCQuat::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[ index ];
}

HF_INLINE float& hfCQuat::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[ index ];
}

HF_INLINE bool hfCQuat::Compare( const hfCQuat &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

HF_INLINE bool hfCQuat::Compare( const hfCQuat &a, const float epsilon ) const {
	if ( hfMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfCQuat::operator==( const hfCQuat &a ) const {
	return Compare( a );
}

HF_INLINE bool hfCQuat::operator!=( const hfCQuat &a ) const {
	return !Compare( a );
}

HF_INLINE int hfCQuat::GetDimension() const {
	return 3;
}

HF_INLINE hfQuat hfCQuat::ToQuat() const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return hfQuat( x, y, z, sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) ) );
}

HF_INLINE const float *hfCQuat::ToFloatPtr() const {
	return &x;
}

HF_INLINE float *hfCQuat::ToFloatPtr() {
	return &x;
}

/*
===============================================================================

	Specialization to get size of an hfCQuat generically.

===============================================================================
*/
template<>
struct hfTupleSize< hfCQuat > {
	enum { value = 3 };
};

#endif /* !__MATH_QUAT_H__ */

