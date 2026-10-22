#ifndef __BV_SPHERE_H__
#define __BV_SPHERE_H__

/*
===============================================================================

	Sphere

===============================================================================
*/

class hfSphere {
public:
					hfSphere();
					explicit hfSphere( const hfVec3 &point );
					explicit hfSphere( const hfVec3 &point, const float r );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	hfSphere		operator+( const hfVec3 &t ) const;				// returns tranlated sphere
	hfSphere &		operator+=( const hfVec3 &t );					// translate the sphere
	hfSphere		operator+( const hfSphere &s ) const;
	hfSphere &		operator+=( const hfSphere &s );

	bool			Compare( const hfSphere &a ) const;							// exact compare, no epsilon
	bool			Compare( const hfSphere &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfSphere &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfSphere &a ) const;						// exact compare, no epsilon

	void			Clear();									// inside out sphere
	void			Zero();									// single point at origin
	void			SetOrigin( const hfVec3 &o );					// set origin of sphere
	void			SetRadius( const float r );						// set square radius

	const hfVec3 &	GetOrigin() const;						// returns origin of sphere
	float			GetRadius() const;						// returns sphere radius
	bool			IsCleared() const;						// returns true if sphere is inside out

	bool			AddPoint( const hfVec3 &p );					// add the point, returns true if the sphere expanded
	bool			AddSphere( const hfSphere &s );					// add the sphere, returns true if the sphere expanded
	hfSphere		Expand( const float d ) const;					// return bounds expanded in all directions with the given value
	hfSphere &		ExpandSelf( const float d );					// expand bounds in all directions with the given value
	hfSphere		Translate( const hfVec3 &translation ) const;
	hfSphere &		TranslateSelf( const hfVec3 &translation );

	float			PlaneDistance( const hfPlane &plane ) const;
	int				PlaneSide( const hfPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const hfVec3 &p ) const;			// includes touching
	bool			IntersectsSphere( const hfSphere &s ) const;	// includes touching
	bool			LineIntersection( const hfVec3 &start, const hfVec3 &end ) const;
					// intersection points are (start + dir * scale1) and (start + dir * scale2)
	bool			RayIntersection( const hfVec3 &start, const hfVec3 &dir, float &scale1, float &scale2 ) const;

					// Tight sphere for a point set.
	void			FromPoints( const hfVec3 *points, const int numPoints );
					// Most tight sphere for a translation.
	void			FromPointTranslation( const hfVec3 &point, const hfVec3 &translation );
	void			FromSphereTranslation( const hfSphere &sphere, const hfVec3 &start, const hfVec3 &translation );
					// Most tight sphere for a rotation.
	void			FromPointRotation( const hfVec3 &point, const hfRotation &rotation );
	void			FromSphereRotation( const hfSphere &sphere, const hfVec3 &start, const hfRotation &rotation );

	void			AxisProjection( const hfVec3 &dir, float &min, float &max ) const;

private:
	hfVec3			origin;
	float			radius;
};

extern hfSphere	sphere_zero;

HF_INLINE hfSphere::hfSphere() {
}

HF_INLINE hfSphere::hfSphere( const hfVec3 &point ) {
	origin = point;
	radius = 0.0f;
}

HF_INLINE hfSphere::hfSphere( const hfVec3 &point, const float r ) {
	origin = point;
	radius = r;
}

HF_INLINE float hfSphere::operator[]( const int index ) const {
	return ((float *) &origin)[index];
}

HF_INLINE float &hfSphere::operator[]( const int index ) {
	return ((float *) &origin)[index];
}

HF_INLINE hfSphere hfSphere::operator+( const hfVec3 &t ) const {
	return hfSphere( origin + t, radius );
}

HF_INLINE hfSphere &hfSphere::operator+=( const hfVec3 &t ) {
	origin += t;
	return *this;
}

HF_INLINE bool hfSphere::Compare( const hfSphere &a ) const {
	return ( origin.Compare( a.origin ) && radius == a.radius );
}

HF_INLINE bool hfSphere::Compare( const hfSphere &a, const float epsilon ) const {
	return ( origin.Compare( a.origin, epsilon ) && hfMath::Fabs( radius - a.radius ) <= epsilon );
}

HF_INLINE bool hfSphere::operator==( const hfSphere &a ) const {
	return Compare( a );
}

HF_INLINE bool hfSphere::operator!=( const hfSphere &a ) const {
	return !Compare( a );
}

HF_INLINE void hfSphere::Clear() {
	origin.Zero();
	radius = -1.0f;
}

HF_INLINE void hfSphere::Zero() {
	origin.Zero();
	radius = 0.0f;
}

HF_INLINE void hfSphere::SetOrigin( const hfVec3 &o ) {
	origin = o;
}

HF_INLINE void hfSphere::SetRadius( const float r ) {
	radius = r;
}

HF_INLINE const hfVec3 &hfSphere::GetOrigin() const {
	return origin;
}

HF_INLINE float hfSphere::GetRadius() const {
	return radius;
}

HF_INLINE bool hfSphere::IsCleared() const {
	return ( radius < 0.0f );
}

HF_INLINE bool hfSphere::AddPoint( const hfVec3 &p ) {
	if ( radius < 0.0f ) {
		origin = p;
		radius = 0.0f;
		return true;
	}
	else {
		float r = ( p - origin ).LengthSqr();
		if ( r > radius * radius ) {
			r = hfMath::Sqrt( r );
			origin += ( p - origin ) * 0.5f * (1.0f - radius / r );
			radius += 0.5f * ( r - radius );
			return true;
		}
		return false;
	}
}

HF_INLINE bool hfSphere::AddSphere( const hfSphere &s ) {
	if ( radius < 0.0f ) {
		origin = s.origin;
		radius = s.radius;
		return true;
	}
	else {
		float r = ( s.origin - origin ).LengthSqr();
		if ( r > ( radius + s.radius ) * ( radius + s.radius ) ) {
			r = hfMath::Sqrt( r );
			origin += ( s.origin - origin ) * 0.5f * (1.0f - radius / ( r + s.radius ) );
			radius += 0.5f * ( ( r + s.radius ) - radius );
			return true;
		}
		return false;
	}
}

HF_INLINE hfSphere hfSphere::Expand( const float d ) const {
	return hfSphere( origin, radius + d );
}

HF_INLINE hfSphere &hfSphere::ExpandSelf( const float d ) {
	radius += d;
	return *this;
}

HF_INLINE hfSphere hfSphere::Translate( const hfVec3 &translation ) const {
	return hfSphere( origin + translation, radius );
}

HF_INLINE hfSphere &hfSphere::TranslateSelf( const hfVec3 &translation ) {
	origin += translation;
	return *this;
}

HF_INLINE bool hfSphere::ContainsPoint( const hfVec3 &p ) const {
	if ( ( p - origin ).LengthSqr() > radius * radius ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfSphere::IntersectsSphere( const hfSphere &s ) const {
	float r = s.radius + radius;
	if ( ( s.origin - origin ).LengthSqr() > r * r ) {
		return false;
	}
	return true;
}

HF_INLINE void hfSphere::FromPointTranslation( const hfVec3 &point, const hfVec3 &translation ) {
	origin = point + 0.5f * translation;
	radius = hfMath::Sqrt( 0.5f * translation.LengthSqr() );
}

HF_INLINE void hfSphere::FromSphereTranslation( const hfSphere &sphere, const hfVec3 &start, const hfVec3 &translation ) {
	origin = start + sphere.origin + 0.5f * translation;
	radius = hfMath::Sqrt( 0.5f * translation.LengthSqr() ) + sphere.radius;
}

HF_INLINE void hfSphere::FromPointRotation( const hfVec3 &point, const hfRotation &rotation ) {
	hfVec3 end = rotation * point;
	origin = ( point + end ) * 0.5f;
	radius = hfMath::Sqrt( 0.5f * ( end - point ).LengthSqr() );
}

HF_INLINE void hfSphere::FromSphereRotation( const hfSphere &sphere, const hfVec3 &start, const hfRotation &rotation ) {
	hfVec3 end = rotation * sphere.origin;
	origin = start + ( sphere.origin + end ) * 0.5f;
	radius = hfMath::Sqrt( 0.5f * ( end - sphere.origin ).LengthSqr() ) + sphere.radius;
}

HF_INLINE void hfSphere::AxisProjection( const hfVec3 &dir, float &min, float &max ) const {
	float d;
	d = dir * origin;
	min = d - radius;
	max = d + radius;
}

#endif /* !__BV_SPHERE_H__ */

