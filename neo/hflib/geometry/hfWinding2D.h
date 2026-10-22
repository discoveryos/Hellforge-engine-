#ifndef __WINDING2D_H__
#define __WINDING2D_H__

/*
===============================================================================

	A 2D winding is an arbitrary convex 2D polygon defined by an array of points.

===============================================================================
*/

#define	MAX_POINTS_ON_WINDING_2D		16


class hfWinding2D {
public:
					hfWinding2D();

	hfWinding2D &	operator=( const hfWinding2D &winding );
	const hfVec2 &	operator[]( const int index ) const;
	hfVec2 &		operator[]( const int index );

	void			Clear();
	void			AddPoint( const hfVec2 &point );
	int				GetNumPoints() const;

	void			Expand( const float d );
	void			ExpandForAxialBox( const hfVec2 bounds[2] );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const hfVec3 &plane, const float epsilon, hfWinding2D **front, hfWinding2D **back ) const;
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const hfVec3 &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

	hfWinding2D *	Copy() const;
	hfWinding2D *	Reverse() const;

	float			GetArea() const;
	hfVec2			GetCenter() const;
	float			GetRadius( const hfVec2 &center ) const;
	void			GetBounds( hfVec2 bounds[2] ) const;

	bool			IsTiny() const;
	bool			IsHuge() const;	// base winding for a plane is typically huge
	void			Print() const;

	float			PlaneDistance( const hfVec3 &plane ) const;
	int				PlaneSide( const hfVec3 &plane, const float epsilon = ON_EPSILON ) const;

	bool			PointInside( const hfVec2 &point, const float epsilon ) const;
	bool			LineIntersection( const hfVec2 &start, const hfVec2 &end ) const;
	bool			RayIntersection( const hfVec2 &start, const hfVec2 &dir, float &scale1, float &scale2, int *edgeNums = NULL ) const;

	static hfVec3	Plane2DFromPoints( const hfVec2 &start, const hfVec2 &end, const bool normalize = false );
	static hfVec3	Plane2DFromVecs( const hfVec2 &start, const hfVec2 &dir, const bool normalize = false );
	static bool		Plane2DIntersection( const hfVec3 &plane1, const hfVec3 &plane2, hfVec2 &point );

private:
	int				numPoints;
	hfVec2			p[MAX_POINTS_ON_WINDING_2D];
};

HF_INLINE hfWinding2D::hfWinding2D() {
	numPoints = 0;
}

HF_INLINE hfWinding2D &hfWinding2D::operator=( const hfWinding2D &winding ) {
	int i;

	for ( i = 0; i < winding.numPoints; i++ ) {
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

HF_INLINE const hfVec2 &hfWinding2D::operator[]( const int index ) const {
	return p[ index ];
}

HF_INLINE hfVec2 &hfWinding2D::operator[]( const int index ) {
	return p[ index ];
}

HF_INLINE void hfWinding2D::Clear() {
	numPoints = 0;
}

HF_INLINE void hfWinding2D::AddPoint( const hfVec2 &point ) {
	p[numPoints++] = point;
}

HF_INLINE int hfWinding2D::GetNumPoints() const {
	return numPoints;
}

HF_INLINE hfVec3 hfWinding2D::Plane2DFromPoints( const hfVec2 &start, const hfVec2 &end, const bool normalize ) {
	hfVec3 plane;
	plane.x = start.y - end.y;
	plane.y = end.x - start.x;
	if ( normalize ) {
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

HF_INLINE hfVec3 hfWinding2D::Plane2DFromVecs( const hfVec2 &start, const hfVec2 &dir, const bool normalize ) {
	hfVec3 plane;
	plane.x = -dir.y;
	plane.y = dir.x;
	if ( normalize ) {
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

HF_INLINE bool hfWinding2D::Plane2DIntersection( const hfVec3 &plane1, const hfVec3 &plane2, hfVec2 &point ) {
	float n00, n01, n11, det, invDet, f0, f1;

	n00 = plane1.x * plane1.x + plane1.y * plane1.y;
	n01 = plane1.x * plane2.x + plane1.y * plane2.y;
	n11 = plane2.x * plane2.x + plane2.y * plane2.y;
	det = n00 * n11 - n01 * n01;

	if ( hfMath::Fabs(det) < 1e-6f ) {
		return false;
	}

	invDet = 1.0f / det;
	f0 = ( n01 * plane2.z - n11 * plane1.z ) * invDet;
	f1 = ( n01 * plane1.z - n00 * plane2.z ) * invDet;
	point.x = f0 * plane1.x + f1 * plane2.x;
	point.y = f0 * plane1.y + f1 * plane2.y;
	return true;
}

#endif /* !__WINDING2D_H__ */

