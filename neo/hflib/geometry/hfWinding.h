#ifndef __WINDING_H__
#define __WINDING_H__

/*
===============================================================================

	A winding is an arbitrary convex polygon defined by an array of points.

===============================================================================
*/

class hfWinding {

public:
					hfWinding();
					explicit hfWinding( const int n );								// allocate for n points
					explicit hfWinding( const hfVec3 *verts, const int n );			// winding from points
					explicit hfWinding( const hfVec3 &normal, const float dist );	// base winding for plane
					explicit hfWinding( const hfPlane &plane );						// base winding for plane
					explicit hfWinding( const hfWinding &winding );
	virtual			~hfWinding();

	hfWinding &		operator=( const hfWinding &winding );
	const hfVec5 &	operator[]( const int index ) const;
	hfVec5 &		operator[]( const int index );

					// add a point to the end of the winding point array
	hfWinding &		operator+=( const hfVec3 &v );
	hfWinding &		operator+=( const hfVec5 &v );
	void			AddPoint( const hfVec3 &v );
	void			AddPoint( const hfVec5 &v );

					// number of points on winding
	int				GetNumPoints() const;
	void			SetNumPoints( int n );
	virtual void	Clear();

					// huge winding for plane, the points go counter clockwise when facing the front of the plane
	void			BaseForPlane( const hfVec3 &normal, const float dist );
	void			BaseForPlane( const hfPlane &plane );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const hfPlane &plane, const float epsilon, hfWinding **front, hfWinding **back ) const;
					// returns the winding fragment at the front of the clipping plane,
					// if there is nothing at the front the winding itself is destroyed and NULL is returned
	hfWinding *		Clip( const hfPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const hfPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

					// returns a copy of the winding
	hfWinding *		Copy() const;
	hfWinding *		Reverse() const;
	void			ReverseSelf();
	void			RemoveEqualPoints( const float epsilon = ON_EPSILON );
	void			RemoveColinearPoints( const hfVec3 &normal, const float epsilon = ON_EPSILON );
	void			RemovePoint( int point );
	void			InsertPoint( const hfVec5 &point, int spot );
	bool			InsertPointIfOnEdge( const hfVec5 &point, const hfPlane &plane, const float epsilon = ON_EPSILON );
					// add a winding to the convex hull
	void			AddToConvexHull( const hfWinding *winding, const hfVec3 &normal, const float epsilon = ON_EPSILON );
					// add a point to the convex hull
	void			AddToConvexHull( const hfVec3 &point, const hfVec3 &normal, const float epsilon = ON_EPSILON );
					// tries to merge 'this' with the given winding, returns NULL if merge fails, both 'this' and 'w' stay intact
					// 'keep' tells if the contacting points should stay even if they create colinear edges
	hfWinding *		TryMerge( const hfWinding &w, const hfVec3 &normal, int keep = false ) const;
					// check whether the winding is valid or not
	bool			Check( bool print = true ) const;

	float			GetArea() const;
	hfVec3			GetCenter() const;
	float			GetRadius( const hfVec3 &center ) const;
	void			GetPlane( hfVec3 &normal, float &dist ) const;
	void			GetPlane( hfPlane &plane ) const;
	void			GetBounds( hfBounds &bounds ) const;

	bool			IsTiny() const;
	bool			IsHuge() const;	// base winding for a plane is typically huge
	void			Print() const;

	float			PlaneDistance( const hfPlane &plane ) const;
	int				PlaneSide( const hfPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			PlanesConcave( const hfWinding &w2, const hfVec3 &normal1, const hfVec3 &normal2, float dist1, float dist2 ) const;

	bool			PointInside( const hfVec3 &normal, const hfVec3 &point, const float epsilon ) const;
					// returns true if the line or ray intersects the winding
	bool			LineIntersection( const hfPlane &windingPlane, const hfVec3 &start, const hfVec3 &end, bool backFaceCull = false ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const hfPlane &windingPlane, const hfVec3 &start, const hfVec3 &dir, float &scale, bool backFaceCull = false ) const;

	static float	TriangleArea( const hfVec3 &a, const hfVec3 &b, const hfVec3 &c );

protected:
	int				numPoints;				// number of points
	hfVec5 *		p;						// pointer to point data
	int				allocedSize;

	bool			EnsureAlloced( int n, bool keep = false );
	virtual bool	ReAllocate( int n, bool keep = false );
};

HF_INLINE hfWinding::hfWinding() {
	numPoints = allocedSize = 0;
	p = NULL;
}

HF_INLINE hfWinding::hfWinding( int n ) {
	numPoints = allocedSize = 0;
	p = NULL;
	EnsureAlloced( n );
}

HF_INLINE hfWinding::hfWinding( const hfVec3 *verts, const int n ) {
	int i;

	numPoints = allocedSize = 0;
	p = NULL;
	if ( !EnsureAlloced( n ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < n; i++ ) {
		p[i].ToVec3() = verts[i];
		p[i].s = p[i].t = 0.0f;
	}
	numPoints = n;
}

HF_INLINE hfWinding::hfWinding( const hfVec3 &normal, const float dist ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( normal, dist );
}

HF_INLINE hfWinding::hfWinding( const hfPlane &plane ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( plane );
}

HF_INLINE hfWinding::hfWinding( const hfWinding &winding ) {
	int i;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

HF_INLINE hfWinding::~hfWinding() {
	delete[] p;
	p = NULL;
}

HF_INLINE hfWinding &hfWinding::operator=( const hfWinding &winding ) {
	int i;

	if ( !EnsureAlloced( winding.numPoints ) ) {
		numPoints = 0;
		return *this;
	}
	for ( i = 0; i < winding.numPoints; i++ ) {
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

HF_INLINE const hfVec5 &hfWinding::operator[]( const int index ) const {
	//assert( index >= 0 && index < numPoints );
	return p[ index ];
}

HF_INLINE hfVec5 &hfWinding::operator[]( const int index ) {
	//assert( index >= 0 && index < numPoints );
	return p[ index ];
}

HF_INLINE hfWinding &hfWinding::operator+=( const hfVec3 &v ) {
	AddPoint( v );
	return *this;
}

HF_INLINE hfWinding &hfWinding::operator+=( const hfVec5 &v ) {
	AddPoint( v );
	return *this;
}

HF_INLINE void hfWinding::AddPoint( const hfVec3 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

HF_INLINE void hfWinding::AddPoint( const hfVec5 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

HF_INLINE int hfWinding::GetNumPoints() const {
	return numPoints;
}

HF_INLINE void hfWinding::SetNumPoints( int n ) {
	if ( !EnsureAlloced( n, true ) ) {
		return;
	}
	numPoints = n;
}

HF_INLINE void hfWinding::Clear() {
	numPoints = 0;
	delete[] p;
	p = NULL;
}

HF_INLINE void hfWinding::BaseForPlane( const hfPlane &plane ) {
	BaseForPlane( plane.Normal(), plane.Dist() );
}

HF_INLINE bool hfWinding::EnsureAlloced( int n, bool keep ) {
	if ( n > allocedSize ) {
		return ReAllocate( n, keep );
	}
	return true;
}


/*
===============================================================================

	hfFixedWinding is a fixed buffer size winding not using
	memory allocations.

	When an operation would overflow the fixed buffer a warning
	is printed and the operation is safely cancelled.

===============================================================================
*/

#define	MAX_POINTS_ON_WINDING	64

class hfFixedWinding : public hfWinding {

public:
					hfFixedWinding();
					explicit hfFixedWinding( const int n );
					explicit hfFixedWinding( const hfVec3 *verts, const int n );
					explicit hfFixedWinding( const hfVec3 &normal, const float dist );
					explicit hfFixedWinding( const hfPlane &plane );
					explicit hfFixedWinding( const hfWinding &winding );
					explicit hfFixedWinding( const hfFixedWinding &winding );
	virtual			~hfFixedWinding();

	hfFixedWinding &operator=( const hfWinding &winding );

	virtual void	Clear();

					// splits the winding in a back and front part, 'this' becomes the front part
					// returns a SIDE_?
	int				Split( hfFixedWinding *back, const hfPlane &plane, const float epsilon = ON_EPSILON );

protected:
	hfVec5			data[MAX_POINTS_ON_WINDING];	// point data

	virtual bool	ReAllocate( int n, bool keep = false );
};

HF_INLINE hfFixedWinding::hfFixedWinding() {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

HF_INLINE hfFixedWinding::hfFixedWinding( int n ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

HF_INLINE hfFixedWinding::hfFixedWinding( const hfVec3 *verts, const int n ) {
	int i;

	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( n ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < n; i++ ) {
		p[i].ToVec3() = verts[i];
		p[i].s = p[i].t = 0;
	}
	numPoints = n;
}

HF_INLINE hfFixedWinding::hfFixedWinding( const hfVec3 &normal, const float dist ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( normal, dist );
}

HF_INLINE hfFixedWinding::hfFixedWinding( const hfPlane &plane ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( plane );
}

HF_INLINE hfFixedWinding::hfFixedWinding( const hfWinding &winding ) {
	int i;

	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

HF_INLINE hfFixedWinding::hfFixedWinding( const hfFixedWinding &winding ) {
	int i;

	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

HF_INLINE hfFixedWinding::~hfFixedWinding() {
	p = NULL;	// otherwise it tries to free the fixed buffer
}

HF_INLINE hfFixedWinding &hfFixedWinding::operator=( const hfWinding &winding ) {
	int i;

	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return *this;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
	return *this;
}

HF_INLINE void hfFixedWinding::Clear() {
	numPoints = 0;
}
#endif	/* !__WINDING_H__ */

