#pragma hdrstop
#include "../hfprecompiled.h"

/*
============
hfRotation::ToAngles
============
*/
hfAngles hfRotation::ToAngles() const {
	return ToMat3().ToAngles();
}

/*
============
hfRotation::ToQuat
============
*/
hfQuat hfRotation::ToQuat() const {
	float a, s, c;

	a = angle * ( hfMath::M_DEG2RAD * 0.5f );
	hfMath::SinCos( a, s, c );
	return hfQuat( vec.x * s, vec.y * s, vec.z * s, c );
}

/*
============
hfRotation::toMat3
============
*/
const hfMat3 &hfRotation::ToMat3() const {
	float wx, wy, wz;
	float xx, yy, yz;
	float xy, xz, zz;
	float x2, y2, z2;
	float a, c, s, x, y, z;

	if ( axisValid ) {
		return axis;
	}

	a = angle * ( hfMath::M_DEG2RAD * 0.5f );
	hfMath::SinCos( a, s, c );

	x = vec[0] * s;
	y = vec[1] * s;
	z = vec[2] * s;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = c * x2;
	wy = c * y2;
	wz = c * z2;

	axis[ 0 ][ 0 ] = 1.0f - ( yy + zz );
	axis[ 0 ][ 1 ] = xy - wz;
	axis[ 0 ][ 2 ] = xz + wy;

	axis[ 1 ][ 0 ] = xy + wz;
	axis[ 1 ][ 1 ] = 1.0f - ( xx + zz );
	axis[ 1 ][ 2 ] = yz - wx;

	axis[ 2 ][ 0 ] = xz - wy;
	axis[ 2 ][ 1 ] = yz + wx;
	axis[ 2 ][ 2 ] = 1.0f - ( xx + yy );

	axisValid = true;

	return axis;
}

/*
============
hfRotation::ToMat4
============
*/
hfMat4 hfRotation::ToMat4() const {
	return ToMat3().ToMat4();
}

/*
============
hfRotation::ToAngularVelocity
============
*/
hfVec3 hfRotation::ToAngularVelocity() const {
	return vec * DEG2RAD( angle );
}

/*
============
hfRotation::Normalize180
============
*/
void hfRotation::Normalize180() {
	angle -= floor( angle / 360.0f ) * 360.0f;
	if ( angle > 180.0f ) {
		angle -= 360.0f;
	}
	else if ( angle < -180.0f ) {
		angle += 360.0f;
	}
}

/*
============
hfRotation::Normalize360
============
*/
void hfRotation::Normalize360() {
	angle -= floor( angle / 360.0f ) * 360.0f;
	if ( angle > 360.0f ) {
		angle -= 360.0f;
	}
	else if ( angle < 0.0f ) {
		angle += 360.0f;
	}
}

