#pragma hdrstop
#include "../hfprecompiled.h"

hfVec2 vec2_origin( 0.0f, 0.0f );
hfVec3 vec3_origin( 0.0f, 0.0f, 0.0f );
hfVec4 vec4_origin( 0.0f, 0.0f, 0.0f, 0.0f );
hfVec5 vec5_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
hfVec6 vec6_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
hfVec6 vec6_infinity( hfMath::INFINITY, hfMath::INFINITY, hfMath::INFINITY, hfMath::INFINITY, hfMath::INFINITY, hfMath::INFINITY );


//===============================================================
//
//	hfVec2
//
//===============================================================

/*
=============
hfVec2::ToString
=============
*/
const char *hfVec2::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void hfVec2::Lerp( const hfVec2 &v1, const hfVec2 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}


//===============================================================
//
//	hfVec3
//
//===============================================================

/*
=============
hfVec3::ToYaw
=============
*/
float hfVec3::ToYaw() const {
	float yaw;
	
	if ( ( y == 0.0f ) && ( x == 0.0f ) ) {
		yaw = 0.0f;
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}
	}

	return yaw;
}

/*
=============
hfVec3::ToPitch
=============
*/
float hfVec3::ToPitch() const {
	float	forward;
	float	pitch;
	
	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		forward = ( float )hfMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return pitch;
}

/*
=============
hfVec3::ToAngles
=============
*/
hfAngles hfVec3::ToAngles() const {
	float forward;
	float yaw;
	float pitch;
	
	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )hfMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return hfAngles( -pitch, yaw, 0.0f );
}

/*
=============
hfVec3::ToPolar
=============
*/
hfPolar3 hfVec3::ToPolar() const {
	float forward;
	float yaw;
	float pitch;
	
	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )hfMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}
	return hfPolar3( hfMath::Sqrt( x * x + y * y + z * z ), yaw, -pitch );
}

/*
=============
hfVec3::ToMat3
=============
*/
hfMat3 hfVec3::ToMat3() const {
	hfMat3	mat;
	float	d;

	mat[0] = *this;
	d = x * x + y * y;
	if ( !d ) {
		mat[1][0] = 1.0f;
		mat[1][1] = 0.0f;
		mat[1][2] = 0.0f;
	} else {
		d = hfMath::InvSqrt( d );
		mat[1][0] = -y * d;
		mat[1][1] = x * d;
		mat[1][2] = 0.0f;
	}
	mat[2] = Cross( mat[1] );

	return mat;
}

/*
=============
hfVec3::ToString
=============
*/
const char *hfVec3::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void hfVec3::Lerp( const hfVec3 &v1, const hfVec3 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}

/*
=============
SLerp

Spherical linear interpolation from v1 to v2.
Vectors are expected to be normalized.
=============
*/
#define LERP_DELTA 1e-6

void hfVec3::SLerp( const hfVec3 &v1, const hfVec3 &v2, const float t ) {
	float omega, cosom, sinom, scale0, scale1;

	if ( t <= 0.0f ) {
		(*this) = v1;
		return;
	} else if ( t >= 1.0f ) {
		(*this) = v2;
		return;
	}

	cosom = v1 * v2;
	if ( ( 1.0f - cosom ) > LERP_DELTA ) {
		omega = acos( cosom );
		sinom = sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) / sinom;
		scale1 = sin( t * omega ) / sinom;
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	(*this) = ( v1 * scale0 + v2 * scale1 );
}

/*
=============
ProjectSelfOntoSphere

Projects the z component onto a sphere.
=============
*/
void hfVec3::ProjectSelfOntoSphere( const float radius ) {
	float rsqr = radius * radius;
	float len = Length();
	if ( len  < rsqr * 0.5f ) {
		z = sqrt( rsqr - len );
	} else {
		z = rsqr / ( 2.0f * sqrt( len ) );
	}
}



//===============================================================
//
//	hfVec4
//
//===============================================================

/*
=============
hfVec4::ToString
=============
*/
const char *hfVec4::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void hfVec4::Lerp( const hfVec4 &v1, const hfVec4 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}


//===============================================================
//
//	hfVec5
//
//===============================================================

/*
=============
hfVec5::ToString
=============
*/
const char *hfVec5::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
hfVec5::Lerp
=============
*/
void hfVec5::Lerp( const hfVec5 &v1, const hfVec5 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		x = v1.x + l * ( v2.x - v1.x );
		y = v1.y + l * ( v2.y - v1.y );
		z = v1.z + l * ( v2.z - v1.z );
		s = v1.s + l * ( v2.s - v1.s );
		t = v1.t + l * ( v2.t - v1.t );
	}
}


//===============================================================
//
//	hfVec6
//
//===============================================================

/*
=============
hfVec6::ToString
=============
*/
const char *hfVec6::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

