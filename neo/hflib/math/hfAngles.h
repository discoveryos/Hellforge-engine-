#ifndef __MATH_ANGLES_H__
#define __MATH_ANGLES_H__

/*
===============================================================================

	Euler angles

===============================================================================
*/

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

class hfVec3;
class hfQuat;
class hfRotation;
class hfMat3;
class hfMat4;

class hfAngles {
public:
	float			pitch;
	float			yaw;
	float			roll;

					hfAngles();
					hfAngles( float pitch, float yaw, float roll );
					explicit hfAngles( const hfVec3 &v );

	void 			Set( float pitch, float yaw, float roll );
	hfAngles &		Zero();

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	hfAngles		operator-() const;			// negate angles, in general not the inverse rotation
	hfAngles &		operator=( const hfAngles &a );
	hfAngles		operator+( const hfAngles &a ) const;
	hfAngles &		operator+=( const hfAngles &a );
	hfAngles		operator-( const hfAngles &a ) const;
	hfAngles &		operator-=( const hfAngles &a );
	hfAngles		operator*( const float a ) const;
	hfAngles &		operator*=( const float a );
	hfAngles		operator/( const float a ) const;
	hfAngles &		operator/=( const float a );

	friend hfAngles	operator*( const float a, const hfAngles &b );

	bool			Compare( const hfAngles &a ) const;							// exact compare, no epsilon
	bool			Compare( const hfAngles &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfAngles &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const hfAngles &a ) const;						// exact compare, no epsilon

	hfAngles &		Normalize360();	// normalizes 'this'
	hfAngles &		Normalize180();	// normalizes 'this'

	void			Clamp( const hfAngles &min, const hfAngles &max );

	int				GetDimension() const;

	void			ToVectors( hfVec3 *forward, hfVec3 *right = NULL, hfVec3 *up = NULL ) const;
	hfVec3			ToForward() const;
	hfQuat			ToQuat() const;
	hfRotation		ToRotation() const;
	hfMat3			ToMat3() const;
	hfMat4			ToMat4() const;
	hfVec3			ToAngularVelocity() const;
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;
};

extern hfAngles ang_zero;

HF_INLINE hfAngles::hfAngles() {
}

HF_INLINE hfAngles::hfAngles( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

HF_INLINE hfAngles::hfAngles( const hfVec3 &v ) {
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

HF_INLINE void hfAngles::Set( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

HF_INLINE hfAngles &hfAngles::Zero() {
	pitch = yaw = roll = 0.0f;
	return *this;
}

HF_INLINE float hfAngles::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

HF_INLINE float &hfAngles::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

HF_INLINE hfAngles hfAngles::operator-() const {
	return hfAngles( -pitch, -yaw, -roll );
}

HF_INLINE hfAngles &hfAngles::operator=( const hfAngles &a ) {
	pitch	= a.pitch;
	yaw		= a.yaw;
	roll	= a.roll;
	return *this;
}

HF_INLINE hfAngles hfAngles::operator+( const hfAngles &a ) const {
	return hfAngles( pitch + a.pitch, yaw + a.yaw, roll + a.roll );
}

HF_INLINE hfAngles& hfAngles::operator+=( const hfAngles &a ) {
	pitch	+= a.pitch;
	yaw		+= a.yaw;
	roll	+= a.roll;

	return *this;
}

HF_INLINE hfAngles hfAngles::operator-( const hfAngles &a ) const {
	return hfAngles( pitch - a.pitch, yaw - a.yaw, roll - a.roll );
}

HF_INLINE hfAngles& hfAngles::operator-=( const hfAngles &a ) {
	pitch	-= a.pitch;
	yaw		-= a.yaw;
	roll	-= a.roll;

	return *this;
}

HF_INLINE hfAngles hfAngles::operator*( const float a ) const {
	return hfAngles( pitch * a, yaw * a, roll * a );
}

HF_INLINE hfAngles& hfAngles::operator*=( float a ) {
	pitch	*= a;
	yaw		*= a;
	roll	*= a;
	return *this;
}

HF_INLINE hfAngles hfAngles::operator/( const float a ) const {
	float inva = 1.0f / a;
	return hfAngles( pitch * inva, yaw * inva, roll * inva );
}

HF_INLINE hfAngles& hfAngles::operator/=( float a ) {
	float inva = 1.0f / a;
	pitch	*= inva;
	yaw		*= inva;
	roll	*= inva;
	return *this;
}

HF_INLINE hfAngles operator*( const float a, const hfAngles &b ) {
	return hfAngles( a * b.pitch, a * b.yaw, a * b.roll );
}

HF_INLINE bool hfAngles::Compare( const hfAngles &a ) const {
	return ( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

HF_INLINE bool hfAngles::Compare( const hfAngles &a, const float epsilon ) const {
	if ( hfMath::Fabs( pitch - a.pitch ) > epsilon ) {
		return false;
	}
			
	if ( hfMath::Fabs( yaw - a.yaw ) > epsilon ) {
		return false;
	}

	if ( hfMath::Fabs( roll - a.roll ) > epsilon ) {
		return false;
	}

	return true;
}

HF_INLINE bool hfAngles::operator==( const hfAngles &a ) const {
	return Compare( a );
}

HF_INLINE bool hfAngles::operator!=( const hfAngles &a ) const {
	return !Compare( a );
}

HF_INLINE void hfAngles::Clamp( const hfAngles &min, const hfAngles &max ) {
	if ( pitch < min.pitch ) {
		pitch = min.pitch;
	} else if ( pitch > max.pitch ) {
		pitch = max.pitch;
	}
	if ( yaw < min.yaw ) {
		yaw = min.yaw;
	} else if ( yaw > max.yaw ) {
		yaw = max.yaw;
	}
	if ( roll < min.roll ) {
		roll = min.roll;
	} else if ( roll > max.roll ) {
		roll = max.roll;
	}
}

HF_INLINE int hfAngles::GetDimension() const {
	return 3;
}

HF_INLINE const float *hfAngles::ToFloatPtr() const {
	return &pitch;
}

HF_INLINE float *hfAngles::ToFloatPtr() {
	return &pitch;
}

#endif /* !__MATH_ANGLES_H__ */

