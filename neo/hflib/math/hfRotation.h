#ifndef __MATH_ROTATION_H__
#define __MATH_ROTATION_H__

/*
===============================================================================

	Describes a complete rotation in degrees about an abritray axis.
	A local rotation matrix is stored for fast rotation of multiple points.

===============================================================================
*/


class hfAngles;
class hfQuat;
class hfMat3;

class hfRotation {

	friend class hfAngles;
	friend class hfQuat;
	friend class hfMat3;

public:
						hfRotation();
						hfRotation( const hfVec3 &rotationOrigin, const hfVec3 &rotationVec, const float rotationAngle );

	void				Set( const hfVec3 &rotationOrigin, const hfVec3 &rotationVec, const float rotationAngle );
	void				SetOrigin( const hfVec3 &rotationOrigin );
	void				SetVec( const hfVec3 &rotationVec );					// has to be normalized
	void				SetVec( const float x, const float y, const float z );	// has to be normalized
	void				SetAngle( const float rotationAngle );
	void				Scale( const float s );
	void				ReCalculateMatrix();
	const hfVec3 &		GetOrigin() const;
	const hfVec3 &		GetVec() const;
	float				GetAngle() const;

	hfRotation			operator-() const;										// flips rotation
	hfRotation			operator*( const float s ) const;						// scale rotation
	hfRotation			operator/( const float s ) const;						// scale rotation
	hfRotation &		operator*=( const float s );							// scale rotation
	hfRotation &		operator/=( const float s );							// scale rotation
	hfVec3				operator*( const hfVec3 &v ) const;						// rotate vector

	friend hfRotation	operator*( const float s, const hfRotation &r );		// scale rotation
	friend hfVec3		operator*( const hfVec3 &v, const hfRotation &r );		// rotate vector
	friend hfVec3 &		operator*=( hfVec3 &v, const hfRotation &r );			// rotate vector

	hfAngles			ToAngles() const;
	hfQuat				ToQuat() const;
	const hfMat3 &		ToMat3() const;
	hfMat4				ToMat4() const;
	hfVec3				ToAngularVelocity() const;

	void				RotatePoint( hfVec3 &point ) const;

	void				Normalize180();
	void				Normalize360();

private:
	hfVec3				origin;			// origin of rotation
	hfVec3				vec;			// normalized vector to rotate around
	float				angle;			// angle of rotation in degrees
	mutable hfMat3		axis;			// rotation axis
	mutable bool		axisValid;		// true if rotation axis is valid
};


HF_INLINE hfRotation::hfRotation() {
}

HF_INLINE hfRotation::hfRotation( const hfVec3 &rotationOrigin, const hfVec3 &rotationVec, const float rotationAngle ) {
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

HF_INLINE void hfRotation::Set( const hfVec3 &rotationOrigin, const hfVec3 &rotationVec, const float rotationAngle ) {
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

HF_INLINE void hfRotation::SetOrigin( const hfVec3 &rotationOrigin ) {
	origin = rotationOrigin;
}

HF_INLINE void hfRotation::SetVec( const hfVec3 &rotationVec ) {
	vec = rotationVec;
	axisValid = false;
}

HF_INLINE void hfRotation::SetVec( float x, float y, float z ) {
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
	axisValid = false;
}

HF_INLINE void hfRotation::SetAngle( const float rotationAngle ) {
	angle = rotationAngle;
	axisValid = false;
}

HF_INLINE void hfRotation::Scale( const float s ) {
	angle *= s;
	axisValid = false;
}

HF_INLINE void hfRotation::ReCalculateMatrix() {
	axisValid = false;
	ToMat3();
}

HF_INLINE const hfVec3 &hfRotation::GetOrigin() const {
	return origin;
}

HF_INLINE const hfVec3 &hfRotation::GetVec() const  {
	return vec;
}

HF_INLINE float hfRotation::GetAngle() const  {
	return angle;
}

HF_INLINE hfRotation hfRotation::operator-() const {
	return hfRotation( origin, vec, -angle );
}

HF_INLINE hfRotation hfRotation::operator*( const float s ) const {
	return hfRotation( origin, vec, angle * s );
}

HF_INLINE hfRotation hfRotation::operator/( const float s ) const {
	assert( s != 0.0f );
	return hfRotation( origin, vec, angle / s );
}

HF_INLINE hfRotation &hfRotation::operator*=( const float s ) {
	angle *= s;
	axisValid = false;
	return *this;
}

HF_INLINE hfRotation &hfRotation::operator/=( const float s ) {
	assert( s != 0.0f );
	angle /= s;
	axisValid = false;
	return *this;
}

HF_INLINE hfVec3 hfRotation::operator*( const hfVec3 &v ) const {
	if ( !axisValid ) {
		ToMat3();
	}
	return ((v - origin) * axis + origin);
}

HF_INLINE hfRotation operator*( const float s, const hfRotation &r ) {
	return r * s;
}

HF_INLINE hfVec3 operator*( const hfVec3 &v, const hfRotation &r ) {
	return r * v;
}

HF_INLINE hfVec3 &operator*=( hfVec3 &v, const hfRotation &r ) {
	v = r * v;
	return v;
}

HF_INLINE void hfRotation::RotatePoint( hfVec3 &point ) const {
	if ( !axisValid ) {
		ToMat3();
	}
	point = ((point - origin) * axis + origin);
}

#endif /* !__MATH_ROTATION_H__ */

