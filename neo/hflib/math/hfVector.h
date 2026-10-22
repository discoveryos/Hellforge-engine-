#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

/*
===============================================================================

  Vector classes

===============================================================================
*/

#include "../containers/hfArray.h" // for hfTupleSize

#define VECTOR_EPSILON		0.001f

class hfAngles;
class hfPolar3;
class hfMat3;

//===============================================================
//
//	hfVec2 - 2D vector
//
//===============================================================

class hfVec2 {
public:
	float			x;
	float			y;

					hfVec2();
					explicit hfVec2( const float x, const float y );

	void 			Set( const float x, const float y );
	void			Zero();

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	hfVec2			operator-() const;
	float			operator*( const hfVec2 &a ) const;
	hfVec2			operator*( const float a ) const;
	hfVec2			operator/( const float a ) const;
	hfVec2			operator+( const hfVec2 &a ) const;
	hfVec2			operator-( const hfVec2 &a ) const;
	hfVec2 &		operator+=( const hfVec2 &a );
	hfVec2 &		operator-=( const hfVec2 &a );
	hfVec2 &		operator/=( const hfVec2 &a );
	hfVec2 &		operator/=( const float a );
	hfVec2 &		operator*=( const float a );

	friend hfVec2	operator*( const float a, const hfVec2 b );

	hfVec2			Scale( const hfVec2 &a ) const;

	bool			Compare( const hfVec2 &a ) const;							// exact compare, no epsilon
	bool			Compare( const hfVec2 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const hfVec2 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfVec2 &a ) const;						// exact compare, no epsilon

	float			Length() const;
	float			LengthFast() const;
	float			LengthSqr() const;
	float			Normalize();			// returns length
	float			NormalizeFast();		// returns length
	hfVec2			Truncate( float length ) const;	// cap length
	void			Clamp( const hfVec2 &min, const hfVec2 &max );
	void			Snap();				// snap to closest integer value
	void			SnapInt();			// snap towards integer (floor)

	int				GetDimension() const;

	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const hfVec2 &v1, const hfVec2 &v2, const float l );
};

extern hfVec2 vec2_origin;
#define vec2_zero vec2_origin

HF_INLINE hfVec2::hfVec2() {
}

HF_INLINE hfVec2::hfVec2( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

HF_INLINE void hfVec2::Set( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

HF_INLINE void hfVec2::Zero() {
	x = y = 0.0f;
}

HF_INLINE bool hfVec2::Compare( const hfVec2 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) );
}

HF_INLINE bool hfVec2::Compare( const hfVec2 &a, const float epsilon ) const {
	if ( hfMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
			
	if ( hfMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	return true;
}

HF_INLINE bool hfVec2::operator==( const hfVec2 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfVec2::operator!=( const hfVec2 &a ) const {
	return !Compare( a );
}

HF_INLINE float hfVec2::operator[]( int index ) const {
	return ( &x )[ index ];
}

HF_INLINE float& hfVec2::operator[]( int index ) {
	return ( &x )[ index ];
}

HF_INLINE float hfVec2::Length() const {
	return ( float )hfMath::Sqrt( x * x + y * y );
}

HF_INLINE float hfVec2::LengthFast() const {
	float sqrLength;

	sqrLength = x * x + y * y;
	return sqrLength * hfMath::InvSqrt( sqrLength );
}

HF_INLINE float hfVec2::LengthSqr() const {
	return ( x * x + y * y );
}

HF_INLINE float hfVec2::Normalize() {
	float sqrLength, invLength;

	sqrLength = x * x + y * y;
	invLength = hfMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	return invLength * sqrLength;
}

HF_INLINE float hfVec2::NormalizeFast() {
	float lengthSqr, invLength;

	lengthSqr = x * x + y * y;
	invLength = hfMath::InvSqrt( lengthSqr );
	x *= invLength;
	y *= invLength;
	return invLength * lengthSqr;
}

HF_INLINE hfVec2 hfVec2::Truncate( float length ) const {
	if ( length < hfMath::FLT_SMALLEST_NON_DENORMAL ) {
		return vec2_zero;
	} else {
		float length2 = LengthSqr();
		if ( length2 > length * length ) {
			float ilength = length * hfMath::InvSqrt( length2 );
			return *this * ilength;
		}
	}
	return *this;
}

HF_INLINE void hfVec2::Clamp( const hfVec2 &min, const hfVec2 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
}

HF_INLINE void hfVec2::Snap() {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
}

HF_INLINE void hfVec2::SnapInt() {
	x = float( int( x ) );
	y = float( int( y ) );
}

HF_INLINE hfVec2 hfVec2::operator-() const {
	return hfVec2( -x, -y );
}
	
HF_INLINE hfVec2 hfVec2::operator-( const hfVec2 &a ) const {
	return hfVec2( x - a.x, y - a.y );
}

HF_INLINE float hfVec2::operator*( const hfVec2 &a ) const {
	return x * a.x + y * a.y;
}

HF_INLINE hfVec2 hfVec2::operator*( const float a ) const {
	return hfVec2( x * a, y * a );
}

HF_INLINE hfVec2 hfVec2::operator/( const float a ) const {
	float inva = 1.0f / a;
	return hfVec2( x * inva, y * inva );
}

HF_INLINE hfVec2 operator*( const float a, const hfVec2 b ) {
	return hfVec2( b.x * a, b.y * a );
}

HF_INLINE hfVec2 hfVec2::operator+( const hfVec2 &a ) const {
	return hfVec2( x + a.x, y + a.y );
}

HF_INLINE hfVec2 &hfVec2::operator+=( const hfVec2 &a ) {
	x += a.x;
	y += a.y;

	return *this;
}

HF_INLINE hfVec2 &hfVec2::operator/=( const hfVec2 &a ) {
	x /= a.x;
	y /= a.y;

	return *this;
}

HF_INLINE hfVec2 &hfVec2::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;

	return *this;
}

HF_INLINE hfVec2 &hfVec2::operator-=( const hfVec2 &a ) {
	x -= a.x;
	y -= a.y;

	return *this;
}

HF_INLINE hfVec2 &hfVec2::operator*=( const float a ) {
	x *= a;
	y *= a;

	return *this;
}

HF_INLINE hfVec2 hfVec2::Scale( const hfVec2 &a ) const {
	return hfVec2( x * a.x, y * a.y );
}

HF_INLINE int hfVec2::GetDimension() const {
	return 2;
}

HF_INLINE const float *hfVec2::ToFloatPtr() const {
	return &x;
}

HF_INLINE float *hfVec2::ToFloatPtr() {
	return &x;
}


//===============================================================
//
//	hfVec3 - 3D vector
//
//===============================================================

class hfVec3 {
public:	
	float			x;
	float			y;
	float			z;

					hfVec3();
					explicit hfVec3( const float xyz ) { Set( xyz, xyz, xyz ); }
					explicit hfVec3( const float x, const float y, const float z );

	void 			Set( const float x, const float y, const float z );
	void			Zero();

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	hfVec3			operator-() const;
	hfVec3 &		operator=( const hfVec3 &a );		// required because of a msvc 6 & 7 bug
	float			operator*( const hfVec3 &a ) const;
	hfVec3			operator*( const float a ) const;
	hfVec3			operator/( const float a ) const;
	hfVec3			operator+( const hfVec3 &a ) const;
	hfVec3			operator-( const hfVec3 &a ) const;
	hfVec3 &		operator+=( const hfVec3 &a );
	hfVec3 &		operator-=( const hfVec3 &a );
	hfVec3 &		operator/=( const hfVec3 &a );
	hfVec3 &		operator/=( const float a );
	hfVec3 &		operator*=( const float a );

	friend hfVec3	operator*( const float a, const hfVec3 b );

	bool			Compare( const hfVec3 &a ) const;							// exact compare, no epsilon
	bool			Compare( const hfVec3 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const hfVec3 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfVec3 &a ) const;						// exact compare, no epsilon

	bool			FixDegenerateNormal();	// fix degenerate axial cases
	bool			FixDenormals();			// change tiny numbers to zero

	hfVec3			Cross( const hfVec3 &a ) const;
	hfVec3 &		Cross( const hfVec3 &a, const hfVec3 &b );
	float			Length() const;
	float			LengthSqr() const;
	float			LengthFast() const;
	float			Normalize();				// returns length
	float			NormalizeFast();			// returns length
	hfVec3			Truncate( float length ) const;		// cap length
	void			Clamp( const hfVec3 &min, const hfVec3 &max );
	void			Snap();					// snap to closest integer value
	void			SnapInt();				// snap towards integer (floor)

	int				GetDimension() const;

	float			ToYaw() const;
	float			ToPitch() const;
	hfAngles		ToAngles() const;
	hfPolar3		ToPolar() const;
	hfMat3			ToMat3() const;		// vector should be normalized
	const hfVec2 &	ToVec2() const;
	hfVec2 &		ToVec2();
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

	void			NormalVectors( hfVec3 &left, hfVec3 &down ) const;	// vector should be normalized
	void			OrthogonalBasis( hfVec3 &left, hfVec3 &up ) const;

	void			ProjectOntoPlane( const hfVec3 &normal, const float overBounce = 1.0f );
	bool			ProjectAlongPlane( const hfVec3 &normal, const float epsilon, const float overBounce = 1.0f );
	void			ProjectSelfOntoSphere( const float radius );

	void			Lerp( const hfVec3 &v1, const hfVec3 &v2, const float l );
	void			SLerp( const hfVec3 &v1, const hfVec3 &v2, const float l );
};

extern hfVec3 vec3_origin;
#define vec3_zero vec3_origin

HF_INLINE hfVec3::hfVec3() {
}

HF_INLINE hfVec3::hfVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

HF_INLINE float hfVec3::operator[]( const int index ) const {
	return ( &x )[ index ];
}

HF_INLINE float &hfVec3::operator[]( const int index ) {
	return ( &x )[ index ];
}

HF_INLINE void hfVec3::Set( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

HF_INLINE void hfVec3::Zero() {
	x = y = z = 0.0f;
}

HF_INLINE hfVec3 hfVec3::operator-() const {
	return hfVec3( -x, -y, -z );
}

HF_INLINE hfVec3 &hfVec3::operator=( const hfVec3 &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	return *this;
}

HF_INLINE hfVec3 hfVec3::operator-( const hfVec3 &a ) const {
	return hfVec3( x - a.x, y - a.y, z - a.z );
}

HF_INLINE float hfVec3::operator*( const hfVec3 &a ) const {
	return x * a.x + y * a.y + z * a.z;
}

HF_INLINE hfVec3 hfVec3::operator*( const float a ) const {
	return hfVec3( x * a, y * a, z * a );
}

HF_INLINE hfVec3 hfVec3::operator/( const float a ) const {
	float inva = 1.0f / a;
	return hfVec3( x * inva, y * inva, z * inva );
}

HF_INLINE hfVec3 operator*( const float a, const hfVec3 b ) {
	return hfVec3( b.x * a, b.y * a, b.z * a );
}

HF_INLINE hfVec3 operator/( const float a, const hfVec3 b ) {
	return hfVec3( a / b.x, a / b.y, a / b.z );
}

HF_INLINE hfVec3 hfVec3::operator+( const hfVec3 &a ) const {
	return hfVec3( x + a.x, y + a.y, z + a.z );
}

HF_INLINE hfVec3 &hfVec3::operator+=( const hfVec3 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;

	return *this;
}

HF_INLINE hfVec3 &hfVec3::operator/=( const hfVec3 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;

	return *this;
}

HF_INLINE hfVec3 &hfVec3::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;

	return *this;
}

HF_INLINE hfVec3 &hfVec3::operator-=( const hfVec3 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;

	return *this;
}

HF_INLINE hfVec3 &hfVec3::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;

	return *this;
}

HF_INLINE bool hfVec3::Compare( const hfVec3 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

HF_INLINE bool hfVec3::Compare( const hfVec3 &a, const float epsilon ) const {
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

HF_INLINE bool hfVec3::operator==( const hfVec3 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfVec3::operator!=( const hfVec3 &a ) const {
	return !Compare( a );
}

HF_INLINE float hfVec3::NormalizeFast() {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z;
	invLength = hfMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

HF_INLINE bool hfVec3::FixDegenerateNormal() {
	if ( x == 0.0f ) {
		if ( y == 0.0f ) {
			if ( z > 0.0f ) {
				if ( z != 1.0f ) {
					z = 1.0f;
					return true;
				}
			} else {
				if ( z != -1.0f ) {
					z = -1.0f;
					return true;
				}
			}
			return false;
		} else if ( z == 0.0f ) {
			if ( y > 0.0f ) {
				if ( y != 1.0f ) {
					y = 1.0f;
					return true;
				}
			} else {
				if ( y != -1.0f ) {
					y = -1.0f;
					return true;
				}
			}
			return false;
		}
	} else if ( y == 0.0f ) {
		if ( z == 0.0f ) {
			if ( x > 0.0f ) {
				if ( x != 1.0f ) {
					x = 1.0f;
					return true;
				}
			} else {
				if ( x != -1.0f ) {
					x = -1.0f;
					return true;
				}
			}
			return false;
		}
	}
	if ( hfMath::Fabs( x ) == 1.0f ) {
		if ( y != 0.0f || z != 0.0f ) {
			y = z = 0.0f;
			return true;
		}
		return false;
	} else if ( hfMath::Fabs( y ) == 1.0f ) {
		if ( x != 0.0f || z != 0.0f ) {
			x = z = 0.0f;
			return true;
		}
		return false;
	} else if ( hfMath::Fabs( z ) == 1.0f ) {
		if ( x != 0.0f || y != 0.0f ) {
			x = y = 0.0f;
			return true;
		}
		return false;
	}
	return false;
}

HF_INLINE bool hfVec3::FixDenormals() {
	bool denormal = false;
	if ( fabs( x ) < 1e-30f ) {
		x = 0.0f;
		denormal = true;
	}
	if ( fabs( y ) < 1e-30f ) {
		y = 0.0f;
		denormal = true;
	}
	if ( fabs( z ) < 1e-30f ) {
		z = 0.0f;
		denormal = true;
	}
	return denormal;
}

HF_INLINE hfVec3 hfVec3::Cross( const hfVec3 &a ) const {
	return hfVec3( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
}

HF_INLINE hfVec3 &hfVec3::Cross( const hfVec3 &a, const hfVec3 &b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	return *this;
}

HF_INLINE float hfVec3::Length() const {
	return ( float )hfMath::Sqrt( x * x + y * y + z * z );
}

HF_INLINE float hfVec3::LengthSqr() const {
	return ( x * x + y * y + z * z );
}

HF_INLINE float hfVec3::LengthFast() const {
	float sqrLength;

	sqrLength = x * x + y * y + z * z;
	return sqrLength * hfMath::InvSqrt( sqrLength );
}

HF_INLINE float hfVec3::Normalize() {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z;
	invLength = hfMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

HF_INLINE hfVec3 hfVec3::Truncate( float length ) const {
	if ( length < hfMath::FLT_SMALLEST_NON_DENORMAL ) {
		return vec3_zero;
	} else {
		float length2 = LengthSqr();
		if ( length2 > length * length ) {
			float ilength = length * hfMath::InvSqrt( length2 );
			return *this * ilength;
		}
	}
	return *this;
}

HF_INLINE void hfVec3::Clamp( const hfVec3 &min, const hfVec3 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
	if ( z < min.z ) {
		z = min.z;
	} else if ( z > max.z ) {
		z = max.z;
	}
}

HF_INLINE void hfVec3::Snap() {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
	z = floor( z + 0.5f );
}

HF_INLINE void hfVec3::SnapInt() {
	x = float( int( x ) );
	y = float( int( y ) );
	z = float( int( z ) );
}

HF_INLINE int hfVec3::GetDimension() const {
	return 3;
}

HF_INLINE const hfVec2 &hfVec3::ToVec2() const {
	return *reinterpret_cast<const hfVec2 *>(this);
}

HF_INLINE hfVec2 &hfVec3::ToVec2() {
	return *reinterpret_cast<hfVec2 *>(this);
}

HF_INLINE const float *hfVec3::ToFloatPtr() const {
	return &x;
}

HF_INLINE float *hfVec3::ToFloatPtr() {
	return &x;
}

HF_INLINE void hfVec3::NormalVectors( hfVec3 &left, hfVec3 &down ) const {
	float d;

	d = x * x + y * y;
	if ( !d ) {
		left[0] = 1;
		left[1] = 0;
		left[2] = 0;
	} else {
		d = hfMath::InvSqrt( d );
		left[0] = -y * d;
		left[1] = x * d;
		left[2] = 0;
	}
	down = left.Cross( *this );
}

HF_INLINE void hfVec3::OrthogonalBasis( hfVec3 &left, hfVec3 &up ) const {
	float l, s;

	if ( hfMath::Fabs( z ) > 0.7f ) {
		l = y * y + z * z;
		s = hfMath::InvSqrt( l );
		up[0] = 0;
		up[1] = z * s;
		up[2] = -y * s;
		left[0] = l * s;
		left[1] = -x * up[2];
		left[2] = x * up[1];
	}
	else {
		l = x * x + y * y;
		s = hfMath::InvSqrt( l );
		left[0] = -y * s;
		left[1] = x * s;
		left[2] = 0;
		up[0] = -z * left[1];
		up[1] = z * left[0];
		up[2] = l * s;
	}
}

HF_INLINE void hfVec3::ProjectOntoPlane( const hfVec3 &normal, const float overBounce ) {
	float backoff;
	
	backoff = *this * normal;
	
	if ( overBounce != 1.0 ) {
		if ( backoff < 0 ) {
			backoff *= overBounce;
		} else {
			backoff /= overBounce;
		}
	}

	*this -= backoff * normal;
}

HF_INLINE bool hfVec3::ProjectAlongPlane( const hfVec3 &normal, const float epsilon, const float overBounce ) {
	hfVec3 cross;
	float len;

	cross = this->Cross( normal ).Cross( (*this) );
	// normalize so a fixed epsilon can be used
	cross.Normalize();
	len = normal * cross;
	if ( hfMath::Fabs( len ) < epsilon ) {
		return false;
	}
	cross *= overBounce * ( normal * (*this) ) / len;
	(*this) -= cross;
	return true;
}

//===============================================================
//
//	hfTupleSize< hfVec3 > - Specialization to get the size
//	of an hfVec3 generically.
//
//===============================================================

template<>
struct hfTupleSize< hfVec3 > {
	enum { value = 3 };
};

//===============================================================
//
//	hfVec4 - 4D vector
//
//===============================================================

class hfVec4 {
public:	
	float			x;
	float			y;
	float			z;
	float			w;

					hfVec4() { }
					explicit hfVec4( const float x ) { Set( x, x, x, x ); }
					explicit hfVec4( const float x, const float y, const float z, const float w ) { Set( x, y, z, w ); }

	void 			Set( const float x, const float y, const float z, const float w );
	void			Zero();

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	hfVec4			operator-() const;
	float			operator*( const hfVec4 &a ) const;
	hfVec4			operator*( const float a ) const;
	hfVec4			operator/( const float a ) const;
	hfVec4			operator+( const hfVec4 &a ) const;
	hfVec4			operator-( const hfVec4 &a ) const;
	hfVec4 &		operator+=( const hfVec4 &a );
	hfVec4 &		operator-=( const hfVec4 &a );
	hfVec4 &		operator/=( const hfVec4 &a );
	hfVec4 &		operator/=( const float a );
	hfVec4 &		operator*=( const float a );

	friend hfVec4	operator*( const float a, const hfVec4 b );

	hfVec4			Multiply( const hfVec4 & a ) const;

	bool			Compare( const hfVec4 &a ) const;							// exact compare, no epsilon
	bool			Compare( const hfVec4 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const hfVec4 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfVec4 &a ) const;						// exact compare, no epsilon

	float			Length() const;
	float			LengthSqr() const;
	float			Normalize();			// returns length
	float			NormalizeFast();		// returns length

	int				GetDimension() const;

	const hfVec2 &	ToVec2() const;
	hfVec2 &		ToVec2();
	const hfVec3 &	ToVec3() const;
	hfVec3 &		ToVec3();
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const hfVec4 &v1, const hfVec4 &v2, const float l );
};

extern hfVec4 vec4_origin;
#define vec4_zero vec4_origin

HF_INLINE void hfVec4::Set( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

HF_INLINE void hfVec4::Zero() {
	x = y = z = w = 0.0f;
}

HF_INLINE float hfVec4::operator[]( int index ) const {
	return ( &x )[ index ];
}

HF_INLINE float& hfVec4::operator[]( int index ) {
	return ( &x )[ index ];
}

HF_INLINE hfVec4 hfVec4::operator-() const {
	return hfVec4( -x, -y, -z, -w );
}

HF_INLINE hfVec4 hfVec4::operator-( const hfVec4 &a ) const {
	return hfVec4( x - a.x, y - a.y, z - a.z, w - a.w );
}

HF_INLINE float hfVec4::operator*( const hfVec4 &a ) const {
	return x * a.x + y * a.y + z * a.z + w * a.w;
}

HF_INLINE hfVec4 hfVec4::operator*( const float a ) const {
	return hfVec4( x * a, y * a, z * a, w * a );
}

HF_INLINE hfVec4 hfVec4::operator/( const float a ) const {
	float inva = 1.0f / a;
	return hfVec4( x * inva, y * inva, z * inva, w * inva );
}

HF_INLINE hfVec4 operator*( const float a, const hfVec4 b ) {
	return hfVec4( b.x * a, b.y * a, b.z * a, b.w * a );
}

HF_INLINE hfVec4 hfVec4::operator+( const hfVec4 &a ) const {
	return hfVec4( x + a.x, y + a.y, z + a.z, w + a.w );
}

HF_INLINE hfVec4 &hfVec4::operator+=( const hfVec4 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

HF_INLINE hfVec4 &hfVec4::operator/=( const hfVec4 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;
	w /= a.w;

	return *this;
}

HF_INLINE hfVec4 &hfVec4::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;
	w *= inva;

	return *this;
}

HF_INLINE hfVec4 &hfVec4::operator-=( const hfVec4 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

HF_INLINE hfVec4 &hfVec4::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

HF_INLINE hfVec4 hfVec4::Multiply( const hfVec4 & a ) const {
	return hfVec4( x * a.x, y * a.y, z * a.z, w * a.w );
}

HF_INLINE bool hfVec4::Compare( const hfVec4 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && w == a.w );
}

HF_INLINE bool hfVec4::Compare( const hfVec4 &a, const float epsilon ) const {
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

HF_INLINE bool hfVec4::operator==( const hfVec4 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfVec4::operator!=( const hfVec4 &a ) const {
	return !Compare( a );
}

HF_INLINE float hfVec4::Length() const {
	return ( float )hfMath::Sqrt( x * x + y * y + z * z + w * w );
}

HF_INLINE float hfVec4::LengthSqr() const {
	return ( x * x + y * y + z * z + w * w );
}

HF_INLINE float hfVec4::Normalize() {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = hfMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

HF_INLINE float hfVec4::NormalizeFast() {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = hfMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

HF_INLINE int hfVec4::GetDimension() const {
	return 4;
}

HF_INLINE const hfVec2 &hfVec4::ToVec2() const {
	return *reinterpret_cast<const hfVec2 *>(this);
}

HF_INLINE hfVec2 &hfVec4::ToVec2() {
	return *reinterpret_cast<hfVec2 *>(this);
}

HF_INLINE const hfVec3 &hfVec4::ToVec3() const {
	return *reinterpret_cast<const hfVec3 *>(this);
}

HF_INLINE hfVec3 &hfVec4::ToVec3() {
	return *reinterpret_cast<hfVec3 *>(this);
}

HF_INLINE const float *hfVec4::ToFloatPtr() const {
	return &x;
}

HF_INLINE float *hfVec4::ToFloatPtr() {
	return &x;
}


//===============================================================
//
//	hfVec5 - 5D vector
//
//===============================================================

class hfVec5 {
public:
	float			x;
	float			y;
	float			z;
	float			s;
	float			t;

					hfVec5();
					explicit hfVec5( const hfVec3 &xyz, const hfVec2 &st );
					explicit hfVec5( const float x, const float y, const float z, const float s, const float t );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	hfVec5 &		operator=( const hfVec3 &a );

	int				GetDimension() const;

	const hfVec3 &	ToVec3() const;
	hfVec3 &		ToVec3();
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const hfVec5 &v1, const hfVec5 &v2, const float l );
};

extern hfVec5 vec5_origin;
#define vec5_zero vec5_origin

HF_INLINE hfVec5::hfVec5() {
}

HF_INLINE hfVec5::hfVec5( const hfVec3 &xyz, const hfVec2 &st ) {
	x = xyz.x;
	y = xyz.y;
	z = xyz.z;
	s = st[0];
	t = st[1];
}

HF_INLINE hfVec5::hfVec5( const float x, const float y, const float z, const float s, const float t ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->s = s;
	this->t = t;
}

HF_INLINE float hfVec5::operator[]( int index ) const {
	return ( &x )[ index ];
}

HF_INLINE float& hfVec5::operator[]( int index ) {
	return ( &x )[ index ];
}

HF_INLINE hfVec5 &hfVec5::operator=( const hfVec3 &a ) { 
	x = a.x;
	y = a.y;
	z = a.z;
	s = t = 0;
	return *this;
}

HF_INLINE int hfVec5::GetDimension() const {
	return 5;
}

HF_INLINE const hfVec3 &hfVec5::ToVec3() const {
	return *reinterpret_cast<const hfVec3 *>(this);
}

HF_INLINE hfVec3 &hfVec5::ToVec3() {
	return *reinterpret_cast<hfVec3 *>(this);
}

HF_INLINE const float *hfVec5::ToFloatPtr() const {
	return &x;
}

HF_INLINE float *hfVec5::ToFloatPtr() {
	return &x;
}


//===============================================================
//
//	hfVec6 - 6D vector
//
//===============================================================

class hfVec6 {
public:	
					hfVec6();
					explicit hfVec6( const float *a );
					explicit hfVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero();

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	hfVec6			operator-() const;
	hfVec6			operator*( const float a ) const;
	hfVec6			operator/( const float a ) const;
	float			operator*( const hfVec6 &a ) const;
	hfVec6			operator-( const hfVec6 &a ) const;
	hfVec6			operator+( const hfVec6 &a ) const;
	hfVec6 &		operator*=( const float a );
	hfVec6 &		operator/=( const float a );
	hfVec6 &		operator+=( const hfVec6 &a );
	hfVec6 &		operator-=( const hfVec6 &a );

	friend hfVec6	operator*( const float a, const hfVec6 b );

	bool			Compare( const hfVec6 &a ) const;							// exact compare, no epsilon
	bool			Compare( const hfVec6 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const hfVec6 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfVec6 &a ) const;						// exact compare, no epsilon

	float			Length() const;
	float			LengthSqr() const;
	float			Normalize();			// returns length
	float			NormalizeFast();		// returns length

	int				GetDimension() const;

	const hfVec3 &	SubVec3( int index ) const;
	hfVec3 &		SubVec3( int index );
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern hfVec6 vec6_origin;
#define vec6_zero vec6_origin
extern hfVec6 vec6_infinity;

HF_INLINE hfVec6::hfVec6() {
}

HF_INLINE hfVec6::hfVec6( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

HF_INLINE hfVec6::hfVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

HF_INLINE hfVec6 hfVec6::operator-() const {
	return hfVec6( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

HF_INLINE float hfVec6::operator[]( const int index ) const {
	return p[index];
}

HF_INLINE float &hfVec6::operator[]( const int index ) {
	return p[index];
}

HF_INLINE hfVec6 hfVec6::operator*( const float a ) const {
	return hfVec6( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

HF_INLINE float hfVec6::operator*( const hfVec6 &a ) const {
	return p[0] * a[0] + p[1] * a[1] + p[2] * a[2] + p[3] * a[3] + p[4] * a[4] + p[5] * a[5];
}

HF_INLINE hfVec6 hfVec6::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return hfVec6( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

HF_INLINE hfVec6 hfVec6::operator+( const hfVec6 &a ) const {
	return hfVec6( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

HF_INLINE hfVec6 hfVec6::operator-( const hfVec6 &a ) const {
	return hfVec6( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

HF_INLINE hfVec6 &hfVec6::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

HF_INLINE hfVec6 &hfVec6::operator/=( const float a ) {
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

HF_INLINE hfVec6 &hfVec6::operator+=( const hfVec6 &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

HF_INLINE hfVec6 &hfVec6::operator-=( const hfVec6 &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

HF_INLINE hfVec6 operator*( const float a, const hfVec6 b ) {
	return b * a;
}

HF_INLINE bool hfVec6::Compare( const hfVec6 &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

HF_INLINE bool hfVec6::Compare( const hfVec6 &a, const float epsilon ) const {
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

HF_INLINE bool hfVec6::operator==( const hfVec6 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfVec6::operator!=( const hfVec6 &a ) const {
	return !Compare( a );
}

HF_INLINE void hfVec6::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

HF_INLINE void hfVec6::Zero() {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

HF_INLINE float hfVec6::Length() const {
	return ( float )hfMath::Sqrt( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

HF_INLINE float hfVec6::LengthSqr() const {
	return ( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

HF_INLINE float hfVec6::Normalize() {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = hfMath::InvSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

HF_INLINE float hfVec6::NormalizeFast() {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = hfMath::InvSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

HF_INLINE int hfVec6::GetDimension() const {
	return 6;
}

HF_INLINE const hfVec3 &hfVec6::SubVec3( int index ) const {
	return *reinterpret_cast<const hfVec3 *>(p + index * 3);
}

HF_INLINE hfVec3 &hfVec6::SubVec3( int index ) {
	return *reinterpret_cast<hfVec3 *>(p + index * 3);
}

HF_INLINE const float *hfVec6::ToFloatPtr() const {
	return p;
}

HF_INLINE float *hfVec6::ToFloatPtr() {
	return p;
}

//===============================================================
//
//	hfPolar3
//
//===============================================================

class hfPolar3 {
public:	
	float			radius, theta, phi;

					hfPolar3();
					explicit hfPolar3( const float radius, const float theta, const float phi );

	void 			Set( const float radius, const float theta, const float phi );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	hfPolar3		operator-() const;
	hfPolar3 &		operator=( const hfPolar3 &a );

	hfVec3			ToVec3() const;
};

HF_INLINE hfPolar3::hfPolar3() {
}

HF_INLINE hfPolar3::hfPolar3( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}
	
HF_INLINE void hfPolar3::Set( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}

HF_INLINE float hfPolar3::operator[]( const int index ) const {
	return ( &radius )[ index ];
}

HF_INLINE float &hfPolar3::operator[]( const int index ) {
	return ( &radius )[ index ];
}

HF_INLINE hfPolar3 hfPolar3::operator-() const {
	return hfPolar3( radius, -theta, -phi );
}

HF_INLINE hfPolar3 &hfPolar3::operator=( const hfPolar3 &a ) { 
	radius = a.radius;
	theta = a.theta;
	phi = a.phi;
	return *this;
}

HF_INLINE hfVec3 hfPolar3::ToVec3() const {
	float sp, cp, st, ct;
	hfMath::SinCos( phi, sp, cp );
	hfMath::SinCos( theta, st, ct );
 	return hfVec3( cp * radius * ct, cp * radius * st, radius * sp );
}


/*
===============================================================================

	Old 3D vector macros, should no longer be used.

===============================================================================
*/

#define	VectorMA( v, s, b, o )		((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))

#endif /* !__MATH_VECTOR_H__ */

