#ifndef __JOINTTRANSFORM_H__
#define __JOINTTRANSFORM_H__

/*
===============================================================================

  Joint Quaternion

===============================================================================
*/

class hfJointQuat {
public:
	const float *	ToFloatPtr() const { return q.ToFloatPtr(); }
	float *			ToFloatPtr() { return q.ToFloatPtr(); }

	hfQuat			q;
	hfVec3			t;
	float			w;
};

// offsets for SIMD code
#define JOINTQUAT_SIZE				(8*4)		// sizeof( hfJointQuat )
#define JOINTQUAT_SIZE_SHIFT		5			// log2( sizeof( hfJointQuat ) )
#define JOINTQUAT_Q_OFFSET			(0*4)		// offsetof( hfJointQuat, q )
#define JOINTQUAT_T_OFFSET			(4*4)		// offsetof( hfJointQuat, t )

assert_sizeof( hfJointQuat, JOINTQUAT_SIZE );
assert_sizeof( hfJointQuat, (1<<JOINTQUAT_SIZE_SHIFT) );
assert_offsetof( hfJointQuat, q, JOINTQUAT_Q_OFFSET );
assert_offsetof( hfJointQuat, t, JOINTQUAT_T_OFFSET );

/*
===============================================================================

	Joint Matrix

===============================================================================
*/

/*
================================================
hfJointMat has the following structure:

	hfMat3 m;
	hfVec3 t;

	m[0][0], m[1][0], m[2][0], t[0]
	m[0][1], m[1][1], m[2][1], t[1]
	m[0][2], m[1][2], m[2][2], t[2]

================================================
*/
class hfJointMat {
public:

	void			SetRotation( const hfMat3 &m );
	hfMat3			GetRotation() const;
	void			SetTranslation( const hfVec3 &t );
	hfVec3			GetTranslation() const;

	hfVec3			operator*( const hfVec3 &v ) const;							// only rotate
	hfVec3			operator*( const hfVec4 &v ) const;							// rotate and translate

	hfJointMat &	operator*=( const hfJointMat &a );							// transform
	hfJointMat &	operator/=( const hfJointMat &a );							// untransform

	bool			Compare( const hfJointMat &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfJointMat &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const hfJointMat &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const hfJointMat &a ) const;					// exact compare, no epsilon

	void			Identity();
	void			Invert();

	void			FromMat4( const hfMat4 & m );

	hfMat3			ToMat3() const;
	hfMat4			ToMat4() const;
	hfVec3			ToVec3() const;
	const float *	ToFloatPtr() const { return mat; }
	float *			ToFloatPtr() { return mat; }
	hfJointQuat		ToJointQuat() const;

	void			Transform( hfVec3 &result, const hfVec3 &v ) const;
	void			Rotate( hfVec3 &result, const hfVec3 &v ) const;

	static void		Mul( hfJointMat &result, const hfJointMat &mat, const float s );
	static void		Mad( hfJointMat &result, const hfJointMat &mat, const float s );
	static void		Multiply( hfJointMat &result, const hfJointMat &m1, const hfJointMat &m2 );
	static void		InverseMultiply( hfJointMat &result, const hfJointMat &m1, const hfJointMat &m2 );

	float			mat[3*4];
};

// offsets for SIMD code
#define JOINTMAT_SIZE				(4*3*4)		// sizeof( hfJointMat )
assert_sizeof( hfJointMat,			JOINTMAT_SIZE );

#define JOINTMAT_TYPESIZE			( 4 * 3 )
/*
========================
hfJointMat::SetRotation
========================
*/
HF_INLINE void hfJointMat::SetRotation( const hfMat3 &m ) {
	// NOTE: hfMat3 is transposed because it is column-major
	mat[0 * 4 + 0] = m[0][0];
	mat[0 * 4 + 1] = m[1][0];
	mat[0 * 4 + 2] = m[2][0];
	mat[1 * 4 + 0] = m[0][1];
	mat[1 * 4 + 1] = m[1][1];
	mat[1 * 4 + 2] = m[2][1];
	mat[2 * 4 + 0] = m[0][2];
	mat[2 * 4 + 1] = m[1][2];
	mat[2 * 4 + 2] = m[2][2];
}

/*
========================
hfJointMat::GetRotation
========================
*/
HF_INLINE hfMat3 hfJointMat::GetRotation() const {
	hfMat3 m;
	m[0][0] = mat[0 * 4 + 0];
	m[1][0] = mat[0 * 4 + 1];
	m[2][0] = mat[0 * 4 + 2];
	m[0][1] = mat[1 * 4 + 0];
	m[1][1] = mat[1 * 4 + 1];
	m[2][1] = mat[1 * 4 + 2];
	m[0][2] = mat[2 * 4 + 0];
	m[1][2] = mat[2 * 4 + 1];
	m[2][2] = mat[2 * 4 + 2];
	return m;
}

/*
========================
hfJointMat::SetTranslation
========================
*/
HF_INLINE void hfJointMat::SetTranslation( const hfVec3 &t ) {
	mat[0 * 4 + 3] = t[0];
	mat[1 * 4 + 3] = t[1];
	mat[2 * 4 + 3] = t[2];
}

/*
========================
hfJointMat::GetTranslation
========================
*/
HF_INLINE hfVec3 hfJointMat::GetTranslation() const {
	hfVec3 t;
	t[0] = mat[0 * 4 + 3];
	t[1] = mat[1 * 4 + 3];
	t[2] = mat[2 * 4 + 3];
	return t;
}

/*
========================
hfJointMat::operator*
========================
*/
HF_INLINE hfVec3 hfJointMat::operator*( const hfVec3 &v ) const {
	return hfVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] );
}

HF_INLINE hfVec3 hfJointMat::operator*( const hfVec4 &v ) const {
	return hfVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2] + mat[0 * 4 + 3] * v[3],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2] + mat[1 * 4 + 3] * v[3],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] + mat[2 * 4 + 3] * v[3] );
}

/*
========================
hfJointMat::operator*=
========================
*/
HF_INLINE hfJointMat & hfJointMat::operator*=( const hfJointMat &a ) {
	float tmp[3];

	tmp[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[0 * 4 + 1] + mat[2 * 4 + 0] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 0] * a.mat[1 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 0] * a.mat[2 * 4 + 0] + mat[1 * 4 + 0] * a.mat[2 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = tmp[0];
	mat[1 * 4 + 0] = tmp[1];
	mat[2 * 4 + 0] = tmp[2];

	tmp[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[0 * 4 + 1] + mat[2 * 4 + 1] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 1] * a.mat[1 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 1] * a.mat[2 * 4 + 0] + mat[1 * 4 + 1] * a.mat[2 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = tmp[0];
	mat[1 * 4 + 1] = tmp[1];
	mat[2 * 4 + 1] = tmp[2];

	tmp[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[0 * 4 + 1] + mat[2 * 4 + 2] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 2] * a.mat[1 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 2] * a.mat[2 * 4 + 0] + mat[1 * 4 + 2] * a.mat[2 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = tmp[0];
	mat[1 * 4 + 2] = tmp[1];
	mat[2 * 4 + 2] = tmp[2];

	tmp[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[0 * 4 + 1] + mat[2 * 4 + 3] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 3] * a.mat[1 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 3] * a.mat[2 * 4 + 0] + mat[1 * 4 + 3] * a.mat[2 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = tmp[0];
	mat[1 * 4 + 3] = tmp[1];
	mat[2 * 4 + 3] = tmp[2];

	mat[0 * 4 + 3] += a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] += a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] += a.mat[2 * 4 + 3];

	return *this;
}

/*
========================
hfJointMat::operator/=
========================
*/
HF_INLINE hfJointMat &hfJointMat::operator/=( const hfJointMat &a ) {
	float tmp[3];

	mat[0 * 4 + 3] -= a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] -= a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] -= a.mat[2 * 4 + 3];

	tmp[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 0] + mat[2 * 4 + 0] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 0] * a.mat[0 * 4 + 1] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 0] * a.mat[0 * 4 + 2] + mat[1 * 4 + 0] * a.mat[1 * 4 + 2] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = tmp[0];
	mat[1 * 4 + 0] = tmp[1];
	mat[2 * 4 + 0] = tmp[2];

	tmp[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 0] + mat[2 * 4 + 1] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 1] * a.mat[0 * 4 + 1] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 1] * a.mat[0 * 4 + 2] + mat[1 * 4 + 1] * a.mat[1 * 4 + 2] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = tmp[0];
	mat[1 * 4 + 1] = tmp[1];
	mat[2 * 4 + 1] = tmp[2];

	tmp[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 0] + mat[2 * 4 + 2] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 2] * a.mat[0 * 4 + 1] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 2] * a.mat[0 * 4 + 2] + mat[1 * 4 + 2] * a.mat[1 * 4 + 2] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = tmp[0];
	mat[1 * 4 + 2] = tmp[1];
	mat[2 * 4 + 2] = tmp[2];

	tmp[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 0] + mat[2 * 4 + 3] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 3] * a.mat[0 * 4 + 1] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 3] * a.mat[0 * 4 + 2] + mat[1 * 4 + 3] * a.mat[1 * 4 + 2] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = tmp[0];
	mat[1 * 4 + 3] = tmp[1];
	mat[2 * 4 + 3] = tmp[2];

	return *this;
}

/*
========================
hfJointMat::Compare
========================
*/
HF_INLINE bool hfJointMat::Compare( const hfJointMat &a ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfJointMat::Compare
========================
*/
HF_INLINE bool hfJointMat::Compare( const hfJointMat &a, const float epsilon ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( hfMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfJointMat::operator==
========================
*/
HF_INLINE bool hfJointMat::operator==( const hfJointMat &a ) const {
	return Compare( a );
}

/*
========================
hfJointMat::operator!=
========================
*/
HF_INLINE bool hfJointMat::operator!=( const hfJointMat &a ) const {
	return !Compare( a );
}

/*
========================
hfJointMat::Identity
========================
*/
HF_INLINE void hfJointMat::Identity() {
	mat[0 * 4 + 0] = 1.0f; mat[0 * 4 + 1] = 0.0f; mat[0 * 4 + 2] = 0.0f; mat[0 * 4 + 3] = 0.0f;
	mat[1 * 4 + 0] = 0.0f; mat[1 * 4 + 1] = 1.0f; mat[1 * 4 + 2] = 0.0f; mat[1 * 4 + 3] = 0.0f;
	mat[2 * 4 + 0] = 0.0f; mat[2 * 4 + 1] = 0.0f; mat[2 * 4 + 2] = 1.0f; mat[2 * 4 + 3] = 0.0f;
}

/*
========================
hfJointMat::Invert
========================
*/
HF_INLINE void hfJointMat::Invert() {
	float tmp[3];

	// negate inverse rotated translation part
	tmp[0] = mat[0 * 4 + 0] * mat[0 * 4 + 3] + mat[1 * 4 + 0] * mat[1 * 4 + 3] + mat[2 * 4 + 0] * mat[2 * 4 + 3];
	tmp[1] = mat[0 * 4 + 1] * mat[0 * 4 + 3] + mat[1 * 4 + 1] * mat[1 * 4 + 3] + mat[2 * 4 + 1] * mat[2 * 4 + 3];
	tmp[2] = mat[0 * 4 + 2] * mat[0 * 4 + 3] + mat[1 * 4 + 2] * mat[1 * 4 + 3] + mat[2 * 4 + 2] * mat[2 * 4 + 3];
	mat[0 * 4 + 3] = -tmp[0];
	mat[1 * 4 + 3] = -tmp[1];
	mat[2 * 4 + 3] = -tmp[2];

	// transpose rotation part
	tmp[0] = mat[0 * 4 + 1];
	mat[0 * 4 + 1] = mat[1 * 4 + 0];
	mat[1 * 4 + 0] = tmp[0];
	tmp[1] = mat[0 * 4 + 2];
	mat[0 * 4 + 2] = mat[2 * 4 + 0];
	mat[2 * 4 + 0] = tmp[1];
	tmp[2] = mat[1 * 4 + 2];
	mat[1 * 4 + 2] = mat[2 * 4 + 1];
	mat[2 * 4 + 1] = tmp[2];
}

/*
========================
hfJointMat::ToMat3
========================
*/
HF_INLINE hfMat3 hfJointMat::ToMat3() const {
	return hfMat3(	mat[0 * 4 + 0], mat[1 * 4 + 0], mat[2 * 4 + 0],
					mat[0 * 4 + 1], mat[1 * 4 + 1], mat[2 * 4 + 1],
					mat[0 * 4 + 2], mat[1 * 4 + 2], mat[2 * 4 + 2] );
}

/*
========================
hfJointMat::ToMat4
========================
*/
HF_INLINE hfMat4 hfJointMat::ToMat4() const {
	return hfMat4(	
		mat[0 * 4 + 0], mat[0 * 4 + 1], mat[0 * 4 + 2], mat[0 * 4 + 3],
		mat[1 * 4 + 0], mat[1 * 4 + 1], mat[1 * 4 + 2], mat[1 * 4 + 3],
		mat[2 * 4 + 0], mat[2 * 4 + 1], mat[2 * 4 + 2], mat[2 * 4 + 3],
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

/*
========================
hfJointMat::FromMat4
========================
*/
HF_INLINE void hfJointMat::FromMat4( const hfMat4 & m ) {
	mat[0*4+0] = m[0][0], mat[0*4+1] = m[0][1], mat[0*4+2] = m[0][2], mat[0*4+3] = m[0][3];
	mat[1*4+0] = m[1][0], mat[1*4+1] = m[1][1], mat[1*4+2] = m[1][2], mat[1*4+3] = m[1][3];
	mat[2*4+0] = m[2][0], mat[2*4+1] = m[2][1], mat[2*4+2] = m[2][2], mat[2*4+3] = m[2][3];
	assert( m[3][0] == 0.0f );
	assert( m[3][1] == 0.0f );
	assert( m[3][2] == 0.0f );
	assert( m[3][3] == 1.0f );
}

/*
========================
hfJointMat::ToVec3
========================
*/
HF_INLINE hfVec3 hfJointMat::ToVec3() const {
	return hfVec3( mat[0 * 4 + 3], mat[1 * 4 + 3], mat[2 * 4 + 3] );
}

/*
========================
hfJointMat::Transform
========================
*/
HF_INLINE void hfJointMat::Transform( hfVec3 &result, const hfVec3 &v ) const {
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z + mat[0 * 4 + 3];
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z + mat[1 * 4 + 3];
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z + mat[2 * 4 + 3];
}

/*
========================
hfJointMat::Rotate
========================
*/
HF_INLINE void hfJointMat::Rotate( hfVec3 &result, const hfVec3 &v ) const {
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z;
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z;
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z;
}

/*
========================
hfJointMat::Mul
========================
*/
HF_INLINE void hfJointMat::Mul( hfJointMat &result, const hfJointMat &mat, const float s ) {
	result.mat[0 * 4 + 0] = s * mat.mat[0 * 4 + 0];
	result.mat[0 * 4 + 1] = s * mat.mat[0 * 4 + 1];
	result.mat[0 * 4 + 2] = s * mat.mat[0 * 4 + 2];
	result.mat[0 * 4 + 3] = s * mat.mat[0 * 4 + 3];
	result.mat[1 * 4 + 0] = s * mat.mat[1 * 4 + 0];
	result.mat[1 * 4 + 1] = s * mat.mat[1 * 4 + 1];
	result.mat[1 * 4 + 2] = s * mat.mat[1 * 4 + 2];
	result.mat[1 * 4 + 3] = s * mat.mat[1 * 4 + 3];
	result.mat[2 * 4 + 0] = s * mat.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] = s * mat.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] = s * mat.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] = s * mat.mat[2 * 4 + 3];
}

/*
========================
hfJointMat::Mad
========================
*/
HF_INLINE void hfJointMat::Mad( hfJointMat &result, const hfJointMat &mat, const float s ) {
	result.mat[0 * 4 + 0] += s * mat.mat[0 * 4 + 0];
	result.mat[0 * 4 + 1] += s * mat.mat[0 * 4 + 1];
	result.mat[0 * 4 + 2] += s * mat.mat[0 * 4 + 2];
	result.mat[0 * 4 + 3] += s * mat.mat[0 * 4 + 3];
	result.mat[1 * 4 + 0] += s * mat.mat[1 * 4 + 0];
	result.mat[1 * 4 + 1] += s * mat.mat[1 * 4 + 1];
	result.mat[1 * 4 + 2] += s * mat.mat[1 * 4 + 2];
	result.mat[1 * 4 + 3] += s * mat.mat[1 * 4 + 3];
	result.mat[2 * 4 + 0] += s * mat.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] += s * mat.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] += s * mat.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] += s * mat.mat[2 * 4 + 3];
}

/*
========================
hfJointMat::Multiply
========================
*/
HF_INLINE void hfJointMat::Multiply( hfJointMat &result, const hfJointMat &m1, const hfJointMat &m2 ) {
	result.mat[0 * 4 + 0] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[0 * 4 + 1] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[0 * 4 + 2] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[0 * 4 + 3] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[0 * 4 + 3];

	result.mat[1 * 4 + 0] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[1 * 4 + 1] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[1 * 4 + 2] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[1 * 4 + 3] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[1 * 4 + 3];

	result.mat[2 * 4 + 0] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[2 * 4 + 3];
}

/*
========================
hfJointMat::InverseMultiply
========================
*/
HF_INLINE void hfJointMat::InverseMultiply( hfJointMat &result, const hfJointMat &m1, const hfJointMat &m2 ) {
	float dst[3];

	result.mat[0 * 4 + 0] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[0 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[0 * 4 + 2];
	result.mat[0 * 4 + 1] = m1.mat[0 * 4 + 0] * m2.mat[1 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[1 * 4 + 2];
	result.mat[0 * 4 + 2] = m1.mat[0 * 4 + 0] * m2.mat[2 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[2 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 2];

	result.mat[1 * 4 + 0] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[0 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[0 * 4 + 2];
	result.mat[1 * 4 + 1] = m1.mat[1 * 4 + 0] * m2.mat[1 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[1 * 4 + 2];
	result.mat[1 * 4 + 2] = m1.mat[1 * 4 + 0] * m2.mat[2 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[2 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 2];

	result.mat[2 * 4 + 0] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[0 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[0 * 4 + 2];
	result.mat[2 * 4 + 1] = m1.mat[2 * 4 + 0] * m2.mat[1 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[1 * 4 + 2];
	result.mat[2 * 4 + 2] = m1.mat[2 * 4 + 0] * m2.mat[2 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[2 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 2];

	dst[0] = - ( m2.mat[0 * 4 + 0] * m2.mat[0 * 4 + 3] + m2.mat[1 * 4 + 0] * m2.mat[1 * 4 + 3] + m2.mat[2 * 4 + 0] * m2.mat[2 * 4 + 3] );
	dst[1] = - ( m2.mat[0 * 4 + 1] * m2.mat[0 * 4 + 3] + m2.mat[1 * 4 + 1] * m2.mat[1 * 4 + 3] + m2.mat[2 * 4 + 1] * m2.mat[2 * 4 + 3] );
	dst[2] = - ( m2.mat[0 * 4 + 2] * m2.mat[0 * 4 + 3] + m2.mat[1 * 4 + 2] * m2.mat[1 * 4 + 3] + m2.mat[2 * 4 + 2] * m2.mat[2 * 4 + 3] );

	result.mat[0 * 4 + 3] = m1.mat[0 * 4 + 0] * dst[0] + m1.mat[0 * 4 + 1] * dst[1] + m1.mat[0 * 4 + 2] * dst[2] + m1.mat[0 * 4 + 3];
	result.mat[1 * 4 + 3] = m1.mat[1 * 4 + 0] * dst[0] + m1.mat[1 * 4 + 1] * dst[1] + m1.mat[1 * 4 + 2] * dst[2] + m1.mat[1 * 4 + 3];
	result.mat[2 * 4 + 3] = m1.mat[2 * 4 + 0] * dst[0] + m1.mat[2 * 4 + 1] * dst[1] + m1.mat[2 * 4 + 2] * dst[2] + m1.mat[2 * 4 + 3];
}

#endif /* !__JOINTTRANSFORM_H__ */

