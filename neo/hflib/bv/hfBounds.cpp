#pragma hdrstop
#include "../hfprecompiled.h"

hfBounds bounds_zero( vec3_zero, vec3_zero );
hfBounds bounds_zeroOneCube( hfVec3( 0.0f ), hfVec3( 1.0f ) );
hfBounds bounds_unitCube( hfVec3( -1.0f ), hfVec3( 1.0f ) );

/*
============
hfBounds::GetRadius
============
*/
float hfBounds::GetRadius() const {
	int		i;
	float	total, b0, b1;

	total = 0.0f;
	for ( i = 0; i < 3; i++ ) {
		b0 = (float)hfMath::Fabs( b[0][i] );
		b1 = (float)hfMath::Fabs( b[1][i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return hfMath::Sqrt( total );
}

/*
============
hfBounds::GetRadius
============
*/
float hfBounds::GetRadius( const hfVec3 &center ) const {
	int		i;
	float	total, b0, b1;

	total = 0.0f;
	for ( i = 0; i < 3; i++ ) {
		b0 = (float)hfMath::Fabs( center[i] - b[0][i] );
		b1 = (float)hfMath::Fabs( b[1][i] - center[i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return hfMath::Sqrt( total );
}

/*
================
hfBounds::PlaneDistance
================
*/
float hfBounds::PlaneDistance( const hfPlane &plane ) const {
	hfVec3 center;
	float d1, d2;

	center = ( b[0] + b[1] ) * 0.5f;

	d1 = plane.Distance( center );
	d2 = hfMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
			hfMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
				hfMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

	if ( d1 - d2 > 0.0f ) {
		return d1 - d2;
	}
	if ( d1 + d2 < 0.0f ) {
		return d1 + d2;
	}
	return 0.0f;
}

/*
================
hfBounds::PlaneSide
================
*/
int hfBounds::PlaneSide( const hfPlane &plane, const float epsilon ) const {
	hfVec3 center;
	float d1, d2;

	center = ( b[0] + b[1] ) * 0.5f;

	d1 = plane.Distance( center );
	d2 = hfMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
			hfMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
				hfMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

	if ( d1 - d2 > epsilon ) {
		return PLANESIDE_FRONT;
	}
	if ( d1 + d2 < -epsilon ) {
		return PLANESIDE_BACK;
	}
	return PLANESIDE_CROSS;
}

/*
============
hfBounds::LineIntersection

  Returns true if the line intersects the bounds between the start and end point.
============
*/
bool hfBounds::LineIntersection( const hfVec3 &start, const hfVec3 &end ) const {
	const hfVec3 center = ( b[0] + b[1] ) * 0.5f;
	const hfVec3 extents = b[1] - center;
	const hfVec3 lineDir = 0.5f * ( end - start );
	const hfVec3 lineCenter = start + lineDir;
	const hfVec3 dir = lineCenter - center;

	const float ld0 = hfMath::Fabs( lineDir[0] );
	if ( hfMath::Fabs( dir[0] ) > extents[0] + ld0 ) {
		return false;
	}

	const float ld1 = hfMath::Fabs( lineDir[1] );
	if ( hfMath::Fabs( dir[1] ) > extents[1] + ld1 ) {
		return false;
	}

	const float ld2 = hfMath::Fabs( lineDir[2] );
	if ( hfMath::Fabs( dir[2] ) > extents[2] + ld2 ) {
		return false;
	}

	const hfVec3 cross = lineDir.Cross( dir );

	if ( hfMath::Fabs( cross[0] ) > extents[1] * ld2 + extents[2] * ld1 ) {
		return false;
	}

	if ( hfMath::Fabs( cross[1] ) > extents[0] * ld2 + extents[2] * ld0 ) {
		return false;
	}

	if ( hfMath::Fabs( cross[2] ) > extents[0] * ld1 + extents[1] * ld0 ) {
		return false;
	}

	return true;
}

/*
============
hfBounds::RayIntersection

  Returns true if the ray intersects the bounds.
  The ray can intersect the bounds in both directions from the start point.
  If start is inside the bounds it is considered an intersection with scale = 0
============
*/
bool hfBounds::RayIntersection( const hfVec3 &start, const hfVec3 &dir, float &scale ) const {
	int i, ax0, ax1, ax2, side, inside;
	float f;
	hfVec3 hit;

	ax0 = -1;
	inside = 0;
	for ( i = 0; i < 3; i++ ) {
		if ( start[i] < b[0][i] ) {
			side = 0;
		}
		else if ( start[i] > b[1][i] ) {
			side = 1;
		}
		else {
			inside++;
			continue;
		}
		if ( dir[i] == 0.0f ) {
			continue;
		}
		f = ( start[i] - b[side][i] );
		if ( ax0 < 0 || hfMath::Fabs( f ) > hfMath::Fabs( scale * dir[i] ) ) {
			scale = - ( f / dir[i] );
			ax0 = i;
		}
	}

	if ( ax0 < 0 ) {
		scale = 0.0f;
		// return true if the start point is inside the bounds
		return ( inside == 3 );
	}

	ax1 = (ax0+1)%3;
	ax2 = (ax0+2)%3;
	hit[ax1] = start[ax1] + scale * dir[ax1];
	hit[ax2] = start[ax2] + scale * dir[ax2];

	return ( hit[ax1] >= b[0][ax1] && hit[ax1] <= b[1][ax1] &&
				hit[ax2] >= b[0][ax2] && hit[ax2] <= b[1][ax2] );
}

/*
============
hfBounds::FromTransformedBounds
============
*/
void hfBounds::FromTransformedBounds( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis ) {
	int i;
	hfVec3 center, extents, rotatedExtents;

	center = (bounds[0] + bounds[1]) * 0.5f;
	extents = bounds[1] - center;

	for ( i = 0; i < 3; i++ ) {
		rotatedExtents[i] = hfMath::Fabs( extents[0] * axis[0][i] ) +
							hfMath::Fabs( extents[1] * axis[1][i] ) +
							hfMath::Fabs( extents[2] * axis[2][i] );
	}

	center = origin + center * axis;
	b[0] = center - rotatedExtents;
	b[1] = center + rotatedExtents;
}

/*
============
hfBounds::FromPoints

  Most tight bounds for a point set.
============
*/
void hfBounds::FromPoints( const hfVec3 *points, const int numPoints ) {
	SIMDProcessor->MinMax( b[0], b[1], points, numPoints );
}

/*
============
hfBounds::FromPointTranslation

  Most tight bounds for the translational movement of the given point.
============
*/
void hfBounds::FromPointTranslation( const hfVec3 &point, const hfVec3 &translation ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( translation[i] < 0.0f ) {
			b[0][i] = point[i] + translation[i];
			b[1][i] = point[i];
		}
		else {
			b[0][i] = point[i];
			b[1][i] = point[i] + translation[i];
		}
	}
}

/*
============
hfBounds::FromBoundsTranslation

  Most tight bounds for the translational movement of the given bounds.
============
*/
void hfBounds::FromBoundsTranslation( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis, const hfVec3 &translation ) {
	int i;

	if ( axis.IsRotated() ) {
		FromTransformedBounds( bounds, origin, axis );
	}
	else {
		b[0] = bounds[0] + origin;
		b[1] = bounds[1] + origin;
	}
	for ( i = 0; i < 3; i++ ) {
		if ( translation[i] < 0.0f ) {
			b[0][i] += translation[i];
		}
		else {
			b[1][i] += translation[i];
		}
	}
}

/*
================
BoundsForPointRotation

  only for rotations < 180 degrees
================
*/
hfBounds BoundsForPointRotation( const hfVec3 &start, const hfRotation &rotation ) {
	int i;
	float radiusSqr;
	hfVec3 v1, v2;
	hfVec3 origin, axis, end;
	hfBounds bounds;

	end = start * rotation;
	axis = rotation.GetVec();
	origin = rotation.GetOrigin() + axis * ( axis * ( start - rotation.GetOrigin() ) );
	radiusSqr = ( start - origin ).LengthSqr();
	v1 = ( start - origin ).Cross( axis );
	v2 = ( end - origin ).Cross( axis );

	for ( i = 0; i < 3; i++ ) {
		// if the derivative changes sign along this axis during the rotation from start to end
		if ( ( v1[i] > 0.0f && v2[i] < 0.0f ) || ( v1[i] < 0.0f && v2[i] > 0.0f ) ) {
			if ( ( 0.5f * (start[i] + end[i]) - origin[i] ) > 0.0f ) {
				bounds[0][i] = Min( start[i], end[i] );
				bounds[1][i] = origin[i] + hfMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
			}
			else {
				bounds[0][i] = origin[i] - hfMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
				bounds[1][i] = Max( start[i], end[i] );
			}
		}
		else if ( start[i] > end[i] ) {
			bounds[0][i] = end[i];
			bounds[1][i] = start[i];
		}
		else {
			bounds[0][i] = start[i];
			bounds[1][i] = end[i];
		}
	}

	return bounds;
}

/*
============
hfBounds::FromPointRotation

  Most tight bounds for the rotational movement of the given point.
============
*/
void hfBounds::FromPointRotation( const hfVec3 &point, const hfRotation &rotation ) {
	float radius;

	if ( hfMath::Fabs( rotation.GetAngle() ) < 180.0f ) {
		(*this) = BoundsForPointRotation( point, rotation );
	}
	else {

		radius = ( point - rotation.GetOrigin() ).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );
	}
}

/*
============
hfBounds::FromBoundsRotation

  Most tight bounds for the rotational movement of the given bounds.
============
*/
void hfBounds::FromBoundsRotation( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis, const hfRotation &rotation ) {
	int i;
	float radius;
	hfVec3 point;
	hfBounds rBounds;

	if ( hfMath::Fabs( rotation.GetAngle() ) < 180.0f ) {

		(*this) = BoundsForPointRotation( bounds[0] * axis + origin, rotation );
		for ( i = 1; i < 8; i++ ) {
			point[0] = bounds[(i^(i>>1))&1][0];
			point[1] = bounds[(i>>1)&1][1];
			point[2] = bounds[(i>>2)&1][2];
			(*this) += BoundsForPointRotation( point * axis + origin, rotation );
		}
	}
	else {

		point = (bounds[1] - bounds[0]) * 0.5f;
		radius = (bounds[1] - point).Length() + (point - rotation.GetOrigin()).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );
	}
}

/*
============
hfBounds::ToPoints
============
*/
void hfBounds::ToPoints( hfVec3 points[8] ) const {
	for ( int i = 0; i < 8; i++ ) {
		points[i][0] = b[(i^(i>>1))&1][0];
		points[i][1] = b[(i>>1)&1][1];
		points[i][2] = b[(i>>2)&1][2];
	}
}

