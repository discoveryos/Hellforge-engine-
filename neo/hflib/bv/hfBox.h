#ifndef __BV_BOX_H__
#define __BV_BOX_H__

/*
===============================================================================

	Oriented Bounding Box

===============================================================================
*/

class hfBox {
public:
					hfBox();
					explicit hfBox( const hfVec3 &center, const hfVec3 &extents, const hfMat3 &axis );
					explicit hfBox( const hfVec3 &point );
					explicit hfBox( const hfBounds &bounds );
					explicit hfBox( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis );

	hfBox			operator+( const hfVec3 &t ) const;				// returns translated box
	hfBox &			operator+=( const hfVec3 &t );					// translate the box
	hfBox			operator*( const hfMat3 &r ) const;				// returns rotated box
	hfBox &			operator*=( const hfMat3 &r );					// rotate the box
	hfBox			operator+( const hfBox &a ) const;
	hfBox &			operator+=( const hfBox &a );
	hfBox			operator-( const hfBox &a ) const;
	hfBox &			operator-=( const hfBox &a );

	bool			Compare( const hfBox &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfBox &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfBox &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfBox &a ) const;						// exact compare, no epsilon

	void			Clear();									// inside out box
	void			Zero();									// single point at origin

	const hfVec3 &	GetCenter() const;						// returns center of the box
	const hfVec3 &	GetExtents() const;						// returns extents of the box
	const hfMat3 &	GetAxis() const;							// returns the axis of the box
	float			GetVolume() const;						// returns the volume of the box
	bool			IsCleared() const;						// returns true if box are inside out

	bool			AddPoint( const hfVec3 &v );					// add the point, returns true if the box expanded
	bool			AddBox( const hfBox &a );						// add the box, returns true if the box expanded
	hfBox			Expand( const float d ) const;					// return box expanded in all directions with the given value
	hfBox &			ExpandSelf( const float d );					// expand box in all directions with the given value
	hfBox			Translate( const hfVec3 &translation ) const;	// return translated box
	hfBox &			TranslateSelf( const hfVec3 &translation );		// translate this box
	hfBox			Rotate( const hfMat3 &rotation ) const;			// return rotated box
	hfBox &			RotateSelf( const hfMat3 &rotation );			// rotate this box

	float			PlaneDistance( const hfPlane &plane ) const;
	int				PlaneSide( const hfPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const hfVec3 &p ) const;			// includes touching
	bool			IntersectsBox( const hfBox &a ) const;			// includes touching
	bool			LineIntersection( const hfVec3 &start, const hfVec3 &end ) const;
					// intersection points are (start + dir * scale1) and (start + dir * scale2)
	bool			RayIntersection( const hfVec3 &start, const hfVec3 &dir, float &scale1, float &scale2 ) const;

					// tight box for a collection of points
	void			FromPoints( const hfVec3 *points, const int numPoints );
					// most tight box for a translation
	void			FromPointTranslation( const hfVec3 &point, const hfVec3 &translation );
	void			FromBoxTranslation( const hfBox &box, const hfVec3 &translation );
					// most tight box for a rotation
	void			FromPointRotation( const hfVec3 &point, const hfRotation &rotation );
	void			FromBoxRotation( const hfBox &box, const hfRotation &rotation );

	void			ToPoints( hfVec3 points[8] ) const;
	hfSphere		ToSphere() const;

					// calculates the projection of this box onto the given axis
	void			AxisProjection( const hfVec3 &dir, float &min, float &max ) const;
	void			AxisProjection( const hfMat3 &ax, hfBounds &bounds ) const;

					// calculates the silhouette of the box
	int				GetProjectionSilhouetteVerts( const hfVec3 &projectionOrigin, hfVec3 silVerts[6] ) const;
	int				GetParallelProjectionSilhouetteVerts( const hfVec3 &projectionDir, hfVec3 silVerts[6] ) const;

private:
	hfVec3			center;
	hfVec3			extents;
	hfMat3			axis;
};

extern hfBox	box_zero;

HF_INLINE hfBox::hfBox() {
}

HF_INLINE hfBox::hfBox( const hfVec3 &center, const hfVec3 &extents, const hfMat3 &axis ) {
	this->center = center;
	this->extents = extents;
	this->axis = axis;
}

HF_INLINE hfBox::hfBox( const hfVec3 &point ) {
	this->center = point;
	this->extents.Zero();
	this->axis.Identity();
}

HF_INLINE hfBox::hfBox( const hfBounds &bounds ) {
	this->center = ( bounds[0] + bounds[1] ) * 0.5f;
	this->extents = bounds[1] - this->center;
	this->axis.Identity();
}

HF_INLINE hfBox::hfBox( const hfBounds &bounds, const hfVec3 &origin, const hfMat3 &axis ) {
	this->center = ( bounds[0] + bounds[1] ) * 0.5f;
	this->extents = bounds[1] - this->center;
	this->center = origin + this->center * axis;
	this->axis = axis;
}

HF_INLINE hfBox hfBox::operator+( const hfVec3 &t ) const {
	return hfBox( center + t, extents, axis );
}

HF_INLINE hfBox &hfBox::operator+=( const hfVec3 &t ) {
	center += t;
	return *this;
}

HF_INLINE hfBox hfBox::operator*( const hfMat3 &r ) const {
	return hfBox( center * r, extents, axis * r );
}

HF_INLINE hfBox &hfBox::operator*=( const hfMat3 &r ) {
	center *= r;
	axis *= r;
	return *this;
}

HF_INLINE hfBox hfBox::operator+( const hfBox &a ) const {
	hfBox newBox;
	newBox = *this;
	newBox.AddBox( a );
	return newBox;
}

HF_INLINE hfBox &hfBox::operator+=( const hfBox &a ) {
	hfBox::AddBox( a );
	return *this;
}

HF_INLINE hfBox hfBox::operator-( const hfBox &a ) const {
	return hfBox( center, extents - a.extents, axis );
}

HF_INLINE hfBox &hfBox::operator-=( const hfBox &a ) {
	extents -= a.extents;
	return *this;
}

HF_INLINE bool hfBox::Compare( const hfBox &a ) const {
	return ( center.Compare( a.center ) && extents.Compare( a.extents ) && axis.Compare( a.axis ) );
}

HF_INLINE bool hfBox::Compare( const hfBox &a, const float epsilon ) const {
	return ( center.Compare( a.center, epsilon ) && extents.Compare( a.extents, epsilon ) && axis.Compare( a.axis, epsilon ) );
}

HF_INLINE bool hfBox::operator==( const hfBox &a ) const {
	return Compare( a );
}

HF_INLINE bool hfBox::operator!=( const hfBox &a ) const {
	return !Compare( a );
}

HF_INLINE void hfBox::Clear() {
	center.Zero();
	extents[0] = extents[1] = extents[2] = -hfMath::INFINITY;
	axis.Identity();
}

HF_INLINE void hfBox::Zero() {
	center.Zero();
	extents.Zero();
	axis.Identity();
}

HF_INLINE const hfVec3 &hfBox::GetCenter() const {
	return center;
}

HF_INLINE const hfVec3 &hfBox::GetExtents() const {
	return extents;
}

HF_INLINE const hfMat3 &hfBox::GetAxis() const {
	return axis;
}

HF_INLINE float hfBox::GetVolume() const {
	return ( extents * 2.0f ).LengthSqr();
}

HF_INLINE bool hfBox::IsCleared() const {
	return extents[0] < 0.0f;
}

HF_INLINE hfBox hfBox::Expand( const float d ) const {
	return hfBox( center, extents + hfVec3( d, d, d ), axis );
}

HF_INLINE hfBox &hfBox::ExpandSelf( const float d ) {
	extents[0] += d;
	extents[1] += d;
	extents[2] += d;
	return *this;
}

HF_INLINE hfBox hfBox::Translate( const hfVec3 &translation ) const {
	return hfBox( center + translation, extents, axis );
}

HF_INLINE hfBox &hfBox::TranslateSelf( const hfVec3 &translation ) {
	center += translation;
	return *this;
}

HF_INLINE hfBox hfBox::Rotate( const hfMat3 &rotation ) const {
	return hfBox( center * rotation, extents, axis * rotation );
}

HF_INLINE hfBox &hfBox::RotateSelf( const hfMat3 &rotation ) {
	center *= rotation;
	axis *= rotation;
	return *this;
}

HF_INLINE bool hfBox::ContainsPoint( const hfVec3 &p ) const {
	hfVec3 lp = p - center;
	if ( hfMath::Fabs( lp * axis[0] ) > extents[0] ||
			hfMath::Fabs( lp * axis[1] ) > extents[1] ||
				hfMath::Fabs( lp * axis[2] ) > extents[2] ) {
		return false;
	}
	return true;
}

HF_INLINE hfSphere hfBox::ToSphere() const {
	return hfSphere( center, extents.Length() );
}

HF_INLINE void hfBox::AxisProjection( const hfVec3 &dir, float &min, float &max ) const {
	float d1 = dir * center;
	float d2 = hfMath::Fabs( extents[0] * ( dir * axis[0] ) ) +
				hfMath::Fabs( extents[1] * ( dir * axis[1] ) ) +
				hfMath::Fabs( extents[2] * ( dir * axis[2] ) );
	min = d1 - d2;
	max = d1 + d2;
}

HF_INLINE void hfBox::AxisProjection( const hfMat3 &ax, hfBounds &bounds ) const {
	for ( int i = 0; i < 3; i++ ) {
		float d1 = ax[i] * center;
		float d2 = hfMath::Fabs( extents[0] * ( ax[i] * axis[0] ) ) +
					hfMath::Fabs( extents[1] * ( ax[i] * axis[1] ) ) +
					hfMath::Fabs( extents[2] * ( ax[i] * axis[2] ) );
		bounds[0][i] = d1 - d2;
		bounds[1][i] = d1 + d2;
	}
}

#endif /* !__BV_BOX_H__ */

