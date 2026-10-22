#ifndef __RENDERMATRIX_H__
#define __RENDERMATRIX_H__

static const int NUM_FRUSTUM_CORNERS	= 8;

struct frustumCorners_t {
	float	x[NUM_FRUSTUM_CORNERS];
	float	y[NUM_FRUSTUM_CORNERS];
	float	z[NUM_FRUSTUM_CORNERS];
};

enum frustumCull_t {
	FRUSTUM_CULL_FRONT		= 1,
	FRUSTUM_CULL_BACK		= 2,
	FRUSTUM_CULL_CROSS		= 3
};

/*
================================================================================================

hfRenderMatrix

This is a row-major matrix and transforms are applied with left-multiplication.

================================================================================================
*/
class hfRenderMatrix {
public:
							hfRenderMatrix() {}
	HF_INLINE				hfRenderMatrix(	float a0, float a1, float a2, float a3,
											float b0, float b1, float b2, float b3,
											float c0, float c1, float c2, float c3,
											float d0, float d1, float d2, float d3 );

	const float *			operator[]( int index ) const { assert( index >= 0 && index < 4 ); return &m[index*4]; }
	float *					operator[]( int index ) { assert( index >= 0 && index < 4 ); return &m[index*4]; }

	void					Zero() { memset( m, 0, sizeof( m ) ); }
	HF_INLINE void			Identity();

	// Matrix classification (only meant to be used for asserts).
	HF_INLINE bool			IsZero( float epsilon ) const;
	HF_INLINE bool			IsIdentity( float epsilon ) const;
	HF_INLINE bool			IsAffineTransform( float epsilon ) const;
	HF_INLINE bool			IsUniformScale( float epsilon ) const;

	// Transform a point.
	// NOTE: the hfVec3 out variant does not divide by W.
	HF_INLINE void			TransformPoint( const hfVec3 & in, hfVec3 & out ) const;
	HF_INLINE void			TransformPoint( const hfVec3 & in, hfVec4 & out ) const;
	HF_INLINE void			TransformPoint( const hfVec4 & in, hfVec4 & out ) const;

	// These assume the matrix has no non-uniform scaling or shearing.
	// NOTE: a direction will only stay normalized if the matrix has no skewing or scaling.
	HF_INLINE void			TransformDir( const hfVec3 & in, hfVec3 & out, bool normalize ) const;
	HF_INLINE void			TransformPlane( const hfPlane & in, hfPlane & out, bool normalize ) const;

	// These transforms work with non-uniform scaling and shearing by multiplying
	// with 'transpose(inverse(M))' where this matrix is assumed to be 'inverse(M)'.
	HF_INLINE void			InverseTransformDir( const hfVec3 & in, hfVec3 & out, bool normalize ) const;
	HF_INLINE void			InverseTransformPlane( const hfPlane & in, hfPlane & out, bool normalize ) const;

	// Project a point.
	static HF_INLINE void	TransformModelToClip( const hfVec3 & src, const hfRenderMatrix & modelMatrix, const hfRenderMatrix & projectionMatrix, hfVec4 & eye, hfVec4 & clip );
	static HF_INLINE void	TransformClipToDevice( const hfVec4 & clip, hfVec3 & ndc );

	// Create a matrix that goes from local space to the space defined by the 'origin' and 'axis'.
	static void				CreateFromOriginAxis( const hfVec3 & origin, const hfMat3 & axis, hfRenderMatrix & out );
	static void				CreateFromOriginAxisScale( const hfVec3 & origin, const hfMat3 & axis, const hfVec3 & scale, hfRenderMatrix & out );

	// Create a matrix that goes from a global coordinate to a view coordinate (OpenGL looking down -Z) based on the given view origin/axis.
	static void				CreateViewMatrix( const hfVec3 & origin, const hfMat3 & axis, hfRenderMatrix & out );

	// Create a projection matrix.
	static void				CreateProjectionMatrix( float xMin, float xMax, float yMin, float yMax, float zNear, float zFar, hfRenderMatrix & out );
	static void				CreateProjectionMatrixFov( float xFovDegrees, float yFovDegrees, float zNear, float zFar, float xOffset, float yOffset, hfRenderMatrix & out );

	// Apply depth hacks to a projection matrix.
	static HF_INLINE void	ApplyDepthHack( hfRenderMatrix & src );
	static HF_INLINE void	ApplyModelDepthHack( hfRenderMatrix & src, float value );

	// Offset and scale the given matrix such that the result matrix transforms the unit-cube to exactly cover the given bounds (and the inverse).
	static void				OffsetScaleForBounds( const hfRenderMatrix & src, const hfBounds & bounds, hfRenderMatrix & out );
	static void				InverseOffsetScaleForBounds( const hfRenderMatrix & src, const hfBounds & bounds, hfRenderMatrix & out );

	// Basic matrix operations.
	static void				Transpose( const hfRenderMatrix & src, hfRenderMatrix & out );
	static void				Multiply( const hfRenderMatrix & a, const hfRenderMatrix & b, hfRenderMatrix & out );
	static bool				Inverse( const hfRenderMatrix & src, hfRenderMatrix & out );
	static void				InverseByTranspose( const hfRenderMatrix & src, hfRenderMatrix & out );
	static bool				InverseByDoubles( const hfRenderMatrix & src, hfRenderMatrix & out );

	// Copy or create a matrix that is stored directly into four float4 vectors which is useful for directly setting vertex program uniforms.
	static void				CopyMatrix( const hfRenderMatrix & matrix, hfVec4 & row0, hfVec4 & row1, hfVec4 & row2, hfVec4 & row3 );
	static void				SetMVP( const hfRenderMatrix & mvp, hfVec4 & row0, hfVec4 & row1, hfVec4 & row2, hfVec4 & row3, bool & negativeDeterminant );
	static void				SetMVPForBounds( const hfRenderMatrix & mvp, const hfBounds & bounds, hfVec4 & row0, hfVec4 & row1, hfVec4 & row2, hfVec4 & row3, bool & negativeDeterminant );
	static void				SetMVPForInverseProject( const hfRenderMatrix & mvp, const hfRenderMatrix & inverseProject, hfVec4 & row0, hfVec4 & row1, hfVec4 & row2, hfVec4 & row3, bool & negativeDeterminant );

	// Cull to a Model-View-Projection (MVP) matrix.
	static bool				CullPointToMVP( const hfRenderMatrix & mvp, const hfVec3 & point, bool zeroToOne = false );
	static bool				CullPointToMVPbits( const hfRenderMatrix & mvp, const hfVec3 & point, byte * outBits, bool zeroToOne = false );
	static bool				CullBoundsToMVP( const hfRenderMatrix & mvp, const hfBounds & bounds, bool zeroToOne = false );
	static bool				CullBoundsToMVPbits( const hfRenderMatrix & mvp, const hfBounds & bounds, byte * outBits, bool zeroToOne = false );
	static bool				CullExtrudedBoundsToMVP( const hfRenderMatrix & mvp, const hfBounds & bounds, const hfVec3 & extrudeDirection, const hfPlane & clipPlane, bool zeroToOne = false );
	static bool				CullExtrudedBoundsToMVPbits( const hfRenderMatrix & mvp, const hfBounds & bounds, const hfVec3 & extrudeDirection, const hfPlane & clipPlane, byte * outBits, bool zeroToOne = false );

	// Calculate the projected bounds.
	static void				ProjectedBounds( hfBounds & projected, const hfRenderMatrix & mvp, const hfBounds & bounds, bool windowSpace = true );
	static void				ProjectedNearClippedBounds( hfBounds & projected, const hfRenderMatrix & mvp, const hfBounds & bounds, bool windowSpace = true );
	static void				ProjectedFullyClippedBounds( hfBounds & projected, const hfRenderMatrix & mvp, const hfBounds & bounds, bool windowSpace = true );

	// Calculate the projected depth bounds.
	static void				DepthBoundsForBounds( float & min, float & max, const hfRenderMatrix & mvp, const hfBounds & bounds, bool windowSpace = true );
	static void				DepthBoundsForExtrudedBounds( float & min, float & max, const hfRenderMatrix & mvp, const hfBounds & bounds, const hfVec3 & extrudeDirection, const hfPlane & clipPlane, bool windowSpace = true );
	static void				DepthBoundsForShadowBounds( float & min, float & max, const hfRenderMatrix & mvp, const hfBounds & bounds, const hfVec3 & localLightOrigin, bool windowSpace = true );

	// Create frustum planes and corners from a matrix.
	static void				GetFrustumPlanes( hfPlane planes[6], const hfRenderMatrix & frustum, bool zeroToOne, bool normalize );
	static void				GetFrustumCorners( frustumCorners_t & corners, const hfRenderMatrix & frustumTransform, const hfBounds & frustumBounds );
	static frustumCull_t	CullFrustumCornersToPlane( const frustumCorners_t & corners, const hfPlane & plane );

private:
	float					m[16];
};

extern const hfRenderMatrix renderMatrix_identity;
extern const hfRenderMatrix renderMatrix_flipToOpenGL;
extern const hfRenderMatrix renderMatrix_windowSpaceToClipSpace;

/*
========================
hfRenderMatrix::hfRenderMatrix
========================
*/
HF_INLINE hfRenderMatrix::hfRenderMatrix(	float a0, float a1, float a2, float a3,
											float b0, float b1, float b2, float b3,
											float c0, float c1, float c2, float c3,
											float d0, float d1, float d2, float d3 ) {
	m[0*4+0] = a0; m[0*4+1] = a1; m[0*4+2] = a2; m[0*4+3] = a3;
	m[1*4+0] = b0; m[1*4+1] = b1; m[1*4+2] = b2; m[1*4+3] = b3;
	m[2*4+0] = c0; m[2*4+1] = c1; m[2*4+2] = c2; m[2*4+3] = c3;
	m[3*4+0] = d0; m[3*4+1] = d1; m[3*4+2] = d2; m[3*4+3] = d3;
}

/*
========================
hfRenderMatrix::Identity
========================
*/
HF_INLINE void hfRenderMatrix::Identity() {
	m[0*4+0] = 1.0f;
	m[0*4+1] = 0.0f;
	m[0*4+2] = 0.0f;
	m[0*4+3] = 0.0f;

	m[1*4+0] = 0.0f;
	m[1*4+1] = 1.0f;
	m[1*4+2] = 0.0f;
	m[1*4+3] = 0.0f;

	m[2*4+0] = 0.0f;
	m[2*4+1] = 0.0f;
	m[2*4+2] = 1.0f;
	m[2*4+3] = 0.0f;

	m[3*4+0] = 0.0f;
	m[3*4+1] = 0.0f;
	m[3*4+2] = 0.0f;
	m[3*4+3] = 1.0f;
}

/*
========================
hfRenderMatrix::IsZero
========================
*/
HF_INLINE bool hfRenderMatrix::IsZero( float epsilon ) const {
	for ( int i = 0; i < 16; i++ ) {
		if ( hfMath::Fabs( m[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfRenderMatrix::IsIdentity
========================
*/
HF_INLINE bool hfRenderMatrix::IsIdentity( float epsilon ) const {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			if ( i == j ) {
				if ( hfMath::Fabs( m[i * 4 + j] - 1.0f ) > epsilon ) {
					return false;
				}
			} else {
				if ( hfMath::Fabs( m[i * 4 + j] ) > epsilon ) {
					return false;
				}
			}
		}
	}
	return true;
}

/*
========================
hfRenderMatrix::IsAffineTransform
========================
*/
HF_INLINE bool hfRenderMatrix::IsAffineTransform( float epsilon ) const {
	if ( hfMath::Fabs( m[3 * 4 + 0] ) > epsilon ||
			hfMath::Fabs( m[3 * 4 + 1] ) > epsilon ||
				hfMath::Fabs( m[3 * 4 + 2] ) > epsilon ||
					hfMath::Fabs( m[3 * 4 + 3] - 1.0f ) > epsilon ) {
		return false;
	}
	return true;
}

/*
========================
hfRenderMatrix::IsUniformScale
========================
*/
HF_INLINE bool hfRenderMatrix::IsUniformScale( float epsilon ) const {
	float d0 = hfMath::InvSqrt( m[0*4+0] * m[0*4+0] + m[1*4+0] * m[1*4+0] + m[2*4+0] * m[2*4+0] );
	float d1 = hfMath::InvSqrt( m[0*4+1] * m[0*4+1] + m[1*4+1] * m[1*4+1] + m[2*4+1] * m[2*4+1] );
	float d2 = hfMath::InvSqrt( m[0*4+2] * m[0*4+2] + m[1*4+2] * m[1*4+2] + m[2*4+2] * m[2*4+2] );
	if ( hfMath::Fabs( d0 - d1 ) > epsilon ) { return false; }
	if ( hfMath::Fabs( d1 - d2 ) > epsilon ) { return false; }
	if ( hfMath::Fabs( d0 - d2 ) > epsilon ) { return false; }
	return true;
}

/*
========================
hfRenderMatrix::TransformPoint
========================
*/
HF_INLINE void hfRenderMatrix::TransformPoint( const hfVec3 & in, hfVec3 & out ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const hfRenderMatrix & matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + matrix[2][3];
	assert( hfMath::Fabs( in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + matrix[3][3] - 1.0f ) < 0.01f );
}

/*
========================
hfRenderMatrix::TransformPoint
========================
*/
HF_INLINE void hfRenderMatrix::TransformPoint( const hfVec3 & in, hfVec4 & out ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const hfRenderMatrix & matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + matrix[2][3];
	out[3] = in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + matrix[3][3];
}

/*
========================
hfRenderMatrix::TransformPoint
========================
*/
HF_INLINE void hfRenderMatrix::TransformPoint( const hfVec4 & in, hfVec4 & out ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const hfRenderMatrix & matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + in[3] * matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + in[3] * matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + in[3] * matrix[2][3];
	out[3] = in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + in[3] * matrix[3][3];
}

/*
========================
hfRenderMatrix::TransformDir
========================
*/
HF_INLINE void hfRenderMatrix::TransformDir( const hfVec3 & in, hfVec3 & out, bool normalize ) const {
	const hfRenderMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2];
	float p1 = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2];
	float p2 = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2];
	if ( normalize ) {
		float r = hfMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
}

/*
========================
hfRenderMatrix::TransformPlane
========================
*/
HF_INLINE void hfRenderMatrix::TransformPlane( const hfPlane & in, hfPlane & out, bool normalize ) const {
	assert( IsUniformScale( 0.01f ) );
	const hfRenderMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2];
	float p1 = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2];
	float p2 = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2];
	float d0 = matrix[0][3] - p0 * in[3];
	float d1 = matrix[1][3] - p1 * in[3];
	float d2 = matrix[2][3] - p2 * in[3];
	if ( normalize ) {
		float r = hfMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
	out[3] = - p0 * d0 - p1 * d1 - p2 * d2;
}

/*
========================
hfRenderMatrix::InverseTransformDir
========================
*/
HF_INLINE void hfRenderMatrix::InverseTransformDir( const hfVec3 & in, hfVec3 & out, bool normalize ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const hfRenderMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0];
	float p1 = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1];
	float p2 = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2];
	if ( normalize ) {
		float r = hfMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
}

/*
========================
hfRenderMatrix::InverseTransformPlane
========================
*/
HF_INLINE void hfRenderMatrix::InverseTransformPlane( const hfPlane & in, hfPlane & out, bool normalize ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const hfRenderMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0] + in[3] * matrix[3][0];
	float p1 = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1] + in[3] * matrix[3][1];
	float p2 = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2] + in[3] * matrix[3][2];
	float p3 = in[0] * matrix[0][3] + in[1] * matrix[1][3] + in[2] * matrix[2][3] + in[3] * matrix[3][3];
	if ( normalize ) {
		float r = hfMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
		p3 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
	out[3] = p3;
}

/*
========================
hfRenderMatrix::TransformModelToClip
========================
*/
HF_INLINE void hfRenderMatrix::TransformModelToClip( const hfVec3 & src, const hfRenderMatrix & modelMatrix, const hfRenderMatrix & projectionMatrix, hfVec4 & eye, hfVec4 & clip ) {
	for ( int i = 0; i < 4; i++ ) {
		eye[i] =	modelMatrix[i][0] * src[0] +
					modelMatrix[i][1] * src[1] +
					modelMatrix[i][2] * src[2] +
					modelMatrix[i][3];
	}
	for ( int i = 0; i < 4; i++ ) {
		clip[i] =	projectionMatrix[i][0] * eye[0] +
					projectionMatrix[i][1] * eye[1] +
					projectionMatrix[i][2] * eye[2] +
					projectionMatrix[i][3] * eye[3];
	}
}

/*
========================
hfRenderMatrix::TransformClipToDevice

Clip to normalized device coordinates.
========================
*/
HF_INLINE void hfRenderMatrix::TransformClipToDevice( const hfVec4 & clip, hfVec3 & ndc ) {
	assert( hfMath::Fabs( clip[3] ) > hfMath::FLT_SMALLEST_NON_DENORMAL );
	float r = 1.0f / clip[3];
	ndc[0] = clip[0] * r;
	ndc[1] = clip[1] * r;
	ndc[2] = clip[2] * r;
}

/*
========================
hfRenderMatrix::ApplyDepthHack
========================
*/
HF_INLINE void hfRenderMatrix::ApplyDepthHack( hfRenderMatrix & src ) {
	// scale projected z by 25%
	src.m[2*4+0] *= 0.25f;
	src.m[2*4+1] *= 0.25f;
	src.m[2*4+2] *= 0.25f;
	src.m[2*4+3] *= 0.25f;
}

/*
========================
hfRenderMatrix::ApplyModelDepthHack
========================
*/
HF_INLINE void hfRenderMatrix::ApplyModelDepthHack( hfRenderMatrix & src, float value ) {
	// offset projected z
	src.m[2*4+3] -= value;
}

/*
========================
hfRenderMatrix::CullPointToMVP
========================
*/
HF_INLINE bool hfRenderMatrix::CullPointToMVP( const hfRenderMatrix & mvp, const hfVec3 & point, bool zeroToOne ) {
	byte bits;
	return CullPointToMVPbits( mvp, point, &bits, zeroToOne );
}

/*
========================
hfRenderMatrix::CullBoundsToMVP
========================
*/
HF_INLINE bool hfRenderMatrix::CullBoundsToMVP( const hfRenderMatrix & mvp, const hfBounds & bounds, bool zeroToOne ) {
	byte bits;
	return CullBoundsToMVPbits( mvp, bounds, &bits, zeroToOne );
}

/*
========================
hfRenderMatrix::CullExtrudedBoundsToMVP
========================
*/
HF_INLINE bool hfRenderMatrix::CullExtrudedBoundsToMVP( const hfRenderMatrix & mvp, const hfBounds & bounds, const hfVec3 & extrudeDirection, const hfPlane & clipPlane, bool zeroToOne ) {
	byte bits;
	return CullExtrudedBoundsToMVPbits( mvp, bounds, extrudeDirection, clipPlane, &bits, zeroToOne );
}

#endif // !__RENDERMATRIX_H__

