#ifndef __BV_BOUNDS_H__
#define __BV_BOUNDS_H__

/*
===============================================================================

	Axis Aligned Bounding Box

===============================================================================
*/

class hfBounds {
public:
					hfBounds();
					explicit hfBounds( const hfVec3 &mins, const hfVec3 &maxs );
					explicit hfBounds( const hfVec3 &point );

	const hfVec3 &	operator[]( const int index ) const;
	hfVec3 &		operator[]( const int index );
	hfBounds		operator+( const hfVec3 &t ) const;				// returns translated bounds
	hfBounds &		operator+=( const hfVec3 &t );					// translate the bounds
	hfBounds		operator*( const hfMat3 &r ) const;				// returns rotated bounds
	hfBounds &		operator*=( const hfMat3 &r );					// rotate the bounds
	hfBounds		operator+( const hfBounds &a ) const;
	hfBounds &		operator+=( const hfBounds &a );
	hfBounds		operator-( const hfBounds &a ) const;
	hfBounds &		operator-=( const hfBounds &a );

	bool			Compare( const hfBounds &a ) const;							// exact compare, no epsilon
	bool			Compare( const hfBounds &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfBounds &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfBounds &a ) const;						// exact compare, no epsilon

	void			Clear();									// inside out bounds
	void			Zero();									// single point at origin

	hfVec3			GetCenter() const;						// returns center of bounds
	float			GetRadius() const;						// returns the radius relative to the bounds origin
	float			GetRadius( const hfVec3 &center ) const;		// returns the radius relative to the given center
	float			GetVolume() const;						// returns the volume of the bounds
	bool			IsCleared() const;						// returns true if bounds are inside out

	bool			AddPoint( const hfVec3 &v );					// add the point, returns true if the bounds expanded
	bool			AddBounds( const hfBounds &a );					// add the bounds, returns true if the bounds expanded
	hfBounds		Intersect( const hfBounds &a ) const;			// return intersection of this bounds with the given bounds
	hfBounds &		IntersectSelf( const hfBounds &a );				// intersect this bounds with the given bounds
	hfBounds		Expand( const float d ) const;					// return bounds expanded in all directions with the given value
	hfBounds &		ExpandSelf( const float d );					// expand bounds in all directions with the given value
	hfBounds		Translate( const hfVec3 &translation ) const;	// return translated bounds
	hfBounds &		TranslateSelf( const hfVec3 &translation );		// translate this bounds
	hfBounds		Rotate( const hfMat3 &rotation ) const;			// return rotated bounds
	hfBounds &		RotateSelf( const hfMat3 &rotation );			// rotate this bounds

	float			PlaneDistance( const hfPlane &plane ) const;
	int				PlaneSide( const hfPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const hfVec3 &p ) const;			// includes touching
	bool			IntersectsBounds( const hfBounds &a ) const;	// includes touching
	bool			LineIntersection( const hfVec3 &start, const hfVec3 &end ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const hfVec3 &start, const hfVec3 &dir, float &scale ) const;

					// most tight bounds for the given transformed bounds
	void			FromTransformedBounds( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis );
					// most tight bounds for a point set
	void			FromPoints( const hfVec3 *points, const int numPoints );
					// most tight bounds for a translation
	void			FromPointTranslation( const hfVec3 &point, const hfVec3 &translation );
	void			FromBoundsTranslation( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis, const hfVec3 &translation );
					// most tight bounds for a rotation
	void			FromPointRotation( const hfVec3 &point, const hfRotation &rotation );
	void			FromBoundsRotation( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis, const hfRotation &rotation );

	void			ToPoints( hfVec3 points[8] ) const;
	hfSphere		ToSphere() const;

	void			AxisProjection( const hfVec3 &dir, float &min, float &max ) const;
	void			AxisProjection( const hfVec3 &origin, const hfMat3 &axis, const hfVec3 &dir, float &min, float &max ) const;

	int				GetDimension() const;

	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();

private:
	hfVec3			b[2];
};

extern hfBounds	bounds_zero;
extern hfBounds bounds_zeroOneCube;
extern hfBounds bounds_unitCube;

HF_INLINE hfBounds::hfBounds() {
}

HF_INLINE hfBounds::hfBounds( const hfVec3 &mins, const hfVec3 &maxs ) {
	b[0] = mins;
	b[1] = maxs;
}

HF_INLINE hfBounds::hfBounds( const hfVec3 &point ) {
	b[0] = point;
	b[1] = point;
}

HF_INLINE const hfVec3 &hfBounds::operator[]( const int index ) const {
	return b[index];
}

HF_INLINE hfVec3 &hfBounds::operator[]( const int index ) {
	return b[index];
}

HF_INLINE hfBounds hfBounds::operator+( const hfVec3 &t ) const {
	return hfBounds( b[0] + t, b[1] + t );
}

HF_INLINE hfBounds &hfBounds::operator+=( const hfVec3 &t ) {
	b[0] += t;
	b[1] += t;
	return *this;
}

HF_INLINE hfBounds hfBounds::operator*( const hfMat3 &r ) const {
	hfBounds bounds;
	bounds.FromTransformedBounds( *this, vec3_origin, r );
	return bounds;
}

HF_INLINE hfBounds &hfBounds::operator*=( const hfMat3 &r ) {
	this->FromTransformedBounds( *this, vec3_origin, r );
	return *this;
}

HF_INLINE hfBounds hfBounds::operator+( const hfBounds &a ) const {
	hfBounds newBounds;
	newBounds = *this;
	newBounds.AddBounds( a );
	return newBounds;
}

HF_INLINE hfBounds &hfBounds::operator+=( const hfBounds &a ) {
	hfBounds::AddBounds( a );
	return *this;
}

HF_INLINE hfBounds hfBounds::operator-( const hfBounds &a ) const {
	assert( b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
				b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
					b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2] );
	return hfBounds( hfVec3( b[0][0] + a.b[1][0], b[0][1] + a.b[1][1], b[0][2] + a.b[1][2] ),
					hfVec3( b[1][0] + a.b[0][0], b[1][1] + a.b[0][1], b[1][2] + a.b[0][2] ) );
}

HF_INLINE hfBounds &hfBounds::operator-=( const hfBounds &a ) {
	assert( b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
				b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
					b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2] );
	b[0] += a.b[1];
	b[1] += a.b[0];
	return *this;
}

HF_INLINE bool hfBounds::Compare( const hfBounds &a ) const {
	return ( b[0].Compare( a.b[0] ) && b[1].Compare( a.b[1] ) );
}

HF_INLINE bool hfBounds::Compare( const hfBounds &a, const float epsilon ) const {
	return ( b[0].Compare( a.b[0], epsilon ) && b[1].Compare( a.b[1], epsilon ) );
}

HF_INLINE bool hfBounds::operator==( const hfBounds &a ) const {
	return Compare( a );
}

HF_INLINE bool hfBounds::operator!=( const hfBounds &a ) const {
	return !Compare( a );
}

HF_INLINE void hfBounds::Clear() {
	b[0][0] = b[0][1] = b[0][2] = hfMath::INFINITY;
	b[1][0] = b[1][1] = b[1][2] = -hfMath::INFINITY;
}

HF_INLINE void hfBounds::Zero() {
	b[0][0] = b[0][1] = b[0][2] =
	b[1][0] = b[1][1] = b[1][2] = 0;
}

HF_INLINE hfVec3 hfBounds::GetCenter() const {
	return hfVec3( ( b[1][0] + b[0][0] ) * 0.5f, ( b[1][1] + b[0][1] ) * 0.5f, ( b[1][2] + b[0][2] ) * 0.5f );
}

HF_INLINE float hfBounds::GetVolume() const {
	if ( b[0][0] >= b[1][0] || b[0][1] >= b[1][1] || b[0][2] >= b[1][2] ) {
		return 0.0f;
	}
	return ( ( b[1][0] - b[0][0] ) * ( b[1][1] - b[0][1] ) * ( b[1][2] - b[0][2] ) );
}

HF_INLINE bool hfBounds::IsCleared() const {
	return b[0][0] > b[1][0];
}

HF_INLINE bool hfBounds::AddPoint( const hfVec3 &v ) {
	bool expanded = false;
	if ( v[0] < b[0][0]) {
		b[0][0] = v[0];
		expanded = true;
	}
	if ( v[0] > b[1][0]) {
		b[1][0] = v[0];
		expanded = true;
	}
	if ( v[1] < b[0][1] ) {
		b[0][1] = v[1];
		expanded = true;
	}
	if ( v[1] > b[1][1]) {
		b[1][1] = v[1];
		expanded = true;
	}
	if ( v[2] < b[0][2] ) {
		b[0][2] = v[2];
		expanded = true;
	}
	if ( v[2] > b[1][2]) {
		b[1][2] = v[2];
		expanded = true;
	}
	return expanded;
}

HF_INLINE bool hfBounds::AddBounds( const hfBounds &a ) {
	bool expanded = false;
	if ( a.b[0][0] < b[0][0] ) {
		b[0][0] = a.b[0][0];
		expanded = true;
	}
	if ( a.b[0][1] < b[0][1] ) {
		b[0][1] = a.b[0][1];
		expanded = true;
	}
	if ( a.b[0][2] < b[0][2] ) {
		b[0][2] = a.b[0][2];
		expanded = true;
	}
	if ( a.b[1][0] > b[1][0] ) {
		b[1][0] = a.b[1][0];
		expanded = true;
	}
	if ( a.b[1][1] > b[1][1] ) {
		b[1][1] = a.b[1][1];
		expanded = true;
	}
	if ( a.b[1][2] > b[1][2] ) {
		b[1][2] = a.b[1][2];
		expanded = true;
	}
	return expanded;
}

HF_INLINE hfBounds hfBounds::Intersect( const hfBounds &a ) const {
	hfBounds n;
	n.b[0][0] = ( a.b[0][0] > b[0][0] ) ? a.b[0][0] : b[0][0];
	n.b[0][1] = ( a.b[0][1] > b[0][1] ) ? a.b[0][1] : b[0][1];
	n.b[0][2] = ( a.b[0][2] > b[0][2] ) ? a.b[0][2] : b[0][2];
	n.b[1][0] = ( a.b[1][0] < b[1][0] ) ? a.b[1][0] : b[1][0];
	n.b[1][1] = ( a.b[1][1] < b[1][1] ) ? a.b[1][1] : b[1][1];
	n.b[1][2] = ( a.b[1][2] < b[1][2] ) ? a.b[1][2] : b[1][2];
	return n;
}

HF_INLINE hfBounds &hfBounds::IntersectSelf( const hfBounds &a ) {
	if ( a.b[0][0] > b[0][0] ) {
		b[0][0] = a.b[0][0];
	}
	if ( a.b[0][1] > b[0][1] ) {
		b[0][1] = a.b[0][1];
	}
	if ( a.b[0][2] > b[0][2] ) {
		b[0][2] = a.b[0][2];
	}
	if ( a.b[1][0] < b[1][0] ) {
		b[1][0] = a.b[1][0];
	}
	if ( a.b[1][1] < b[1][1] ) {
		b[1][1] = a.b[1][1];
	}
	if ( a.b[1][2] < b[1][2] ) {
		b[1][2] = a.b[1][2];
	}
	return *this;
}

HF_INLINE hfBounds hfBounds::Expand( const float d ) const {
	return hfBounds( hfVec3( b[0][0] - d, b[0][1] - d, b[0][2] - d ),
						hfVec3( b[1][0] + d, b[1][1] + d, b[1][2] + d ) );
}

HF_INLINE hfBounds &hfBounds::ExpandSelf( const float d ) {
	b[0][0] -= d;
	b[0][1] -= d;
	b[0][2] -= d;
	b[1][0] += d;
	b[1][1] += d;
	b[1][2] += d;
	return *this;
}

HF_INLINE hfBounds hfBounds::Translate( const hfVec3 &translation ) const {
	return hfBounds( b[0] + translation, b[1] + translation );
}

HF_INLINE hfBounds &hfBounds::TranslateSelf( const hfVec3 &translation ) {
	b[0] += translation;
	b[1] += translation;
	return *this;
}

HF_INLINE hfBounds hfBounds::Rotate( const hfMat3 &rotation ) const {
	hfBounds bounds;
	bounds.FromTransformedBounds( *this, vec3_origin, rotation );
	return bounds;
}

HF_INLINE hfBounds &hfBounds::RotateSelf( const hfMat3 &rotation ) {
	FromTransformedBounds( *this, vec3_origin, rotation );
	return *this;
}

HF_INLINE bool hfBounds::ContainsPoint( const hfVec3 &p ) const {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2]
		|| p[0] > b[1][0] || p[1] > b[1][1] || p[2] > b[1][2] ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfBounds::IntersectsBounds( const hfBounds &a ) const {
	if ( a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2]
		|| a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2] ) {
		return false;
	}
	return true;
}

HF_INLINE hfSphere hfBounds::ToSphere() const {
	hfSphere sphere;
	sphere.SetOrigin( ( b[0] + b[1] ) * 0.5f );
	sphere.SetRadius( ( b[1] - sphere.GetOrigin() ).Length() );
	return sphere;
}

HF_INLINE void hfBounds::AxisProjection( const hfVec3 &dir, float &min, float &max ) const {
	float d1, d2;
	hfVec3 center, extents;

	center = ( b[0] + b[1] ) * 0.5f;
	extents = b[1] - center;

	d1 = dir * center;
	d2 = hfMath::Fabs( extents[0] * dir[0] ) +
			hfMath::Fabs( extents[1] * dir[1] ) +
				hfMath::Fabs( extents[2] * dir[2] );

	min = d1 - d2;
	max = d1 + d2;
}

HF_INLINE void hfBounds::AxisProjection( const hfVec3 &origin, const hfMat3 &axis, const hfVec3 &dir, float &min, float &max ) const {
	float d1, d2;
	hfVec3 center, extents;

	center = ( b[0] + b[1] ) * 0.5f;
	extents = b[1] - center;
	center = origin + center * axis;

	d1 = dir * center;
	d2 = hfMath::Fabs( extents[0] * ( dir * axis[0] ) ) +
			hfMath::Fabs( extents[1] * ( dir * axis[1] ) ) +
				hfMath::Fabs( extents[2] * ( dir * axis[2] ) );

	min = d1 - d2;
	max = d1 + d2;
}

HF_INLINE int hfBounds::GetDimension() const {
	return 6;
}

HF_INLINE const float *hfBounds::ToFloatPtr() const {
	return &b[0].x;
}

HF_INLINE float *hfBounds::ToFloatPtr() {
	return &b[0].x;
}

#endif /* !__BV_BOUNDS_H__ */

