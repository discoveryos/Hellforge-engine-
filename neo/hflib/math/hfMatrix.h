#ifndef __MATH_MATRIX_H__
#define __MATH_MATRIX_H__

/*
===============================================================================

  Matrix classes, all matrices are row-major except hfMat3

===============================================================================
*/

#define MATRIX_INVERSE_EPSILON		1e-14
#define MATRIX_EPSILON				1e-6

class hfAngles;
class hfQuat;
class hfCQuat;
class hfRotation;
class hfMat4;

//===============================================================
//
//	hfMat2 - 2x2 matrix
//
//===============================================================

class hfMat2 {
public:
					hfMat2();
					explicit hfMat2( const hfVec2 &x, const hfVec2 &y );
					explicit hfMat2( const float xx, const float xy, const float yx, const float yy );
					explicit hfMat2( const float src[ 2 ][ 2 ] );

	const hfVec2 &	operator[]( int index ) const;
	hfVec2 &		operator[]( int index );
	hfMat2			operator-() const;
	hfMat2			operator*( const float a ) const;
	hfVec2			operator*( const hfVec2 &vec ) const;
	hfMat2			operator*( const hfMat2 &a ) const;
	hfMat2			operator+( const hfMat2 &a ) const;
	hfMat2			operator-( const hfMat2 &a ) const;
	hfMat2 &		operator*=( const float a );
	hfMat2 &		operator*=( const hfMat2 &a );
	hfMat2 &		operator+=( const hfMat2 &a );
	hfMat2 &		operator-=( const hfMat2 &a );

	friend hfMat2	operator*( const float a, const hfMat2 &mat );
	friend hfVec2	operator*( const hfVec2 &vec, const hfMat2 &mat );
	friend hfVec2 &	operator*=( hfVec2 &vec, const hfMat2 &mat );

	bool			Compare( const hfMat2 &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfMat2 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const hfMat2 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const hfMat2 &a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	float			Trace() const;
	float			Determinant() const;
	hfMat2			Transpose() const;	// returns transpose
	hfMat2 &		TransposeSelf();
	hfMat2			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	hfMat2			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero

	int				GetDimension() const;

	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	hfVec2			mat[ 2 ];
};

extern hfMat2 mat2_zero;
extern hfMat2 mat2_identity;
#define mat2_default	mat2_identity

HF_INLINE hfMat2::hfMat2() {
}

HF_INLINE hfMat2::hfMat2( const hfVec2 &x, const hfVec2 &y ) {
	mat[ 0 ].x = x.x; mat[ 0 ].y = x.y;
	mat[ 1 ].x = y.x; mat[ 1 ].y = y.y;
}

HF_INLINE hfMat2::hfMat2( const float xx, const float xy, const float yx, const float yy ) {
	mat[ 0 ].x = xx; mat[ 0 ].y = xy;
	mat[ 1 ].x = yx; mat[ 1 ].y = yy;
}

HF_INLINE hfMat2::hfMat2( const float src[ 2 ][ 2 ] ) {
	memcpy( mat, src, 2 * 2 * sizeof( float ) );
}

HF_INLINE const hfVec2 &hfMat2::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 2 ) );
	return mat[ index ];
}

HF_INLINE hfVec2 &hfMat2::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 2 ) );
	return mat[ index ];
}

HF_INLINE hfMat2 hfMat2::operator-() const {
	return hfMat2(	-mat[0][0], -mat[0][1],
					-mat[1][0], -mat[1][1] );
}

HF_INLINE hfVec2 hfMat2::operator*( const hfVec2 &vec ) const {
	return hfVec2(
		mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y,
		mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y );
}

HF_INLINE hfMat2 hfMat2::operator*( const hfMat2 &a ) const {
	return hfMat2(
		mat[0].x * a[0].x + mat[0].y * a[1].x,
		mat[0].x * a[0].y + mat[0].y * a[1].y,
		mat[1].x * a[0].x + mat[1].y * a[1].x,
		mat[1].x * a[0].y + mat[1].y * a[1].y );
}

HF_INLINE hfMat2 hfMat2::operator*( const float a ) const {
	return hfMat2(
		mat[0].x * a, mat[0].y * a, 
		mat[1].x * a, mat[1].y * a );
}

HF_INLINE hfMat2 hfMat2::operator+( const hfMat2 &a ) const {
	return hfMat2(
		mat[0].x + a[0].x, mat[0].y + a[0].y, 
		mat[1].x + a[1].x, mat[1].y + a[1].y );
}
    
HF_INLINE hfMat2 hfMat2::operator-( const hfMat2 &a ) const {
	return hfMat2(
		mat[0].x - a[0].x, mat[0].y - a[0].y,
		mat[1].x - a[1].x, mat[1].y - a[1].y );
}

HF_INLINE hfMat2 &hfMat2::operator*=( const float a ) {
	mat[0].x *= a; mat[0].y *= a;
	mat[1].x *= a; mat[1].y *= a;

    return *this;
}

HF_INLINE hfMat2 &hfMat2::operator*=( const hfMat2 &a ) {
	float x, y;
	x = mat[0].x; y = mat[0].y;
	mat[0].x = x * a[0].x + y * a[1].x;
	mat[0].y = x * a[0].y + y * a[1].y;
	x = mat[1].x; y = mat[1].y;
	mat[1].x = x * a[0].x + y * a[1].x;
	mat[1].y = x * a[0].y + y * a[1].y;
	return *this;
}

HF_INLINE hfMat2 &hfMat2::operator+=( const hfMat2 &a ) {
	mat[0].x += a[0].x; mat[0].y += a[0].y;
	mat[1].x += a[1].x; mat[1].y += a[1].y;

    return *this;
}

HF_INLINE hfMat2 &hfMat2::operator-=( const hfMat2 &a ) {
	mat[0].x -= a[0].x; mat[0].y -= a[0].y;
	mat[1].x -= a[1].x; mat[1].y -= a[1].y;

    return *this;
}

HF_INLINE hfVec2 operator*( const hfVec2 &vec, const hfMat2 &mat ) {
	return mat * vec;
}

HF_INLINE hfMat2 operator*( const float a, hfMat2 const &mat ) {
	return mat * a;
}

HF_INLINE hfVec2 &operator*=( hfVec2 &vec, const hfMat2 &mat ) {
	vec = mat * vec;
	return vec;
}

HF_INLINE bool hfMat2::Compare( const hfMat2 &a ) const {
	if ( mat[0].Compare( a[0] ) &&
		mat[1].Compare( a[1] ) ) {
		return true;
	}
	return false;
}

HF_INLINE bool hfMat2::Compare( const hfMat2 &a, const float epsilon ) const {
	if ( mat[0].Compare( a[0], epsilon ) &&
		mat[1].Compare( a[1], epsilon ) ) {
		return true;
	}
	return false;
}

HF_INLINE bool hfMat2::operator==( const hfMat2 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfMat2::operator!=( const hfMat2 &a ) const {
	return !Compare( a );
}

HF_INLINE void hfMat2::Zero() {
	mat[0].Zero();
	mat[1].Zero();
}

HF_INLINE void hfMat2::Identity() {
	*this = mat2_identity;
}

HF_INLINE bool hfMat2::IsIdentity( const float epsilon ) const {
	return Compare( mat2_identity, epsilon );
}

HF_INLINE bool hfMat2::IsSymmetric( const float epsilon ) const {
	return ( hfMath::Fabs( mat[0][1] - mat[1][0] ) < epsilon );
}

HF_INLINE bool hfMat2::IsDiagonal( const float epsilon ) const {
	if ( hfMath::Fabs( mat[0][1] ) > epsilon ||
		hfMath::Fabs( mat[1][0] ) > epsilon ) {
		return false;
	}
	return true;
}

HF_INLINE float hfMat2::Trace() const {
	return ( mat[0][0] + mat[1][1] );
}

HF_INLINE float hfMat2::Determinant() const {
	return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
}

HF_INLINE hfMat2 hfMat2::Transpose() const {
	return hfMat2(	mat[0][0], mat[1][0],
					mat[0][1], mat[1][1] );
}

HF_INLINE hfMat2 &hfMat2::TransposeSelf() {
	float tmp;

	tmp = mat[0][1];
	mat[0][1] = mat[1][0];
	mat[1][0] = tmp;

	return *this;
}

HF_INLINE hfMat2 hfMat2::Inverse() const {
	hfMat2 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

HF_INLINE hfMat2 hfMat2::InverseFast() const {
	hfMat2 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

HF_INLINE int hfMat2::GetDimension() const {
	return 4;
}

HF_INLINE const float *hfMat2::ToFloatPtr() const {
	return mat[0].ToFloatPtr();
}

HF_INLINE float *hfMat2::ToFloatPtr() {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	hfMat3 - 3x3 matrix
//
//	NOTE:	matrix is column-major
//
//===============================================================

class hfMat3 {
public:
					hfMat3();
					explicit hfMat3( const hfVec3 &x, const hfVec3 &y, const hfVec3 &z );
					explicit hfMat3( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz );
					explicit hfMat3( const float src[ 3 ][ 3 ] );

	const hfVec3 &	operator[]( int index ) const;
	hfVec3 &		operator[]( int index );
	hfMat3			operator-() const;
	hfMat3			operator*( const float a ) const;
	hfVec3			operator*( const hfVec3 &vec ) const;
	hfMat3			operator*( const hfMat3 &a ) const;
	hfMat3			operator+( const hfMat3 &a ) const;
	hfMat3			operator-( const hfMat3 &a ) const;
	hfMat3 &		operator*=( const float a );
	hfMat3 &		operator*=( const hfMat3 &a );
	hfMat3 &		operator+=( const hfMat3 &a );
	hfMat3 &		operator-=( const hfMat3 &a );

	friend hfMat3	operator*( const float a, const hfMat3 &mat );
	friend hfVec3	operator*( const hfVec3 &vec, const hfMat3 &mat );
	friend hfVec3 &	operator*=( hfVec3 &vec, const hfMat3 &mat );

	bool			Compare( const hfMat3 &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfMat3 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const hfMat3 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const hfMat3 &a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsRotated() const;

	void			ProjectVector( const hfVec3 &src, hfVec3 &dst ) const;
	void			UnprojectVector( const hfVec3 &src, hfVec3 &dst ) const;

	bool			FixDegeneracies();	// fix degenerate axial cases
	bool			FixDenormals();		// change tiny numbers to zero

	float			Trace() const;
	float			Determinant() const;
	hfMat3			OrthoNormalize() const;
	hfMat3 &		OrthoNormalizeSelf();
	hfMat3			Transpose() const;	// returns transpose
	hfMat3 &		TransposeSelf();
	hfMat3			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	hfMat3			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero
	hfMat3			TransposeMultiply( const hfMat3 &b ) const;

	hfMat3			InertiaTranslate( const float mass, const hfVec3 &centerOfMass, const hfVec3 &translation ) const;
	hfMat3 &		InertiaTranslateSelf( const float mass, const hfVec3 &centerOfMass, const hfVec3 &translation );
	hfMat3			InertiaRotate( const hfMat3 &rotation ) const;
	hfMat3 &		InertiaRotateSelf( const hfMat3 &rotation );

	int				GetDimension() const;

	hfAngles		ToAngles() const;
	hfQuat			ToQuat() const;
	hfCQuat			ToCQuat() const;
	hfRotation		ToRotation() const;
	hfMat4			ToMat4() const;
	hfVec3			ToAngularVelocity() const;
	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

	friend void		TransposeMultiply( const hfMat3 &inv, const hfMat3 &b, hfMat3 &dst );
	friend hfMat3	SkewSymmetric( hfVec3 const &src );

private:
	hfVec3			mat[ 3 ];
};

extern hfMat3 mat3_zero;
extern hfMat3 mat3_identity;
#define mat3_default	mat3_identity

HF_INLINE hfMat3::hfMat3() {
}

HF_INLINE hfMat3::hfMat3( const hfVec3 &x, const hfVec3 &y, const hfVec3 &z ) {
	mat[ 0 ].x = x.x; mat[ 0 ].y = x.y; mat[ 0 ].z = x.z;
	mat[ 1 ].x = y.x; mat[ 1 ].y = y.y; mat[ 1 ].z = y.z;
	mat[ 2 ].x = z.x; mat[ 2 ].y = z.y; mat[ 2 ].z = z.z;
}

HF_INLINE hfMat3::hfMat3( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz ) {
	mat[ 0 ].x = xx; mat[ 0 ].y = xy; mat[ 0 ].z = xz;
	mat[ 1 ].x = yx; mat[ 1 ].y = yy; mat[ 1 ].z = yz;
	mat[ 2 ].x = zx; mat[ 2 ].y = zy; mat[ 2 ].z = zz;
}

HF_INLINE hfMat3::hfMat3( const float src[ 3 ][ 3 ] ) {
	memcpy( mat, src, 3 * 3 * sizeof( float ) );
}

HF_INLINE const hfVec3 &hfMat3::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[ index ];
}

HF_INLINE hfVec3 &hfMat3::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[ index ];
}

HF_INLINE hfMat3 hfMat3::operator-() const {
	return hfMat3(	-mat[0][0], -mat[0][1], -mat[0][2],
					-mat[1][0], -mat[1][1], -mat[1][2],
					-mat[2][0], -mat[2][1], -mat[2][2] );
}

HF_INLINE hfVec3 hfMat3::operator*( const hfVec3 &vec ) const {
	return hfVec3(
		mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z,
		mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z,
		mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z );
}

HF_INLINE hfMat3 hfMat3::operator*( const hfMat3 &a ) const {
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	hfMat3 dst;

	m1Ptr = reinterpret_cast<const float *>(this);
	m2Ptr = reinterpret_cast<const float *>(&a);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 3 + j ];
			dstPtr++;
		}
		m1Ptr += 3;
	}
	return dst;
}

HF_INLINE hfMat3 hfMat3::operator*( const float a ) const {
	return hfMat3(
		mat[0].x * a, mat[0].y * a, mat[0].z * a,
		mat[1].x * a, mat[1].y * a, mat[1].z * a,
		mat[2].x * a, mat[2].y * a, mat[2].z * a );
}

HF_INLINE hfMat3 hfMat3::operator+( const hfMat3 &a ) const {
	return hfMat3(
		mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z,
		mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z,
		mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z );
}
    
HF_INLINE hfMat3 hfMat3::operator-( const hfMat3 &a ) const {
	return hfMat3(
		mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z,
		mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z,
		mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z );
}

HF_INLINE hfMat3 &hfMat3::operator*=( const float a ) {
	mat[0].x *= a; mat[0].y *= a; mat[0].z *= a;
	mat[1].x *= a; mat[1].y *= a; mat[1].z *= a; 
	mat[2].x *= a; mat[2].y *= a; mat[2].z *= a;

    return *this;
}

HF_INLINE hfMat3 &hfMat3::operator*=( const hfMat3 &a ) {
	int i, j;
	const float *m2Ptr;
	float *m1Ptr, dst[3];

	m1Ptr = reinterpret_cast<float *>(this);
	m2Ptr = reinterpret_cast<const float *>(&a);

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			dst[j]  = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 3 + j ];
		}
		m1Ptr[0] = dst[0]; m1Ptr[1] = dst[1]; m1Ptr[2] = dst[2];
		m1Ptr += 3;
	}
	return *this;
}

HF_INLINE hfMat3 &hfMat3::operator+=( const hfMat3 &a ) {
	mat[0].x += a[0].x; mat[0].y += a[0].y; mat[0].z += a[0].z;
	mat[1].x += a[1].x; mat[1].y += a[1].y; mat[1].z += a[1].z;
	mat[2].x += a[2].x; mat[2].y += a[2].y; mat[2].z += a[2].z;

    return *this;
}

HF_INLINE hfMat3 &hfMat3::operator-=( const hfMat3 &a ) {
	mat[0].x -= a[0].x; mat[0].y -= a[0].y; mat[0].z -= a[0].z;
	mat[1].x -= a[1].x; mat[1].y -= a[1].y; mat[1].z -= a[1].z;
	mat[2].x -= a[2].x; mat[2].y -= a[2].y; mat[2].z -= a[2].z;

    return *this;
}

HF_INLINE hfVec3 operator*( const hfVec3 &vec, const hfMat3 &mat ) {
	return mat * vec;
}

HF_INLINE hfMat3 operator*( const float a, const hfMat3 &mat ) {
	return mat * a;
}

HF_INLINE hfVec3 &operator*=( hfVec3 &vec, const hfMat3 &mat ) {
	float x = mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z;
	float y = mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z;
	vec.z = mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z;
	vec.x = x;
	vec.y = y;
	return vec;
}

HF_INLINE bool hfMat3::Compare( const hfMat3 &a ) const {
	if ( mat[0].Compare( a[0] ) &&
		mat[1].Compare( a[1] ) &&
		mat[2].Compare( a[2] ) ) {
		return true;
	}
	return false;
}

HF_INLINE bool hfMat3::Compare( const hfMat3 &a, const float epsilon ) const {
	if ( mat[0].Compare( a[0], epsilon ) &&
		mat[1].Compare( a[1], epsilon ) &&
		mat[2].Compare( a[2], epsilon ) ) {
		return true;
	}
	return false;
}

HF_INLINE bool hfMat3::operator==( const hfMat3 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfMat3::operator!=( const hfMat3 &a ) const {
	return !Compare( a );
}

HF_INLINE void hfMat3::Zero() {
	memset( mat, 0, sizeof( hfMat3 ) );
}

HF_INLINE void hfMat3::Identity() {
	*this = mat3_identity;
}

HF_INLINE bool hfMat3::IsIdentity( const float epsilon ) const {
	return Compare( mat3_identity, epsilon );
}

HF_INLINE bool hfMat3::IsSymmetric( const float epsilon ) const {
	if ( hfMath::Fabs( mat[0][1] - mat[1][0] ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( mat[0][2] - mat[2][0] ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( mat[1][2] - mat[2][1] ) > epsilon ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfMat3::IsDiagonal( const float epsilon ) const {
	if ( hfMath::Fabs( mat[0][1] ) > epsilon ||
		hfMath::Fabs( mat[0][2] ) > epsilon ||
		hfMath::Fabs( mat[1][0] ) > epsilon ||
		hfMath::Fabs( mat[1][2] ) > epsilon ||
		hfMath::Fabs( mat[2][0] ) > epsilon ||
		hfMath::Fabs( mat[2][1] ) > epsilon ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfMat3::IsRotated() const {
	return !Compare( mat3_identity );
}

HF_INLINE void hfMat3::ProjectVector( const hfVec3 &src, hfVec3 &dst ) const {
	dst.x = src * mat[ 0 ];
	dst.y = src * mat[ 1 ];
	dst.z = src * mat[ 2 ];
}

HF_INLINE void hfMat3::UnprojectVector( const hfVec3 &src, hfVec3 &dst ) const {
	dst = mat[ 0 ] * src.x + mat[ 1 ] * src.y + mat[ 2 ] * src.z;
}

HF_INLINE bool hfMat3::FixDegeneracies() {
	bool r = mat[0].FixDegenerateNormal();
	r |= mat[1].FixDegenerateNormal();
	r |= mat[2].FixDegenerateNormal();
	return r;
}

HF_INLINE bool hfMat3::FixDenormals() {
	bool r = mat[0].FixDenormals();
	r |= mat[1].FixDenormals();
	r |= mat[2].FixDenormals();
	return r;
}

HF_INLINE float hfMat3::Trace() const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] );
}

HF_INLINE hfMat3 hfMat3::OrthoNormalize() const {
	hfMat3 ortho;

	ortho = *this;
	ortho[ 0 ].Normalize();
	ortho[ 2 ].Cross( mat[ 0 ], mat[ 1 ] );
	ortho[ 2 ].Normalize();
	ortho[ 1 ].Cross( mat[ 2 ], mat[ 0 ] );
	ortho[ 1 ].Normalize();
	return ortho;
}

HF_INLINE hfMat3 &hfMat3::OrthoNormalizeSelf() {
	mat[ 0 ].Normalize();
	mat[ 2 ].Cross( mat[ 0 ], mat[ 1 ] );
	mat[ 2 ].Normalize();
	mat[ 1 ].Cross( mat[ 2 ], mat[ 0 ] );
	mat[ 1 ].Normalize();
	return *this;
}

HF_INLINE hfMat3 hfMat3::Transpose() const {
	return hfMat3(	mat[0][0], mat[1][0], mat[2][0],
					mat[0][1], mat[1][1], mat[2][1],
					mat[0][2], mat[1][2], mat[2][2] );
}

HF_INLINE hfMat3 &hfMat3::TransposeSelf() {
	float tmp0, tmp1, tmp2;

	tmp0 = mat[0][1];
	mat[0][1] = mat[1][0];
	mat[1][0] = tmp0;
	tmp1 = mat[0][2];
	mat[0][2] = mat[2][0];
	mat[2][0] = tmp1;
	tmp2 = mat[1][2];
	mat[1][2] = mat[2][1];
	mat[2][1] = tmp2;

	return *this;
}

HF_INLINE hfMat3 hfMat3::Inverse() const {
	hfMat3 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

HF_INLINE hfMat3 hfMat3::InverseFast() const {
	hfMat3 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

HF_INLINE hfMat3 hfMat3::TransposeMultiply( const hfMat3 &b ) const {
	return hfMat3(	mat[0].x * b[0].x + mat[1].x * b[1].x + mat[2].x * b[2].x,
					mat[0].x * b[0].y + mat[1].x * b[1].y + mat[2].x * b[2].y,
					mat[0].x * b[0].z + mat[1].x * b[1].z + mat[2].x * b[2].z,
					mat[0].y * b[0].x + mat[1].y * b[1].x + mat[2].y * b[2].x,
					mat[0].y * b[0].y + mat[1].y * b[1].y + mat[2].y * b[2].y,
					mat[0].y * b[0].z + mat[1].y * b[1].z + mat[2].y * b[2].z,
					mat[0].z * b[0].x + mat[1].z * b[1].x + mat[2].z * b[2].x,
					mat[0].z * b[0].y + mat[1].z * b[1].y + mat[2].z * b[2].y,
					mat[0].z * b[0].z + mat[1].z * b[1].z + mat[2].z * b[2].z );
}

HF_INLINE void TransposeMultiply( const hfMat3 &transpose, const hfMat3 &b, hfMat3 &dst ) {
	dst[0].x = transpose[0].x * b[0].x + transpose[1].x * b[1].x + transpose[2].x * b[2].x;
	dst[0].y = transpose[0].x * b[0].y + transpose[1].x * b[1].y + transpose[2].x * b[2].y;
	dst[0].z = transpose[0].x * b[0].z + transpose[1].x * b[1].z + transpose[2].x * b[2].z;
	dst[1].x = transpose[0].y * b[0].x + transpose[1].y * b[1].x + transpose[2].y * b[2].x;
	dst[1].y = transpose[0].y * b[0].y + transpose[1].y * b[1].y + transpose[2].y * b[2].y;
	dst[1].z = transpose[0].y * b[0].z + transpose[1].y * b[1].z + transpose[2].y * b[2].z;
	dst[2].x = transpose[0].z * b[0].x + transpose[1].z * b[1].x + transpose[2].z * b[2].x;
	dst[2].y = transpose[0].z * b[0].y + transpose[1].z * b[1].y + transpose[2].z * b[2].y;
	dst[2].z = transpose[0].z * b[0].z + transpose[1].z * b[1].z + transpose[2].z * b[2].z;
}

HF_INLINE hfMat3 SkewSymmetric( hfVec3 const &src ) {
	return hfMat3( 0.0f, -src.z,  src.y, src.z,   0.0f, -src.x, -src.y,  src.x,   0.0f );
}

HF_INLINE int hfMat3::GetDimension() const {
	return 9;
}

HF_INLINE const float *hfMat3::ToFloatPtr() const {
	return mat[0].ToFloatPtr();
}

HF_INLINE float *hfMat3::ToFloatPtr() {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	hfMat4 - 4x4 matrix
//
//===============================================================

class hfMat4 {
public:
					hfMat4();
					explicit hfMat4( const hfVec4 &x, const hfVec4 &y, const hfVec4 &z, const hfVec4 &w );
					explicit hfMat4(const float xx, const float xy, const float xz, const float xw,
									const float yx, const float yy, const float yz, const float yw,
									const float zx, const float zy, const float zz, const float zw,
									const float wx, const float wy, const float wz, const float ww );
					explicit hfMat4( const hfMat3 &rotation, const hfVec3 &translation );
					explicit hfMat4( const float src[ 4 ][ 4 ] );

	const hfVec4 &	operator[]( int index ) const;
	hfVec4 &		operator[]( int index );
	hfMat4			operator*( const float a ) const;
	hfVec4			operator*( const hfVec4 &vec ) const;
	hfVec3			operator*( const hfVec3 &vec ) const;
	hfMat4			operator*( const hfMat4 &a ) const;
	hfMat4			operator+( const hfMat4 &a ) const;
	hfMat4			operator-( const hfMat4 &a ) const;
	hfMat4 &		operator*=( const float a );
	hfMat4 &		operator*=( const hfMat4 &a );
	hfMat4 &		operator+=( const hfMat4 &a );
	hfMat4 &		operator-=( const hfMat4 &a );

	friend hfMat4	operator*( const float a, const hfMat4 &mat );
	friend hfVec4	operator*( const hfVec4 &vec, const hfMat4 &mat );
	friend hfVec3	operator*( const hfVec3 &vec, const hfMat4 &mat );
	friend hfVec4 &	operator*=( hfVec4 &vec, const hfMat4 &mat );
	friend hfVec3 &	operator*=( hfVec3 &vec, const hfMat4 &mat );

	bool			Compare( const hfMat4 &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfMat4 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const hfMat4 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const hfMat4 &a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsRotated() const;

	void			ProjectVector( const hfVec4 &src, hfVec4 &dst ) const;
	void			UnprojectVector( const hfVec4 &src, hfVec4 &dst ) const;

	float			Trace() const;
	float			Determinant() const;
	hfMat4			Transpose() const;	// returns transpose
	hfMat4 &		TransposeSelf();
	hfMat4			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	hfMat4			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero
	hfMat4			TransposeMultiply( const hfMat4 &b ) const;

	int				GetDimension() const;

	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	hfVec4			mat[ 4 ];
};

extern hfMat4 mat4_zero;
extern hfMat4 mat4_identity;
#define mat4_default	mat4_identity

HF_INLINE hfMat4::hfMat4() {
}

HF_INLINE hfMat4::hfMat4( const hfVec4 &x, const hfVec4 &y, const hfVec4 &z, const hfVec4 &w ) {
	mat[ 0 ] = x;
	mat[ 1 ] = y;
	mat[ 2 ] = z;
	mat[ 3 ] = w;
}

HF_INLINE hfMat4::hfMat4( const float xx, const float xy, const float xz, const float xw,
							const float yx, const float yy, const float yz, const float yw,
							const float zx, const float zy, const float zz, const float zw,
							const float wx, const float wy, const float wz, const float ww ) {
	mat[0][0] = xx; mat[0][1] = xy; mat[0][2] = xz; mat[0][3] = xw;
	mat[1][0] = yx; mat[1][1] = yy; mat[1][2] = yz; mat[1][3] = yw;
	mat[2][0] = zx; mat[2][1] = zy; mat[2][2] = zz; mat[2][3] = zw;
	mat[3][0] = wx; mat[3][1] = wy; mat[3][2] = wz; mat[3][3] = ww;
}

HF_INLINE hfMat4::hfMat4( const hfMat3 &rotation, const hfVec3 &translation ) {
	// NOTE: hfMat3 is transposed because it is column-major
	mat[ 0 ][ 0 ] = rotation[0][0];
	mat[ 0 ][ 1 ] = rotation[1][0];
	mat[ 0 ][ 2 ] = rotation[2][0];
	mat[ 0 ][ 3 ] = translation[0];
	mat[ 1 ][ 0 ] = rotation[0][1];
	mat[ 1 ][ 1 ] = rotation[1][1];
	mat[ 1 ][ 2 ] = rotation[2][1];
	mat[ 1 ][ 3 ] = translation[1];
	mat[ 2 ][ 0 ] = rotation[0][2];
	mat[ 2 ][ 1 ] = rotation[1][2];
	mat[ 2 ][ 2 ] = rotation[2][2];
	mat[ 2 ][ 3 ] = translation[2];
	mat[ 3 ][ 0 ] = 0.0f;
	mat[ 3 ][ 1 ] = 0.0f;
	mat[ 3 ][ 2 ] = 0.0f;
	mat[ 3 ][ 3 ] = 1.0f;
}

HF_INLINE hfMat4::hfMat4( const float src[ 4 ][ 4 ] ) {
	memcpy( mat, src, 4 * 4 * sizeof( float ) );
}

HF_INLINE const hfVec4 &hfMat4::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 4 ) );
	return mat[ index ];
}

HF_INLINE hfVec4 &hfMat4::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 4 ) );
	return mat[ index ];
}

HF_INLINE hfMat4 hfMat4::operator*( const float a ) const {
	return hfMat4(
		mat[0].x * a, mat[0].y * a, mat[0].z * a, mat[0].w * a,
		mat[1].x * a, mat[1].y * a, mat[1].z * a, mat[1].w * a,
		mat[2].x * a, mat[2].y * a, mat[2].z * a, mat[2].w * a,
		mat[3].x * a, mat[3].y * a, mat[3].z * a, mat[3].w * a );
}

HF_INLINE hfVec4 hfMat4::operator*( const hfVec4 &vec ) const {
	return hfVec4(
		mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w * vec.w,
		mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w * vec.w,
		mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w * vec.w,
		mat[ 3 ].x * vec.x + mat[ 3 ].y * vec.y + mat[ 3 ].z * vec.z + mat[ 3 ].w * vec.w );
}

HF_INLINE hfVec3 hfMat4::operator*( const hfVec3 &vec ) const {
	float s = mat[ 3 ].x * vec.x + mat[ 3 ].y * vec.y + mat[ 3 ].z * vec.z + mat[ 3 ].w;
	if ( s == 0.0f ) {
		return hfVec3( 0.0f, 0.0f, 0.0f );
	}
	if ( s == 1.0f ) {
		return hfVec3(
			mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w,
			mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w,
			mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w );
	}
	else {
		float invS = 1.0f / s;
		return hfVec3(
			(mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w) * invS,
			(mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w) * invS,
			(mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w) * invS );
	}
}

HF_INLINE hfMat4 hfMat4::operator*( const hfMat4 &a ) const {
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	hfMat4 dst;

	m1Ptr = reinterpret_cast<const float *>(this);
	m2Ptr = reinterpret_cast<const float *>(&a);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 4 + j ];
			dstPtr++;
		}
		m1Ptr += 4;
	}
	return dst;
}

HF_INLINE hfMat4 hfMat4::operator+( const hfMat4 &a ) const {
	return hfMat4( 
		mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z, mat[0].w + a[0].w,
		mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z, mat[1].w + a[1].w,
		mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z, mat[2].w + a[2].w,
		mat[3].x + a[3].x, mat[3].y + a[3].y, mat[3].z + a[3].z, mat[3].w + a[3].w );
}
    
HF_INLINE hfMat4 hfMat4::operator-( const hfMat4 &a ) const {
	return hfMat4( 
		mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z, mat[0].w - a[0].w,
		mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z, mat[1].w - a[1].w,
		mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z, mat[2].w - a[2].w,
		mat[3].x - a[3].x, mat[3].y - a[3].y, mat[3].z - a[3].z, mat[3].w - a[3].w );
}

HF_INLINE hfMat4 &hfMat4::operator*=( const float a ) {
	mat[0].x *= a; mat[0].y *= a; mat[0].z *= a; mat[0].w *= a;
	mat[1].x *= a; mat[1].y *= a; mat[1].z *= a; mat[1].w *= a;
	mat[2].x *= a; mat[2].y *= a; mat[2].z *= a; mat[2].w *= a;
	mat[3].x *= a; mat[3].y *= a; mat[3].z *= a; mat[3].w *= a;
    return *this;
}

HF_INLINE hfMat4 &hfMat4::operator*=( const hfMat4 &a ) {
	*this = (*this) * a;
	return *this;
}

HF_INLINE hfMat4 &hfMat4::operator+=( const hfMat4 &a ) {
	mat[0].x += a[0].x; mat[0].y += a[0].y; mat[0].z += a[0].z; mat[0].w += a[0].w;
	mat[1].x += a[1].x; mat[1].y += a[1].y; mat[1].z += a[1].z; mat[1].w += a[1].w;
	mat[2].x += a[2].x; mat[2].y += a[2].y; mat[2].z += a[2].z; mat[2].w += a[2].w;
	mat[3].x += a[3].x; mat[3].y += a[3].y; mat[3].z += a[3].z; mat[3].w += a[3].w;
    return *this;
}

HF_INLINE hfMat4 &hfMat4::operator-=( const hfMat4 &a ) {
	mat[0].x -= a[0].x; mat[0].y -= a[0].y; mat[0].z -= a[0].z; mat[0].w -= a[0].w;
	mat[1].x -= a[1].x; mat[1].y -= a[1].y; mat[1].z -= a[1].z; mat[1].w -= a[1].w;
	mat[2].x -= a[2].x; mat[2].y -= a[2].y; mat[2].z -= a[2].z; mat[2].w -= a[2].w;
	mat[3].x -= a[3].x; mat[3].y -= a[3].y; mat[3].z -= a[3].z; mat[3].w -= a[3].w;
    return *this;
}

HF_INLINE hfMat4 operator*( const float a, const hfMat4 &mat ) {
	return mat * a;
}

HF_INLINE hfVec4 operator*( const hfVec4 &vec, const hfMat4 &mat ) {
	return mat * vec;
}

HF_INLINE hfVec3 operator*( const hfVec3 &vec, const hfMat4 &mat ) {
	return mat * vec;
}

HF_INLINE hfVec4 &operator*=( hfVec4 &vec, const hfMat4 &mat ) {
	vec = mat * vec;
	return vec;
}

HF_INLINE hfVec3 &operator*=( hfVec3 &vec, const hfMat4 &mat ) {
	vec = mat * vec;
	return vec;
}

HF_INLINE bool hfMat4::Compare( const hfMat4 &a ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 4*4; i++ ) {
		if ( ptr1[i] != ptr2[i] ) {
			return false;
		}
	}
	return true;
}

HF_INLINE bool hfMat4::Compare( const hfMat4 &a, const float epsilon ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 4*4; i++ ) {
		if ( hfMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

HF_INLINE bool hfMat4::operator==( const hfMat4 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfMat4::operator!=( const hfMat4 &a ) const {
	return !Compare( a );
}

HF_INLINE void hfMat4::Zero() {
	memset( mat, 0, sizeof( hfMat4 ) );
}

HF_INLINE void hfMat4::Identity() {
	*this = mat4_identity;
}

HF_INLINE bool hfMat4::IsIdentity( const float epsilon ) const {
	return Compare( mat4_identity, epsilon );
}

HF_INLINE bool hfMat4::IsSymmetric( const float epsilon ) const {
	for ( int i = 1; i < 4; i++ ) {
		for ( int j = 0; j < i; j++ ) {
			if ( hfMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

HF_INLINE bool hfMat4::IsDiagonal( const float epsilon ) const {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			if ( i != j && hfMath::Fabs( mat[i][j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

HF_INLINE bool hfMat4::IsRotated() const {
	if ( !mat[ 0 ][ 1 ] && !mat[ 0 ][ 2 ] &&
		!mat[ 1 ][ 0 ] && !mat[ 1 ][ 2 ] &&
		!mat[ 2 ][ 0 ] && !mat[ 2 ][ 1 ] ) {
		return false;
	}
	return true;
}

HF_INLINE void hfMat4::ProjectVector( const hfVec4 &src, hfVec4 &dst ) const {
	dst.x = src * mat[ 0 ];
	dst.y = src * mat[ 1 ];
	dst.z = src * mat[ 2 ];
	dst.w = src * mat[ 3 ];
}

HF_INLINE void hfMat4::UnprojectVector( const hfVec4 &src, hfVec4 &dst ) const {
	dst = mat[ 0 ] * src.x + mat[ 1 ] * src.y + mat[ 2 ] * src.z + mat[ 3 ] * src.w;
}

HF_INLINE float hfMat4::Trace() const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] );
}

HF_INLINE hfMat4 hfMat4::Inverse() const {
	hfMat4 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

HF_INLINE hfMat4 hfMat4::InverseFast() const {
	hfMat4 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

HF_INLINE hfMat4 hfMat3::ToMat4() const {
	// NOTE: hfMat3 is transposed because it is column-major
	return hfMat4(	mat[0][0],	mat[1][0],	mat[2][0],	0.0f,
					mat[0][1],	mat[1][1],	mat[2][1],	0.0f,
					mat[0][2],	mat[1][2],	mat[2][2],	0.0f,
					0.0f,		0.0f,		0.0f,		1.0f );
}

HF_INLINE int hfMat4::GetDimension() const {
	return 16;
}

HF_INLINE const float *hfMat4::ToFloatPtr() const {
	return mat[0].ToFloatPtr();
}

HF_INLINE float *hfMat4::ToFloatPtr() {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	hfMat5 - 5x5 matrix
//
//===============================================================

class hfMat5 {
public:
					hfMat5();
					explicit hfMat5( const hfVec5 &v0, const hfVec5 &v1, const hfVec5 &v2, const hfVec5 &v3, const hfVec5 &v4 );
					explicit hfMat5( const float src[ 5 ][ 5 ] );

	const hfVec5 &	operator[]( int index ) const;
	hfVec5 &		operator[]( int index );
	hfMat5			operator*( const float a ) const;
	hfVec5			operator*( const hfVec5 &vec ) const;
	hfMat5			operator*( const hfMat5 &a ) const;
	hfMat5			operator+( const hfMat5 &a ) const;
	hfMat5			operator-( const hfMat5 &a ) const;
	hfMat5 &		operator*=( const float a );
	hfMat5 &		operator*=( const hfMat5 &a );
	hfMat5 &		operator+=( const hfMat5 &a );
	hfMat5 &		operator-=( const hfMat5 &a );

	friend hfMat5	operator*( const float a, const hfMat5 &mat );
	friend hfVec5	operator*( const hfVec5 &vec, const hfMat5 &mat );
	friend hfVec5 &	operator*=( hfVec5 &vec, const hfMat5 &mat );

	bool			Compare( const hfMat5 &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfMat5 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const hfMat5 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const hfMat5 &a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	float			Trace() const;
	float			Determinant() const;
	hfMat5			Transpose() const;	// returns transpose
	hfMat5 &		TransposeSelf();
	hfMat5			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	hfMat5			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero

	int				GetDimension() const;

	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	hfVec5			mat[ 5 ];
};

extern hfMat5 mat5_zero;
extern hfMat5 mat5_identity;
#define mat5_default	mat5_identity

HF_INLINE hfMat5::hfMat5() {
}

HF_INLINE hfMat5::hfMat5( const float src[ 5 ][ 5 ] ) {
	memcpy( mat, src, 5 * 5 * sizeof( float ) );
}

HF_INLINE hfMat5::hfMat5( const hfVec5 &v0, const hfVec5 &v1, const hfVec5 &v2, const hfVec5 &v3, const hfVec5 &v4 ) {
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
	mat[3] = v3;
	mat[4] = v4;
}

HF_INLINE const hfVec5 &hfMat5::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 5 ) );
	return mat[ index ];
}

HF_INLINE hfVec5 &hfMat5::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 5 ) );
	return mat[ index ];
}

HF_INLINE hfMat5 hfMat5::operator*( const hfMat5 &a ) const {
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	hfMat5 dst;

	m1Ptr = reinterpret_cast<const float *>(this);
	m2Ptr = reinterpret_cast<const float *>(&a);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 5; i++ ) {
		for ( j = 0; j < 5; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 5 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 5 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 5 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 5 + j ]
					+ m1Ptr[4] * m2Ptr[ 4 * 5 + j ];
			dstPtr++;
		}
		m1Ptr += 5;
	}
	return dst;
}

HF_INLINE hfMat5 hfMat5::operator*( const float a ) const {
	return hfMat5(
		hfVec5( mat[0][0] * a, mat[0][1] * a, mat[0][2] * a, mat[0][3] * a, mat[0][4] * a ),
		hfVec5( mat[1][0] * a, mat[1][1] * a, mat[1][2] * a, mat[1][3] * a, mat[1][4] * a ),
		hfVec5( mat[2][0] * a, mat[2][1] * a, mat[2][2] * a, mat[2][3] * a, mat[2][4] * a ),
		hfVec5( mat[3][0] * a, mat[3][1] * a, mat[3][2] * a, mat[3][3] * a, mat[3][4] * a ),
		hfVec5( mat[4][0] * a, mat[4][1] * a, mat[4][2] * a, mat[4][3] * a, mat[4][4] * a ) );
}

HF_INLINE hfVec5 hfMat5::operator*( const hfVec5 &vec ) const {
	return hfVec5(
		mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3] + mat[0][4] * vec[4],
		mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3] + mat[1][4] * vec[4],
		mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3] + mat[2][4] * vec[4],
		mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3] + mat[3][4] * vec[4],
		mat[4][0] * vec[0] + mat[4][1] * vec[1] + mat[4][2] * vec[2] + mat[4][3] * vec[3] + mat[4][4] * vec[4] );
}

HF_INLINE hfMat5 hfMat5::operator+( const hfMat5 &a ) const {
	return hfMat5(
		hfVec5( mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2], mat[0][3] + a[0][3], mat[0][4] + a[0][4] ),
		hfVec5( mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2], mat[1][3] + a[1][3], mat[1][4] + a[1][4] ),
		hfVec5( mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2], mat[2][3] + a[2][3], mat[2][4] + a[2][4] ),
		hfVec5( mat[3][0] + a[3][0], mat[3][1] + a[3][1], mat[3][2] + a[3][2], mat[3][3] + a[3][3], mat[3][4] + a[3][4] ),
		hfVec5( mat[4][0] + a[4][0], mat[4][1] + a[4][1], mat[4][2] + a[4][2], mat[4][3] + a[4][3], mat[4][4] + a[4][4] ) );
}

HF_INLINE hfMat5 hfMat5::operator-( const hfMat5 &a ) const {
	return hfMat5(
		hfVec5( mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2], mat[0][3] - a[0][3], mat[0][4] - a[0][4] ),
		hfVec5( mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2], mat[1][3] - a[1][3], mat[1][4] - a[1][4] ),
		hfVec5( mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2], mat[2][3] - a[2][3], mat[2][4] - a[2][4] ),
		hfVec5( mat[3][0] - a[3][0], mat[3][1] - a[3][1], mat[3][2] - a[3][2], mat[3][3] - a[3][3], mat[3][4] - a[3][4] ),
		hfVec5( mat[4][0] - a[4][0], mat[4][1] - a[4][1], mat[4][2] - a[4][2], mat[4][3] - a[4][3], mat[4][4] - a[4][4] ) );
}

HF_INLINE hfMat5 &hfMat5::operator*=( const float a ) {
	mat[0][0] *= a; mat[0][1] *= a; mat[0][2] *= a; mat[0][3] *= a; mat[0][4] *= a;
	mat[1][0] *= a; mat[1][1] *= a; mat[1][2] *= a; mat[1][3] *= a; mat[1][4] *= a;
	mat[2][0] *= a; mat[2][1] *= a; mat[2][2] *= a; mat[2][3] *= a; mat[2][4] *= a;
	mat[3][0] *= a; mat[3][1] *= a; mat[3][2] *= a; mat[3][3] *= a; mat[3][4] *= a;
	mat[4][0] *= a; mat[4][1] *= a; mat[4][2] *= a; mat[4][3] *= a; mat[4][4] *= a;
	return *this;
}

HF_INLINE hfMat5 &hfMat5::operator*=( const hfMat5 &a ) {
	*this = *this * a;
	return *this;
}

HF_INLINE hfMat5 &hfMat5::operator+=( const hfMat5 &a ) {
	mat[0][0] += a[0][0]; mat[0][1] += a[0][1]; mat[0][2] += a[0][2]; mat[0][3] += a[0][3]; mat[0][4] += a[0][4];
	mat[1][0] += a[1][0]; mat[1][1] += a[1][1]; mat[1][2] += a[1][2]; mat[1][3] += a[1][3]; mat[1][4] += a[1][4];
	mat[2][0] += a[2][0]; mat[2][1] += a[2][1]; mat[2][2] += a[2][2]; mat[2][3] += a[2][3]; mat[2][4] += a[2][4];
	mat[3][0] += a[3][0]; mat[3][1] += a[3][1]; mat[3][2] += a[3][2]; mat[3][3] += a[3][3]; mat[3][4] += a[3][4];
	mat[4][0] += a[4][0]; mat[4][1] += a[4][1]; mat[4][2] += a[4][2]; mat[4][3] += a[4][3]; mat[4][4] += a[4][4];
	return *this;
}

HF_INLINE hfMat5 &hfMat5::operator-=( const hfMat5 &a ) {
	mat[0][0] -= a[0][0]; mat[0][1] -= a[0][1]; mat[0][2] -= a[0][2]; mat[0][3] -= a[0][3]; mat[0][4] -= a[0][4];
	mat[1][0] -= a[1][0]; mat[1][1] -= a[1][1]; mat[1][2] -= a[1][2]; mat[1][3] -= a[1][3]; mat[1][4] -= a[1][4];
	mat[2][0] -= a[2][0]; mat[2][1] -= a[2][1]; mat[2][2] -= a[2][2]; mat[2][3] -= a[2][3]; mat[2][4] -= a[2][4];
	mat[3][0] -= a[3][0]; mat[3][1] -= a[3][1]; mat[3][2] -= a[3][2]; mat[3][3] -= a[3][3]; mat[3][4] -= a[3][4];
	mat[4][0] -= a[4][0]; mat[4][1] -= a[4][1]; mat[4][2] -= a[4][2]; mat[4][3] -= a[4][3]; mat[4][4] -= a[4][4];
	return *this;
}

HF_INLINE hfVec5 operator*( const hfVec5 &vec, const hfMat5 &mat ) {
	return mat * vec;
}

HF_INLINE hfMat5 operator*( const float a, hfMat5 const &mat ) {
	return mat * a;
}

HF_INLINE hfVec5 &operator*=( hfVec5 &vec, const hfMat5 &mat ) {
	vec = mat * vec;
	return vec;
}

HF_INLINE bool hfMat5::Compare( const hfMat5 &a ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 5*5; i++ ) {
		if ( ptr1[i] != ptr2[i] ) {
			return false;
		}
	}
	return true;
}

HF_INLINE bool hfMat5::Compare( const hfMat5 &a, const float epsilon ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 5*5; i++ ) {
		if ( hfMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

HF_INLINE bool hfMat5::operator==( const hfMat5 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfMat5::operator!=( const hfMat5 &a ) const {
	return !Compare( a );
}

HF_INLINE void hfMat5::Zero() {
	memset( mat, 0, sizeof( hfMat5 ) );
}

HF_INLINE void hfMat5::Identity() {
	*this = mat5_identity;
}

HF_INLINE bool hfMat5::IsIdentity( const float epsilon ) const {
	return Compare( mat5_identity, epsilon );
}

HF_INLINE bool hfMat5::IsSymmetric( const float epsilon ) const {
	for ( int i = 1; i < 5; i++ ) {
		for ( int j = 0; j < i; j++ ) {
			if ( hfMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

HF_INLINE bool hfMat5::IsDiagonal( const float epsilon ) const {
	for ( int i = 0; i < 5; i++ ) {
		for ( int j = 0; j < 5; j++ ) {
			if ( i != j && hfMath::Fabs( mat[i][j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

HF_INLINE float hfMat5::Trace() const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] + mat[4][4] );
}

HF_INLINE hfMat5 hfMat5::Inverse() const {
	hfMat5 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

HF_INLINE hfMat5 hfMat5::InverseFast() const {
	hfMat5 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

HF_INLINE int hfMat5::GetDimension() const {
	return 25;
}

HF_INLINE const float *hfMat5::ToFloatPtr() const {
	return mat[0].ToFloatPtr();
}

HF_INLINE float *hfMat5::ToFloatPtr() {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	hfMat6 - 6x6 matrix
//
//===============================================================

class hfMat6 {
public:
					hfMat6();
					explicit hfMat6( const hfVec6 &v0, const hfVec6 &v1, const hfVec6 &v2, const hfVec6 &v3, const hfVec6 &v4, const hfVec6 &v5 );
					explicit hfMat6( const hfMat3 &m0, const hfMat3 &m1, const hfMat3 &m2, const hfMat3 &m3 );
					explicit hfMat6( const float src[ 6 ][ 6 ] );

	const hfVec6 &	operator[]( int index ) const;
	hfVec6 &		operator[]( int index );
	hfMat6			operator*( const float a ) const;
	hfVec6			operator*( const hfVec6 &vec ) const;
	hfMat6			operator*( const hfMat6 &a ) const;
	hfMat6			operator+( const hfMat6 &a ) const;
	hfMat6			operator-( const hfMat6 &a ) const;
	hfMat6 &		operator*=( const float a );
	hfMat6 &		operator*=( const hfMat6 &a );
	hfMat6 &		operator+=( const hfMat6 &a );
	hfMat6 &		operator-=( const hfMat6 &a );

	friend hfMat6	operator*( const float a, const hfMat6 &mat );
	friend hfVec6	operator*( const hfVec6 &vec, const hfMat6 &mat );
	friend hfVec6 &	operator*=( hfVec6 &vec, const hfMat6 &mat );

	bool			Compare( const hfMat6 &a ) const;						// exact compare, no epsilon
	bool			Compare( const hfMat6 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const hfMat6 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const hfMat6 &a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	hfMat3			SubMat3( int n ) const;
	float			Trace() const;
	float			Determinant() const;
	hfMat6			Transpose() const;	// returns transpose
	hfMat6 &		TransposeSelf();
	hfMat6			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	hfMat6			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero

	int				GetDimension() const;

	const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	hfVec6			mat[ 6 ];
};

extern hfMat6 mat6_zero;
extern hfMat6 mat6_identity;
#define mat6_default	mat6_identity

HF_INLINE hfMat6::hfMat6() {
}

HF_INLINE hfMat6::hfMat6( const hfMat3 &m0, const hfMat3 &m1, const hfMat3 &m2, const hfMat3 &m3 ) {
	mat[0] = hfVec6( m0[0][0], m0[0][1], m0[0][2], m1[0][0], m1[0][1], m1[0][2] );
	mat[1] = hfVec6( m0[1][0], m0[1][1], m0[1][2], m1[1][0], m1[1][1], m1[1][2] );
	mat[2] = hfVec6( m0[2][0], m0[2][1], m0[2][2], m1[2][0], m1[2][1], m1[2][2] );
	mat[3] = hfVec6( m2[0][0], m2[0][1], m2[0][2], m3[0][0], m3[0][1], m3[0][2] );
	mat[4] = hfVec6( m2[1][0], m2[1][1], m2[1][2], m3[1][0], m3[1][1], m3[1][2] );
	mat[5] = hfVec6( m2[2][0], m2[2][1], m2[2][2], m3[2][0], m3[2][1], m3[2][2] );
}

HF_INLINE hfMat6::hfMat6( const hfVec6 &v0, const hfVec6 &v1, const hfVec6 &v2, const hfVec6 &v3, const hfVec6 &v4, const hfVec6 &v5 ) {
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
	mat[3] = v3;
	mat[4] = v4;
	mat[5] = v5;
}

HF_INLINE hfMat6::hfMat6( const float src[ 6 ][ 6 ] ) {
	memcpy( mat, src, 6 * 6 * sizeof( float ) );
}

HF_INLINE const hfVec6 &hfMat6::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 6 ) );
	return mat[ index ];
}

HF_INLINE hfVec6 &hfMat6::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 6 ) );
	return mat[ index ];
}

HF_INLINE hfMat6 hfMat6::operator*( const hfMat6 &a ) const {
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	hfMat6 dst;

	m1Ptr = reinterpret_cast<const float *>(this);
	m2Ptr = reinterpret_cast<const float *>(&a);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 6; i++ ) {
		for ( j = 0; j < 6; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 6 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 6 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 6 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 6 + j ]
					+ m1Ptr[4] * m2Ptr[ 4 * 6 + j ]
					+ m1Ptr[5] * m2Ptr[ 5 * 6 + j ];
			dstPtr++;
		}
		m1Ptr += 6;
	}
	return dst;
}

HF_INLINE hfMat6 hfMat6::operator*( const float a ) const {
	return hfMat6(
		hfVec6( mat[0][0] * a, mat[0][1] * a, mat[0][2] * a, mat[0][3] * a, mat[0][4] * a, mat[0][5] * a ),
		hfVec6( mat[1][0] * a, mat[1][1] * a, mat[1][2] * a, mat[1][3] * a, mat[1][4] * a, mat[1][5] * a ),
		hfVec6( mat[2][0] * a, mat[2][1] * a, mat[2][2] * a, mat[2][3] * a, mat[2][4] * a, mat[2][5] * a ),
		hfVec6( mat[3][0] * a, mat[3][1] * a, mat[3][2] * a, mat[3][3] * a, mat[3][4] * a, mat[3][5] * a ),
		hfVec6( mat[4][0] * a, mat[4][1] * a, mat[4][2] * a, mat[4][3] * a, mat[4][4] * a, mat[4][5] * a ),
		hfVec6( mat[5][0] * a, mat[5][1] * a, mat[5][2] * a, mat[5][3] * a, mat[5][4] * a, mat[5][5] * a ) );
}

HF_INLINE hfVec6 hfMat6::operator*( const hfVec6 &vec ) const {
	return hfVec6(
		mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3] + mat[0][4] * vec[4] + mat[0][5] * vec[5],
		mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3] + mat[1][4] * vec[4] + mat[1][5] * vec[5],
		mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3] + mat[2][4] * vec[4] + mat[2][5] * vec[5],
		mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3] + mat[3][4] * vec[4] + mat[3][5] * vec[5],
		mat[4][0] * vec[0] + mat[4][1] * vec[1] + mat[4][2] * vec[2] + mat[4][3] * vec[3] + mat[4][4] * vec[4] + mat[4][5] * vec[5],
		mat[5][0] * vec[0] + mat[5][1] * vec[1] + mat[5][2] * vec[2] + mat[5][3] * vec[3] + mat[5][4] * vec[4] + mat[5][5] * vec[5] );
}

HF_INLINE hfMat6 hfMat6::operator+( const hfMat6 &a ) const {
	return hfMat6(
		hfVec6( mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2], mat[0][3] + a[0][3], mat[0][4] + a[0][4], mat[0][5] + a[0][5] ),
		hfVec6( mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2], mat[1][3] + a[1][3], mat[1][4] + a[1][4], mat[1][5] + a[1][5] ),
		hfVec6( mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2], mat[2][3] + a[2][3], mat[2][4] + a[2][4], mat[2][5] + a[2][5] ),
		hfVec6( mat[3][0] + a[3][0], mat[3][1] + a[3][1], mat[3][2] + a[3][2], mat[3][3] + a[3][3], mat[3][4] + a[3][4], mat[3][5] + a[3][5] ),
		hfVec6( mat[4][0] + a[4][0], mat[4][1] + a[4][1], mat[4][2] + a[4][2], mat[4][3] + a[4][3], mat[4][4] + a[4][4], mat[4][5] + a[4][5] ),
		hfVec6( mat[5][0] + a[5][0], mat[5][1] + a[5][1], mat[5][2] + a[5][2], mat[5][3] + a[5][3], mat[5][4] + a[5][4], mat[5][5] + a[5][5] ) );
}

HF_INLINE hfMat6 hfMat6::operator-( const hfMat6 &a ) const {
	return hfMat6(
		hfVec6( mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2], mat[0][3] - a[0][3], mat[0][4] - a[0][4], mat[0][5] - a[0][5] ),
		hfVec6( mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2], mat[1][3] - a[1][3], mat[1][4] - a[1][4], mat[1][5] - a[1][5] ),
		hfVec6( mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2], mat[2][3] - a[2][3], mat[2][4] - a[2][4], mat[2][5] - a[2][5] ),
		hfVec6( mat[3][0] - a[3][0], mat[3][1] - a[3][1], mat[3][2] - a[3][2], mat[3][3] - a[3][3], mat[3][4] - a[3][4], mat[3][5] - a[3][5] ),
		hfVec6( mat[4][0] - a[4][0], mat[4][1] - a[4][1], mat[4][2] - a[4][2], mat[4][3] - a[4][3], mat[4][4] - a[4][4], mat[4][5] - a[4][5] ),
		hfVec6( mat[5][0] - a[5][0], mat[5][1] - a[5][1], mat[5][2] - a[5][2], mat[5][3] - a[5][3], mat[5][4] - a[5][4], mat[5][5] - a[5][5] ) );
}

HF_INLINE hfMat6 &hfMat6::operator*=( const float a ) {
	mat[0][0] *= a; mat[0][1] *= a; mat[0][2] *= a; mat[0][3] *= a; mat[0][4] *= a; mat[0][5] *= a;
	mat[1][0] *= a; mat[1][1] *= a; mat[1][2] *= a; mat[1][3] *= a; mat[1][4] *= a; mat[1][5] *= a;
	mat[2][0] *= a; mat[2][1] *= a; mat[2][2] *= a; mat[2][3] *= a; mat[2][4] *= a; mat[2][5] *= a;
	mat[3][0] *= a; mat[3][1] *= a; mat[3][2] *= a; mat[3][3] *= a; mat[3][4] *= a; mat[3][5] *= a;
	mat[4][0] *= a; mat[4][1] *= a; mat[4][2] *= a; mat[4][3] *= a; mat[4][4] *= a; mat[4][5] *= a;
	mat[5][0] *= a; mat[5][1] *= a; mat[5][2] *= a; mat[5][3] *= a; mat[5][4] *= a; mat[5][5] *= a;
	return *this;
}

HF_INLINE hfMat6 &hfMat6::operator*=( const hfMat6 &a ) {
	*this = *this * a;
	return *this;
}

HF_INLINE hfMat6 &hfMat6::operator+=( const hfMat6 &a ) {
	mat[0][0] += a[0][0]; mat[0][1] += a[0][1]; mat[0][2] += a[0][2]; mat[0][3] += a[0][3]; mat[0][4] += a[0][4]; mat[0][5] += a[0][5];
	mat[1][0] += a[1][0]; mat[1][1] += a[1][1]; mat[1][2] += a[1][2]; mat[1][3] += a[1][3]; mat[1][4] += a[1][4]; mat[1][5] += a[1][5];
	mat[2][0] += a[2][0]; mat[2][1] += a[2][1]; mat[2][2] += a[2][2]; mat[2][3] += a[2][3]; mat[2][4] += a[2][4]; mat[2][5] += a[2][5];
	mat[3][0] += a[3][0]; mat[3][1] += a[3][1]; mat[3][2] += a[3][2]; mat[3][3] += a[3][3]; mat[3][4] += a[3][4]; mat[3][5] += a[3][5];
	mat[4][0] += a[4][0]; mat[4][1] += a[4][1]; mat[4][2] += a[4][2]; mat[4][3] += a[4][3]; mat[4][4] += a[4][4]; mat[4][5] += a[4][5];
	mat[5][0] += a[5][0]; mat[5][1] += a[5][1]; mat[5][2] += a[5][2]; mat[5][3] += a[5][3]; mat[5][4] += a[5][4]; mat[5][5] += a[5][5];
	return *this;
}

HF_INLINE hfMat6 &hfMat6::operator-=( const hfMat6 &a ) {
	mat[0][0] -= a[0][0]; mat[0][1] -= a[0][1]; mat[0][2] -= a[0][2]; mat[0][3] -= a[0][3]; mat[0][4] -= a[0][4]; mat[0][5] -= a[0][5];
	mat[1][0] -= a[1][0]; mat[1][1] -= a[1][1]; mat[1][2] -= a[1][2]; mat[1][3] -= a[1][3]; mat[1][4] -= a[1][4]; mat[1][5] -= a[1][5];
	mat[2][0] -= a[2][0]; mat[2][1] -= a[2][1]; mat[2][2] -= a[2][2]; mat[2][3] -= a[2][3]; mat[2][4] -= a[2][4]; mat[2][5] -= a[2][5];
	mat[3][0] -= a[3][0]; mat[3][1] -= a[3][1]; mat[3][2] -= a[3][2]; mat[3][3] -= a[3][3]; mat[3][4] -= a[3][4]; mat[3][5] -= a[3][5];
	mat[4][0] -= a[4][0]; mat[4][1] -= a[4][1]; mat[4][2] -= a[4][2]; mat[4][3] -= a[4][3]; mat[4][4] -= a[4][4]; mat[4][5] -= a[4][5];
	mat[5][0] -= a[5][0]; mat[5][1] -= a[5][1]; mat[5][2] -= a[5][2]; mat[5][3] -= a[5][3]; mat[5][4] -= a[5][4]; mat[5][5] -= a[5][5];
	return *this;
}

HF_INLINE hfVec6 operator*( const hfVec6 &vec, const hfMat6 &mat ) {
	return mat * vec;
}

HF_INLINE hfMat6 operator*( const float a, hfMat6 const &mat ) {
	return mat * a;
}

HF_INLINE hfVec6 &operator*=( hfVec6 &vec, const hfMat6 &mat ) {
	vec = mat * vec;
	return vec;
}

HF_INLINE bool hfMat6::Compare( const hfMat6 &a ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 6*6; i++ ) {
		if ( ptr1[i] != ptr2[i] ) {
			return false;
		}
	}
	return true;
}

HF_INLINE bool hfMat6::Compare( const hfMat6 &a, const float epsilon ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 6*6; i++ ) {
		if ( hfMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

HF_INLINE bool hfMat6::operator==( const hfMat6 &a ) const {
	return Compare( a );
}

HF_INLINE bool hfMat6::operator!=( const hfMat6 &a ) const {
	return !Compare( a );
}

HF_INLINE void hfMat6::Zero() {
	memset( mat, 0, sizeof( hfMat6 ) );
}

HF_INLINE void hfMat6::Identity() {
	*this = mat6_identity;
}

HF_INLINE bool hfMat6::IsIdentity( const float epsilon ) const {
	return Compare( mat6_identity, epsilon );
}

HF_INLINE bool hfMat6::IsSymmetric( const float epsilon ) const {
	for ( int i = 1; i < 6; i++ ) {
		for ( int j = 0; j < i; j++ ) {
			if ( hfMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

HF_INLINE bool hfMat6::IsDiagonal( const float epsilon ) const {
	for ( int i = 0; i < 6; i++ ) {
		for ( int j = 0; j < 6; j++ ) {
			if ( i != j && hfMath::Fabs( mat[i][j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

HF_INLINE hfMat3 hfMat6::SubMat3( int n ) const {
	assert( n >= 0 && n < 4 );
	int b0 = ((n & 2) >> 1) * 3;
	int b1 = (n & 1) * 3;
	return hfMat3(
		mat[b0 + 0][b1 + 0], mat[b0 + 0][b1 + 1], mat[b0 + 0][b1 + 2],
		mat[b0 + 1][b1 + 0], mat[b0 + 1][b1 + 1], mat[b0 + 1][b1 + 2],
		mat[b0 + 2][b1 + 0], mat[b0 + 2][b1 + 1], mat[b0 + 2][b1 + 2] );
}

HF_INLINE float hfMat6::Trace() const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] + mat[4][4] + mat[5][5] );
}

HF_INLINE hfMat6 hfMat6::Inverse() const {
	hfMat6 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

HF_INLINE hfMat6 hfMat6::InverseFast() const {
	hfMat6 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

HF_INLINE int hfMat6::GetDimension() const {
	return 36;
}

HF_INLINE const float *hfMat6::ToFloatPtr() const {
	return mat[0].ToFloatPtr();
}

HF_INLINE float *hfMat6::ToFloatPtr() {
	return mat[0].ToFloatPtr();
}

#endif /* !__MATH_MATRIX_H__ */

