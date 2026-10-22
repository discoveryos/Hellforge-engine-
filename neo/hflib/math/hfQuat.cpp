#pragma hdrstop
#include "../hfprecompiled.h"

/*
=====================
hfQuat::ToAngles
=====================
*/
hfAngles hfQuat::ToAngles() const {
	return ToMat3().ToAngles();
}

/*
=====================
hfQuat::ToRotation
=====================
*/
hfRotation hfQuat::ToRotation() const {
	hfVec3 vec;
	float angle;

	vec.x = x;
	vec.y = y;
	vec.z = z;
	angle = hfMath::ACos( w );
	if ( angle == 0.0f ) {
		vec.Set( 0.0f, 0.0f, 1.0f );
	} else {
		//vec *= (1.0f / sin( angle ));
		vec.Normalize();
		vec.FixDegenerateNormal();
		angle *= 2.0f * hfMath::M_RAD2DEG;
	}
	return hfRotation( vec3_origin, vec, angle );
}

/*
=====================
hfQuat::ToMat3
=====================
*/
hfMat3 hfQuat::ToMat3() const {
	hfMat3	mat;
	float	wx, wy, wz;
	float	xx, yy, yz;
	float	xy, xz, zz;
	float	x2, y2, z2;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = w * x2;
	wy = w * y2;
	wz = w * z2;

	mat[ 0 ][ 0 ] = 1.0f - ( yy + zz );
	mat[ 0 ][ 1 ] = xy - wz;
	mat[ 0 ][ 2 ] = xz + wy;

	mat[ 1 ][ 0 ] = xy + wz;
	mat[ 1 ][ 1 ] = 1.0f - ( xx + zz );
	mat[ 1 ][ 2 ] = yz - wx;

	mat[ 2 ][ 0 ] = xz - wy;
	mat[ 2 ][ 1 ] = yz + wx;
	mat[ 2 ][ 2 ] = 1.0f - ( xx + yy );

	return mat;
}

/*
=====================
hfQuat::ToMat4
=====================
*/
hfMat4 hfQuat::ToMat4() const {
	return ToMat3().ToMat4();
}

/*
=====================
hfQuat::ToCQuat
=====================
*/
hfCQuat hfQuat::ToCQuat() const {
	if ( w < 0.0f ) {
		return hfCQuat( -x, -y, -z );
	}
	return hfCQuat( x, y, z );
}

/*
============
hfQuat::ToAngularVelocity
============
*/
hfVec3 hfQuat::ToAngularVelocity() const {
	hfVec3 vec;

	vec.x = x;
	vec.y = y;
	vec.z = z;
	vec.Normalize();
	return vec * hfMath::ACos( w );
}

/*
=============
hfQuat::ToString
=============
*/
const char *hfQuat::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=====================
hfQuat::Slerp

Spherical linear interpolation between two quaternions.
=====================
*/
hfQuat &hfQuat::Slerp( const hfQuat &from, const hfQuat &to, float t ) {
	hfQuat	temp;
	float	omega, cosom, sinom, scale0, scale1;

	if ( t <= 0.0f ) {
		*this = from;
		return *this;
	}

	if ( t >= 1.0f ) {
		*this = to;
		return *this;
	}

	if ( from == to ) {
		*this = to;
		return *this;
	}

	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	if ( cosom < 0.0f ) {
		temp = -to;
		cosom = -cosom;
	} else {
		temp = to;
	}

	if ( ( 1.0f - cosom ) > 1e-6f ) {
#if 0
		omega = acos( cosom );
		sinom = 1.0f / sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) * sinom;
		scale1 = sin( t * omega ) * sinom;
#else
		scale0 = 1.0f - cosom * cosom;
		sinom = hfMath::InvSqrt( scale0 );
		omega = hfMath::ATan16( scale0 * sinom, cosom );
		scale0 = hfMath::Sin16( ( 1.0f - t ) * omega ) * sinom;
		scale1 = hfMath::Sin16( t * omega ) * sinom;
#endif
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	*this = ( scale0 * from ) + ( scale1 * temp );
	return *this;
}

/*
========================
hfQuat::Lerp

Approximation of spherical linear interpolation between two quaternions. The interpolation 
traces out the exact same curve as Slerp but does not maintain a constant speed across the arc.
========================
*/
hfQuat &hfQuat::Lerp( const hfQuat &from, const hfQuat &to, const float t ) {
	if ( t <= 0.0f ) {
		*this = from;
		return *this;
	}

	if ( t >= 1.0f ) {
		*this = to;
		return *this;
	}

	if ( from == to ) {
		*this = to;
		return *this;
	}

	float cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

	float scale0 = 1.0f - t;
	float scale1 = ( cosom >= 0.0f ) ? t : -t;

	x = scale0 * from.x + scale1 * to.x;
	y = scale0 * from.y + scale1 * to.y;
	z = scale0 * from.z + scale1 * to.z;
	w = scale0 * from.w + scale1 * to.w;

	float s = hfMath::InvSqrt( x * x + y * y + z * z + w * w );

	x *= s;
	y *= s;
	z *= s;
	w *= s;

	return *this;
}

/*
=============
hfCQuat::ToAngles
=============
*/
hfAngles hfCQuat::ToAngles() const {
	return ToQuat().ToAngles();
}

/*
=============
hfCQuat::ToRotation
=============
*/
hfRotation hfCQuat::ToRotation() const {
	return ToQuat().ToRotation();
}

/*
=============
hfCQuat::ToMat3
=============
*/
hfMat3 hfCQuat::ToMat3() const {
	return ToQuat().ToMat3();
}

/*
=============
hfCQuat::ToMat4
=============
*/
hfMat4 hfCQuat::ToMat4() const {
	return ToQuat().ToMat4();
}

/*
=============
hfCQuat::ToString
=============
*/
const char *hfCQuat::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=====================
Slerp

Spherical linear interpolation between two quaternions.
=====================
*/
hfQuat Slerp( const hfQuat & from, const hfQuat & to, const float t ) {
	return hfQuat().Slerp( from, to, t );
}

