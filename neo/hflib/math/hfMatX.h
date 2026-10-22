#ifndef __MATH_MATX_H__
#define __MATH_MATX_H__

/*
===============================================================================

hfMatX - arbitrary sized dense real matrix

The matrix lives on 16 byte aligned and 16 byte padded memory.

NOTE: due to the temporary memory pool hfMatX cannot be used by multiple threads.

===============================================================================
*/

#define MATX_MAX_TEMP		1024
#define MATX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define MATX_CLEAREND()		int s = numRows * numColumns; while( s < ( ( s + 3 ) & ~3 ) ) { mat[s++] = 0.0f; }
#define MATX_ALLOCA( n )	( (float *) _alloca16( MATX_QUAD( n ) ) )
#define MATX_ALLOCA_CACHE_LINES( n )	( (float *) _alloca128( ( ( n ) * sizeof( float ) + CACHE_LINE_SIZE - 1 ) & ~ ( CACHE_LINE_SIZE - 1 ) ) )
#define MATX_SIMD

class hfMatX {
public:
	HF_INLINE					hfMatX();
	HF_INLINE					hfMatX( const hfMatX & other );
	HF_INLINE					explicit hfMatX( int rows, int columns );
	HF_INLINE					explicit hfMatX( int rows, int columns, float *src );
	HF_INLINE					~hfMatX();

	HF_INLINE	void			Set( int rows, int columns, const float *src );
	HF_INLINE	void			Set( const hfMat3 &m1, const hfMat3 &m2 );
	HF_INLINE	void			Set( const hfMat3 &m1, const hfMat3 &m2, const hfMat3 &m3, const hfMat3 &m4 );

	HF_INLINE	const float *	operator[]( int index ) const;
	HF_INLINE	float *			operator[]( int index );
	HF_INLINE	hfMatX &		operator=( const hfMatX &a );
	HF_INLINE	hfMatX			operator*( const float a ) const;
	HF_INLINE	hfVecX			operator*( const hfVecX &vec ) const;
	HF_INLINE	hfMatX			operator*( const hfMatX &a ) const;
	HF_INLINE	hfMatX			operator+( const hfMatX &a ) const;
	HF_INLINE	hfMatX			operator-( const hfMatX &a ) const;
	HF_INLINE	hfMatX &		operator*=( const float a );
	HF_INLINE	hfMatX &		operator*=( const hfMatX &a );
	HF_INLINE	hfMatX &		operator+=( const hfMatX &a );
	HF_INLINE	hfMatX &		operator-=( const hfMatX &a );

	friend HF_INLINE	hfMatX	operator*( const float a, const hfMatX &m );
	friend HF_INLINE	hfVecX	operator*( const hfVecX &vec, const hfMatX &m );
	friend HF_INLINE	hfVecX &operator*=( hfVecX &vec, const hfMatX &m );

	HF_INLINE	bool			Compare( const hfMatX &a ) const;									// exact compare, no epsilon
	HF_INLINE	bool			Compare( const hfMatX &a, const float epsilon ) const;				// compare with epsilon
	HF_INLINE	bool			operator==( const hfMatX &a ) const;								// exact compare, no epsilon
	HF_INLINE	bool			operator!=( const hfMatX &a ) const;								// exact compare, no epsilon

	HF_INLINE	void			SetSize( int rows, int columns );									// set the number of rows/columns
	void			ChangeSize( int rows, int columns, bool makeZero = false );		// change the size keeping data intact where possible
	HF_INLINE	void			ChangeNumRows( int rows ) { ChangeSize( rows, numColumns );	}		// set the number of rows/columns
	int				GetNumRows() const { return numRows; }					// get the number of rows
	int				GetNumColumns() const { return numColumns; }				// get the number of columns
	HF_INLINE	void			SetData( int rows, int columns, float *data );						// set float array pointer
	HF_INLINE	void			SetDataCacheLines( int rows, int columns, float *data, bool clear );// set float array pointer
	HF_INLINE	void			Zero();																// clear matrix
	HF_INLINE	void			Zero( int rows, int columns );										// set size and clear matrix
	HF_INLINE	void			Identity();															// clear to identity matrix
	HF_INLINE	void			Identity( int rows, int columns );									// set size and clear to identity matrix
	HF_INLINE	void			Diag( const hfVecX &v );											// create diagonal matrix from vector
	HF_INLINE	void			Random( int seed, float l = 0.0f, float u = 1.0f );					// fill matrix with random values
	HF_INLINE	void			Random( int rows, int columns, int seed, float l = 0.0f, float u = 1.0f );
	HF_INLINE	void			Negate();															// (*this) = - (*this)
	HF_INLINE	void			Clamp( float min, float max );										// clamp all values
	HF_INLINE	hfMatX &		SwapRows( int r1, int r2 );											// swap rows
	HF_INLINE	hfMatX &		SwapColumns( int r1, int r2 );										// swap columns
	HF_INLINE	hfMatX &		SwapRowsColumns( int r1, int r2 );									// swap rows and columns
	hfMatX &		RemoveRow( int r );												// remove a row
	hfMatX &		RemoveColumn( int r );											// remove a column
	hfMatX &		RemoveRowColumn( int r );										// remove a row and column
	HF_INLINE	void			ClearUpperTriangle();												// clear the upper triangle
	HF_INLINE	void			ClearLowerTriangle();												// clear the lower triangle
	void			CopyLowerToUpperTriangle();											// copy the lower triangle to the upper triangle
	HF_INLINE	void			SquareSubMatrix( const hfMatX &m, int size );						// get square sub-matrix from 0,0 to size,size
	HF_INLINE	float			MaxDifference( const hfMatX &m ) const;								// return maximum element difference between this and m

	HF_INLINE	bool			IsSquare() const { return ( numRows == numColumns ); }
	HF_INLINE	bool			IsZero( const float epsilon = MATRIX_EPSILON ) const;
	HF_INLINE	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	HF_INLINE	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	HF_INLINE	bool			IsTriDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	HF_INLINE	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsOrthogonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsOrthonormal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPMatrix( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsZMatrix( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPositiveDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetricPositiveDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPositiveSemiDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetricPositiveSemiDefinite( const float epsilon = MATRIX_EPSILON ) const;

	HF_INLINE	float			Trace() const;													// returns product of diagonal elements
	HF_INLINE	float			Determinant() const;											// returns determinant of matrix
	HF_INLINE	hfMatX			Transpose() const;												// returns transpose
	HF_INLINE	hfMatX &		TransposeSelf();												// transposes the matrix itself
	HF_INLINE	void			Transpose( hfMatX & dst ) const;								// stores transpose in 'dst'
	HF_INLINE	hfMatX			Inverse() const;												// returns the inverse ( m * m.Inverse() = identity )
	HF_INLINE	bool			InverseSelf();													// returns false if determinant is zero
	HF_INLINE	hfMatX			InverseFast() const;											// returns the inverse ( m * m.Inverse() = identity )
	HF_INLINE	bool			InverseFastSelf();												// returns false if determinant is zero
	HF_INLINE	void			Inverse( hfMatX & dst ) const;									// stores the inverse in 'dst' ( m * m.Inverse() = identity )

	bool			LowerTriangularInverse();									// in-place inversion, returns false if determinant is zero
	bool			UpperTriangularInverse();									// in-place inversion, returns false if determinant is zero

	HF_INLINE	void			Subtract( const hfMatX & a );									// (*this) -= a;

	HF_INLINE	hfVecX			Multiply( const hfVecX &vec ) const;							// (*this) * vec
	HF_INLINE	hfVecX			TransposeMultiply( const hfVecX &vec ) const;					// this->Transpose() * vec

	HF_INLINE	hfMatX			Multiply( const hfMatX &a ) const;								// (*this) * a
	HF_INLINE	hfMatX			TransposeMultiply( const hfMatX &a ) const;						// this->Transpose() * a

	HF_INLINE	void			Multiply( hfVecX &dst, const hfVecX &vec ) const;				// dst = (*this) * vec
	HF_INLINE	void			MultiplyAdd( hfVecX &dst, const hfVecX &vec ) const;			// dst += (*this) * vec
	HF_INLINE	void			MultiplySub( hfVecX &dst, const hfVecX &vec ) const;			// dst -= (*this) * vec
	HF_INLINE	void			TransposeMultiply( hfVecX &dst, const hfVecX &vec ) const;		// dst = this->Transpose() * vec
	HF_INLINE	void			TransposeMultiplyAdd( hfVecX &dst, const hfVecX &vec ) const;	// dst += this->Transpose() * vec
	HF_INLINE	void			TransposeMultiplySub( hfVecX &dst, const hfVecX &vec ) const;	// dst -= this->Transpose() * vec

	HF_INLINE	void			Multiply( hfMatX &dst, const hfMatX &a ) const;					// dst = (*this) * a
	HF_INLINE	void			TransposeMultiply( hfMatX &dst, const hfMatX &a ) const;		// dst = this->Transpose() * a

	HF_INLINE	int				GetDimension() const;											// returns total number of values in matrix

	HF_INLINE	const hfVec6 &	SubVec6( int row ) const;										// interpret beginning of row as a const hfVec6
	HF_INLINE	hfVec6 &		SubVec6( int row );												// interpret beginning of row as an hfVec6
	HF_INLINE	const hfVecX	SubVecX( int row ) const;										// interpret complete row as a const hfVecX
	HF_INLINE	hfVecX			SubVecX( int row );												// interpret complete row as an hfVecX
	HF_INLINE	const float *	ToFloatPtr() const;												// pointer to const matrix float array
	HF_INLINE	float *			ToFloatPtr();													// pointer to matrix float array
	const char *	ToString( int precision = 2 ) const;

	void			Update_RankOne( const hfVecX &v, const hfVecX &w, float alpha );
	void			Update_RankOneSymmetric( const hfVecX &v, float alpha );
	void			Update_RowColumn( const hfVecX &v, const hfVecX &w, int r );
	void			Update_RowColumnSymmetric( const hfVecX &v, int r );
	void			Update_Increment( const hfVecX &v, const hfVecX &w );
	void			Update_IncrementSymmetric( const hfVecX &v );
	void			Update_Decrement( int r );

	bool			Inverse_GaussJordan();					// invert in-place with Gauss-Jordan elimination
	bool			Inverse_UpdateRankOne( const hfVecX &v, const hfVecX &w, float alpha );
	bool			Inverse_UpdateRowColumn( const hfVecX &v, const hfVecX &w, int r );
	bool			Inverse_UpdateIncrement( const hfVecX &v, const hfVecX &w );
	bool			Inverse_UpdateDecrement( const hfVecX &v, const hfVecX &w, int r );
	void			Inverse_Solve( hfVecX &x, const hfVecX &b ) const;

	bool			LU_Factor( int *index, float *det = NULL );		// factor in-place: L * U
	bool			LU_UpdateRankOne( const hfVecX &v, const hfVecX &w, float alpha, int *index );
	bool			LU_UpdateRowColumn( const hfVecX &v, const hfVecX &w, int r, int *index );
	bool			LU_UpdateIncrement( const hfVecX &v, const hfVecX &w, int *index );
	bool			LU_UpdateDecrement( const hfVecX &v, const hfVecX &w, const hfVecX &u, int r, int *index );
	void			LU_Solve( hfVecX &x, const hfVecX &b, const int *index ) const;
	void			LU_Inverse( hfMatX &inv, const int *index ) const;
	void			LU_UnpackFactors( hfMatX &L, hfMatX &U ) const;
	void			LU_MultiplyFactors( hfMatX &m, const int *index ) const;

	bool			QR_Factor( hfVecX &c, hfVecX &d );				// factor in-place: Q * R
	bool			QR_UpdateRankOne( hfMatX &R, const hfVecX &v, const hfVecX &w, float alpha );
	bool			QR_UpdateRowColumn( hfMatX &R, const hfVecX &v, const hfVecX &w, int r );
	bool			QR_UpdateIncrement( hfMatX &R, const hfVecX &v, const hfVecX &w );
	bool			QR_UpdateDecrement( hfMatX &R, const hfVecX &v, const hfVecX &w, int r );
	void			QR_Solve( hfVecX &x, const hfVecX &b, const hfVecX &c, const hfVecX &d ) const;
	void			QR_Solve( hfVecX &x, const hfVecX &b, const hfMatX &R ) const;
	void			QR_Inverse( hfMatX &inv, const hfVecX &c, const hfVecX &d ) const;
	void			QR_UnpackFactors( hfMatX &Q, hfMatX &R, const hfVecX &c, const hfVecX &d ) const;
	void			QR_MultiplyFactors( hfMatX &m, const hfVecX &c, const hfVecX &d ) const;

	bool			SVD_Factor( hfVecX &w, hfMatX &V );				// factor in-place: U * Diag(w) * V.Transpose()
	void			SVD_Solve( hfVecX &x, const hfVecX &b, const hfVecX &w, const hfMatX &V ) const;
	void			SVD_Inverse( hfMatX &inv, const hfVecX &w, const hfMatX &V ) const;
	void			SVD_MultiplyFactors( hfMatX &m, const hfVecX &w, const hfMatX &V ) const;

	bool			Cholesky_Factor();						// factor in-place: L * L.Transpose()
	bool			Cholesky_UpdateRankOne( const hfVecX &v, float alpha, int offset = 0 );
	bool			Cholesky_UpdateRowColumn( const hfVecX &v, int r );
	bool			Cholesky_UpdateIncrement( const hfVecX &v );
	bool			Cholesky_UpdateDecrement( const hfVecX &v, int r );
	void			Cholesky_Solve( hfVecX &x, const hfVecX &b ) const;
	void			Cholesky_Inverse( hfMatX &inv ) const;
	void			Cholesky_MultiplyFactors( hfMatX &m ) const;

	bool			LDLT_Factor();							// factor in-place: L * D * L.Transpose()
	bool			LDLT_UpdateRankOne( const hfVecX &v, float alpha, int offset = 0 );
	bool			LDLT_UpdateRowColumn( const hfVecX &v, int r );
	bool			LDLT_UpdateIncrement( const hfVecX &v );
	bool			LDLT_UpdateDecrement( const hfVecX &v, int r );
	void			LDLT_Solve( hfVecX &x, const hfVecX &b ) const;
	void			LDLT_Inverse( hfMatX &inv ) const;
	void			LDLT_UnpackFactors( hfMatX &L, hfMatX &D ) const;
	void			LDLT_MultiplyFactors( hfMatX &m ) const;

	void			TriDiagonal_ClearTriangles();
	bool			TriDiagonal_Solve( hfVecX &x, const hfVecX &b ) const;
	void			TriDiagonal_Inverse( hfMatX &inv ) const;

	bool			Eigen_SolveSymmetricTriDiagonal( hfVecX &eigenValues );
	bool			Eigen_SolveSymmetric( hfVecX &eigenValues );
	bool			Eigen_Solve( hfVecX &realEigenValues, hfVecX &imaginaryEigenValues );
	void			Eigen_SortIncreasing( hfVecX &eigenValues );
	void			Eigen_SortDecreasing( hfVecX &eigenValues );

	static void		Test();

private:
	int				numRows;				// number of rows
	int				numColumns;				// number of columns
	int				alloced;				// floats allocated, if -1 then mat points to data set with SetData
	float *			mat;					// memory the matrix is stored

	static float	temp[MATX_MAX_TEMP+4];	// used to store intermediate results
	static float *	tempPtr;				// pointer to 16 byte aligned temporary memory
	static int		tempIndex;				// index into memory pool, wraps around

private:
	void			SetTempSize( int rows, int columns );
	float			DeterminantGeneric() const;
	bool			InverseSelfGeneric();
	void			QR_Rotate( hfMatX &R, int i, float a, float b );
	float			Pythag( float a, float b ) const;
	void			SVD_BiDiag( hfVecX &w, hfVecX &rv1, float &anorm );
	void			SVD_InitialWV( hfVecX &w, hfMatX &V, hfVecX &rv1 );
	void			HouseholderReduction( hfVecX &diag, hfVecX &subd );
	bool			QL( hfVecX &diag, hfVecX &subd );
	void			HessenbergReduction( hfMatX &H );
	void			ComplexDivision( float xr, float xi, float yr, float yi, float &cdivr, float &cdivi );
	bool			HessenbergToRealSchur( hfMatX &H, hfVecX &realEigenValues, hfVecX &imaginaryEigenValues );
};

/*
========================
hfMatX::hfMatX
========================
*/
HF_INLINE hfMatX::hfMatX() {
	numRows = numColumns = alloced = 0;
	mat = NULL;
}

/*
========================
hfMatX::~hfMatX
========================
*/
HF_INLINE hfMatX::~hfMatX() {
	// if not temp memory
	if ( mat != NULL && ( mat < hfMatX::tempPtr || mat > hfMatX::tempPtr + MATX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( mat );
	}
}

/*
========================
hfMatX::hfMatX
========================
*/
HF_INLINE hfMatX::hfMatX( int rows, int columns ) {
	numRows = numColumns = alloced = 0;
	mat = NULL;
	SetSize( rows, columns );
}

/*
========================
hfMatX::hfMatX
========================
*/
HF_INLINE hfMatX::hfMatX( const hfMatX & other ) {
	numRows = numColumns = alloced = 0;
	mat = NULL;
	Set( other.GetNumRows(), other.GetNumColumns(), other.ToFloatPtr() );
}

/*
========================
hfMatX::hfMatX
========================
*/
HF_INLINE hfMatX::hfMatX( int rows, int columns, float *src ) {
	numRows = numColumns = alloced = 0;
	mat = NULL;
	SetData( rows, columns, src );
}

/*
========================
hfMatX::Set
========================
*/
HF_INLINE void hfMatX::Set( int rows, int columns, const float *src ) {
	SetSize( rows, columns );
	memcpy( this->mat, src, rows * columns * sizeof( float ) );
}

/*
========================
hfMatX::Set
========================
*/
HF_INLINE void hfMatX::Set( const hfMat3 &m1, const hfMat3 &m2 ) {
	SetSize( 3, 6 );
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			mat[(i+0) * numColumns + (j+0)] = m1[i][j];
			mat[(i+0) * numColumns + (j+3)] = m2[i][j];
		}
	}
}

/*
========================
hfMatX::Set
========================
*/
HF_INLINE void hfMatX::Set( const hfMat3 &m1, const hfMat3 &m2, const hfMat3 &m3, const hfMat3 &m4 ) {
	SetSize( 6, 6 );
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			mat[(i+0) * numColumns + (j+0)] = m1[i][j];
			mat[(i+0) * numColumns + (j+3)] = m2[i][j];
			mat[(i+3) * numColumns + (j+0)] = m3[i][j];
			mat[(i+3) * numColumns + (j+3)] = m4[i][j];
		}
	}
}

/*
========================
hfMatX::operator[]
========================
*/
HF_INLINE const float *hfMatX::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < numRows ) );
	return mat + index * numColumns;
}

/*
========================
hfMatX::operator[]
========================
*/
HF_INLINE float *hfMatX::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < numRows ) );
	return mat + index * numColumns;
}

/*
========================
hfMatX::operator=
========================
*/
HF_INLINE hfMatX &hfMatX::operator=( const hfMatX &a ) {
	SetSize( a.numRows, a.numColumns );
	int s = a.numRows * a.numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( mat + i, _mm_load_ps( a.mat + i ) );
	}
#else
	memcpy( mat, a.mat, s * sizeof( float ) );
#endif
	hfMatX::tempIndex = 0;
	return *this;
}

/*
========================
hfMatX::operator*
========================
*/
HF_INLINE hfMatX hfMatX::operator*( const float a ) const {
	hfMatX m;

	m.SetTempSize( numRows, numColumns );
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	__m128 va = _mm_load1_ps( & a );
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( m.mat + i, _mm_mul_ps( _mm_load_ps( mat + i ), va ) );
	}
#else
	for ( int i = 0; i < s; i++ ) {
		m.mat[i] = mat[i] * a;
	}
#endif
	return m;
}

/*
========================
hfMatX::operator*
========================
*/
HF_INLINE hfVecX hfMatX::operator*( const hfVecX &vec ) const {
	assert( numColumns == vec.GetSize() );

	hfVecX dst;
	dst.SetTempSize( numRows );
	Multiply( dst, vec );
	return dst;
}

/*
========================
hfMatX::operator*
========================
*/
HF_INLINE hfMatX hfMatX::operator*( const hfMatX &a ) const {
	assert( numColumns == a.numRows );

	hfMatX dst;
	dst.SetTempSize( numRows, a.numColumns );
	Multiply( dst, a );
	return dst;
}

/*
========================
hfMatX::operator+
========================
*/
HF_INLINE hfMatX hfMatX::operator+( const hfMatX &a ) const {
	hfMatX m;

	assert( numRows == a.numRows && numColumns == a.numColumns );
	m.SetTempSize( numRows, numColumns );
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( m.mat + i, _mm_add_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for ( int i = 0; i < s; i++ ) {
		m.mat[i] = mat[i] + a.mat[i];
	}
#endif
	return m;
}

/*
========================
hfMatX::operator-
========================
*/
HF_INLINE hfMatX hfMatX::operator-( const hfMatX &a ) const {
	hfMatX m;

	assert( numRows == a.numRows && numColumns == a.numColumns );
	m.SetTempSize( numRows, numColumns );
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( m.mat + i, _mm_sub_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for ( int i = 0; i < s; i++ ) {
		m.mat[i] = mat[i] - a.mat[i];
	}
#endif
	return m;
}

/*
========================
hfMatX::operator*=
========================
*/
HF_INLINE hfMatX &hfMatX::operator*=( const float a ) {
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	__m128 va = _mm_load1_ps( & a );
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( mat + i, _mm_mul_ps( _mm_load_ps( mat + i ), va ) );
	}
#else
	for ( int i = 0; i < s; i++ ) {
		mat[i] *= a;
	}
#endif
	hfMatX::tempIndex = 0;
	return *this;
}

/*
========================
hfMatX::operator*=
========================
*/
HF_INLINE hfMatX &hfMatX::operator*=( const hfMatX &a ) {
	*this = *this * a;
	hfMatX::tempIndex = 0;
	return *this;
}

/*
========================
hfMatX::operator+=
========================
*/
HF_INLINE hfMatX &hfMatX::operator+=( const hfMatX &a ) {
	assert( numRows == a.numRows && numColumns == a.numColumns );
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( mat + i, _mm_add_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for ( int i = 0; i < s; i++ ) {
		mat[i] += a.mat[i];
	}
#endif
	hfMatX::tempIndex = 0;
	return *this;
}

/*
========================
hfMatX::operator-=
========================
*/
HF_INLINE hfMatX &hfMatX::operator-=( const hfMatX &a ) {
	assert( numRows == a.numRows && numColumns == a.numColumns );
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( mat + i, _mm_sub_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for ( int i = 0; i < s; i++ ) {
		mat[i] -= a.mat[i];
	}
#endif
	hfMatX::tempIndex = 0;
	return *this;
}

/*
========================
operator*
========================
*/
HF_INLINE hfMatX operator*( const float a, hfMatX const &m ) {
	return m * a;
}

/*
========================
operator* 
========================
*/
HF_INLINE hfVecX operator*( const hfVecX &vec, const hfMatX &m ) {
	return m * vec;
}

/*
========================
operator*=
========================
*/
HF_INLINE hfVecX &operator*=( hfVecX &vec, const hfMatX &m ) {
	vec = m * vec;
	return vec;
}

/*
========================
hfMatX::Compare
========================
*/
HF_INLINE bool hfMatX::Compare( const hfMatX &a ) const {
	assert( numRows == a.numRows && numColumns == a.numColumns );

	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		if ( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfMatX::Compare
========================
*/
HF_INLINE bool hfMatX::Compare( const hfMatX &a, const float epsilon ) const {
	assert( numRows == a.numRows && numColumns == a.numColumns );

	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		if ( hfMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfMatX::operator==
========================
*/
HF_INLINE bool hfMatX::operator==( const hfMatX &a ) const {
	return Compare( a );
}

/*
========================
hfMatX::operator!=
========================
*/
HF_INLINE bool hfMatX::operator!=( const hfMatX &a ) const {
	return !Compare( a );
}

/*
========================
hfMatX::SetSize
========================
*/
HF_INLINE void hfMatX::SetSize( int rows, int columns ) {
	if ( rows != numRows || columns != numColumns || mat == NULL ) {
		assert( mat < hfMatX::tempPtr || mat > hfMatX::tempPtr + MATX_MAX_TEMP );
		int alloc = ( rows * columns + 3 ) & ~3;
		if ( alloc > alloced && alloced != -1 ) {
			if ( mat != NULL ) {
				Mem_Free16( mat );
			}
			mat = (float *) Mem_Alloc16( alloc * sizeof( float ), TAG_MATH );
			alloced = alloc;
		}
		numRows = rows;
		numColumns = columns;
		MATX_CLEAREND();
	}
}

/*
========================
hfMatX::SetTempSize
========================
*/
HF_INLINE void hfMatX::SetTempSize( int rows, int columns ) {
	int newSize;

	newSize = ( rows * columns + 3 ) & ~3;
	assert( newSize < MATX_MAX_TEMP );
	if ( hfMatX::tempIndex + newSize > MATX_MAX_TEMP ) {
		hfMatX::tempIndex = 0;
	}
	mat = hfMatX::tempPtr + hfMatX::tempIndex;
	hfMatX::tempIndex += newSize;
	alloced = newSize;
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

/*
========================
hfMatX::SetData
========================
*/
HF_INLINE void hfMatX::SetData( int rows, int columns, float *data ) {
	assert( mat < hfMatX::tempPtr || mat > hfMatX::tempPtr + MATX_MAX_TEMP );
	if ( mat != NULL && alloced != -1 ) {
		Mem_Free16( mat );
	}
	assert( ( ( (UINT_PTR) data ) & 15 ) == 0 ); // data must be 16 byte aligned
	mat = data;
	alloced = -1;
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

/*
========================
hfMatX::SetDataCacheLines
========================
*/
HF_INLINE void hfMatX::SetDataCacheLines( int rows, int columns, float *data, bool clear ) {
	if ( mat != NULL && alloced != -1 ) {
		Mem_Free( mat );
	}
	assert( ( ( (UINT_PTR) data ) & 127 ) == 0 ); // data must be 128 byte aligned
	mat = data;
	alloced = -1;
	numRows = rows;
	numColumns = columns;

	if ( clear ) {
		int size = numRows * numColumns * sizeof( float );
		for ( int i = 0; i < size; i += CACHE_LINE_SIZE ) {
			ZeroCacheLine( mat, i );
		}
	} else {
		MATX_CLEAREND();
	}
}

/*
========================
hfMatX::Zero
========================
*/
HF_INLINE void hfMatX::Zero() {
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( mat + i, _mm_setzero_ps() );
	}
#else
	s;
	memset( mat, 0, numRows * numColumns * sizeof( float ) );
#endif
}

/*
========================
hfMatX::Zero
========================
*/
HF_INLINE void hfMatX::Zero( int rows, int columns ) {
	SetSize( rows, columns );
	Zero();
}

/*
========================
hfMatX::Identity
========================
*/
HF_INLINE void hfMatX::Identity() {
	assert( numRows == numColumns );
	Zero();
	for ( int i = 0; i < numRows; i++ ) {
		mat[i * numColumns + i] = 1.0f;
	}
}

/*
========================
hfMatX::Identity
========================
*/
HF_INLINE void hfMatX::Identity( int rows, int columns ) {
	assert( rows == columns );
	SetSize( rows, columns );
	hfMatX::Identity();
}

/*
========================
hfMatX::Diag
========================
*/
HF_INLINE void hfMatX::Diag( const hfVecX &v ) {
	Zero( v.GetSize(), v.GetSize() );
	for ( int i = 0; i < v.GetSize(); i++ ) {
		mat[i * numColumns + i] = v[i];
	}
}

/*
========================
hfMatX::Random
========================
*/
HF_INLINE void hfMatX::Random( int seed, float l, float u ) {
	hfRandom rnd(seed);

	float c = u - l;
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
hfMatX::Random
========================
*/
HF_INLINE void hfMatX::Random( int rows, int columns, int seed, float l, float u ) {
	hfRandom rnd(seed);

	SetSize( rows, columns );
	float c = u - l;
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
hfMatX::Negate
========================
*/
HF_INLINE void hfMatX::Negate() {
	int s = numRows * numColumns;
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(MATX_SIMD)
	ALIGN16( const unsigned int signBit[4] ) = { IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK };
	for ( int i = 0; i < s; i += 4 ) {
		_mm_store_ps( mat + i, _mm_xor_ps( _mm_load_ps( mat + i ), (__m128 &) signBit[0] ) );
	}
#else
	for ( int i = 0; i < s; i++ ) {
		mat[i] = -mat[i];
	}
#endif
}

/*
========================
hfMatX::Clamp
========================
*/
HF_INLINE void hfMatX::Clamp( float min, float max ) {
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		if ( mat[i] < min ) {
			mat[i] = min;
		} else if ( mat[i] > max ) {
			mat[i] = max;
		}
	}
}

/*
========================
hfMatX::SwapRows
========================
*/
HF_INLINE hfMatX &hfMatX::SwapRows( int r1, int r2 ) {
	float * ptr1 = mat + r1 * numColumns;
	float * ptr2 = mat + r2 * numColumns;
	for ( int i = 0; i < numColumns; i++ ) {
		SwapValues( ptr1[i], ptr2[i] );
	}
	return *this;
}

/*
========================
hfMatX::SwapColumns
========================
*/
HF_INLINE hfMatX &hfMatX::SwapColumns( int r1, int r2 ) {
	float * ptr = mat;
	for ( int i = 0; i < numRows; i++, ptr += numColumns ) {
		SwapValues( ptr[r1], ptr[r2] );
	}
	return *this;
}

/*
========================
hfMatX::SwapRowsColumns
========================
*/
HF_INLINE hfMatX &hfMatX::SwapRowsColumns( int r1, int r2 ) {
	SwapRows( r1, r2 );
	SwapColumns( r1, r2 );
	return *this;
}

/*
========================
hfMatX::ClearUpperTriangle
========================
*/
HF_INLINE void hfMatX::ClearUpperTriangle() {
	assert( numRows == numColumns );
	for ( int i = numRows-2; i >= 0; i-- ) {
		memset( mat + i * numColumns + i + 1, 0, (numColumns - 1 - i) * sizeof(float) );
	}
}

/*
========================
hfMatX::ClearLowerTriangle
========================
*/
HF_INLINE void hfMatX::ClearLowerTriangle() {
	assert( numRows == numColumns );
	for ( int i = 1; i < numRows; i++ ) {
		memset( mat + i * numColumns, 0, i * sizeof(float) );
	}
}

/*
========================
hfMatX::SquareSubMatrix
========================
*/
HF_INLINE void hfMatX::SquareSubMatrix( const hfMatX &m, int size ) {
	assert( size <= m.numRows && size <= m.numColumns );
	SetSize( size, size );
	for ( int i = 0; i < size; i++ ) {
		memcpy( mat + i * numColumns, m.mat + i * m.numColumns, size * sizeof( float ) );
	}
}

/*
========================
hfMatX::MaxDifference
========================
*/
HF_INLINE float hfMatX::MaxDifference( const hfMatX &m ) const {
	assert( numRows == m.numRows && numColumns == m.numColumns );

	float maxDiff = -1.0f;
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			float diff = hfMath::Fabs( mat[ i * numColumns + j ] - m[i][j] );
			if ( maxDiff < 0.0f || diff > maxDiff ) {
				maxDiff = diff;
			}
		}
	}
	return maxDiff;
}

/*
========================
hfMatX::IsZero
========================
*/
HF_INLINE bool hfMatX::IsZero( const float epsilon ) const {
	// returns true if (*this) == Zero
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( hfMath::Fabs( mat[i * numColumns + j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

/*
========================
hfMatX::IsIdentity
========================
*/
HF_INLINE bool hfMatX::IsIdentity( const float epsilon ) const {
	// returns true if (*this) == Identity
	assert( numRows == numColumns );
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( hfMath::Fabs( mat[i * numColumns + j] - (float)( i == j ) ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

/*
========================
hfMatX::IsDiagonal
========================
*/
HF_INLINE bool hfMatX::IsDiagonal( const float epsilon ) const {
	// returns true if all elements are zero except for the elements on the diagonal
	assert( numRows == numColumns );
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( i != j && hfMath::Fabs( mat[i * numColumns + j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

/*
========================
hfMatX::IsTriDiagonal
========================
*/
HF_INLINE bool hfMatX::IsTriDiagonal( const float epsilon ) const {
	// returns true if all elements are zero except for the elements on the diagonal plus or minus one column

	if ( numRows != numColumns ) {
		return false;
	}
	for ( int i = 0; i < numRows-2; i++ ) {
		for ( int j = i+2; j < numColumns; j++ ) {
			if ( hfMath::Fabs( (*this)[i][j] ) > epsilon ) {
				return false;
			}
			if ( hfMath::Fabs( (*this)[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

/*
========================
hfMatX::IsSymmetric
========================
*/
HF_INLINE bool hfMatX::IsSymmetric( const float epsilon ) const {
	// (*this)[i][j] == (*this)[j][i]
	if ( numRows != numColumns ) {
		return false;
	}
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( hfMath::Fabs( mat[ i * numColumns + j ] - mat[ j * numColumns + i ] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

/*
========================
hfMatX::Trace
========================
*/
HF_INLINE float hfMatX::Trace() const {
	float trace = 0.0f;

	assert( numRows == numColumns );

	// sum of elements on the diagonal
	for ( int i = 0; i < numRows; i++ ) {
		trace += mat[i * numRows + i];
	}
	return trace;
}

/*
========================
hfMatX::Determinant
========================
*/
HF_INLINE float hfMatX::Determinant() const {

	assert( numRows == numColumns );

	switch( numRows ) {
		case 1: return mat[0];
		case 2: return reinterpret_cast<const hfMat2 *>( mat )->Determinant();
		case 3: return reinterpret_cast<const hfMat3 *>( mat )->Determinant();
		case 4: return reinterpret_cast<const hfMat4 *>( mat )->Determinant();
		case 5: return reinterpret_cast<const hfMat5 *>( mat )->Determinant();
		case 6: return reinterpret_cast<const hfMat6 *>( mat )->Determinant();
		default: return DeterminantGeneric();
	}
}

/*
========================
hfMatX::Transpose
========================
*/
HF_INLINE hfMatX hfMatX::Transpose() const {
	hfMatX transpose;

	transpose.SetTempSize( numColumns, numRows );

	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			transpose.mat[j * transpose.numColumns + i] = mat[i * numColumns + j];
		}
	}

	return transpose;
}

/*
========================
hfMatX::TransposeSelf
========================
*/
HF_INLINE hfMatX &hfMatX::TransposeSelf() {
	*this = Transpose();
	return *this;
}

/*
========================
hfMatX::Transpose
========================
*/
HF_INLINE void hfMatX::Transpose( hfMatX & dst ) const {
	dst = Transpose();
}

/*
========================
hfMatX::Inverse
========================
*/
HF_INLINE hfMatX hfMatX::Inverse() const {
	hfMatX invMat;

	invMat.SetTempSize( numRows, numColumns );
	memcpy( invMat.mat, mat, numRows * numColumns * sizeof( float ) );
	verify( invMat.InverseSelf() );
	return invMat;
}

/*
========================
hfMatX::InverseSelf
========================
*/
HF_INLINE bool hfMatX::InverseSelf() {

	assert( numRows == numColumns );

	switch( numRows ) {
		case 1:
			if ( hfMath::Fabs( mat[0] ) < MATRIX_INVERSE_EPSILON ) {
				return false;
			}
			mat[0] = 1.0f / mat[0];
			return true;
		case 2:
			return reinterpret_cast<hfMat2 *>(mat)->InverseSelf();
		case 3:
			return reinterpret_cast<hfMat3 *>(mat)->InverseSelf();
		case 4:
			return reinterpret_cast<hfMat4 *>(mat)->InverseSelf();
		case 5:
			return reinterpret_cast<hfMat5 *>(mat)->InverseSelf();
		case 6:
			return reinterpret_cast<hfMat6 *>(mat)->InverseSelf();
		default:
			return InverseSelfGeneric();
	}
}

/*
========================
hfMatX::InverseFast
========================
*/
HF_INLINE hfMatX hfMatX::InverseFast() const {
	hfMatX invMat;

	invMat.SetTempSize( numRows, numColumns );
	memcpy( invMat.mat, mat, numRows * numColumns * sizeof( float ) );
	verify( invMat.InverseFastSelf() );
	return invMat;
}

/*
========================
hfMatX::InverseFastSelf
========================
*/
HF_INLINE bool hfMatX::InverseFastSelf() {

	assert( numRows == numColumns );

	switch( numRows ) {
		case 1:
			if ( hfMath::Fabs( mat[0] ) < MATRIX_INVERSE_EPSILON ) {
				return false;
			}
			mat[0] = 1.0f / mat[0];
			return true;
		case 2:
			return reinterpret_cast<hfMat2 *>(mat)->InverseFastSelf();
		case 3:
			return reinterpret_cast<hfMat3 *>(mat)->InverseFastSelf();
		case 4:
			return reinterpret_cast<hfMat4 *>(mat)->InverseFastSelf();
		case 5:
			return reinterpret_cast<hfMat5 *>(mat)->InverseFastSelf();
		case 6:
			return reinterpret_cast<hfMat6 *>(mat)->InverseFastSelf();
		default:
			return InverseSelfGeneric();
	}
}

/*
========================
hfMatX::Inverse
========================
*/
HF_INLINE void hfMatX::Inverse( hfMatX & dst ) const {
	dst = InverseFast();
}

/*
========================
hfMatX::Subtract
========================
*/
HF_INLINE void hfMatX::Subtract( const hfMatX & a ) {
	(*this) -= a;
}

/*
========================
hfMatX::Multiply
========================
*/
HF_INLINE hfVecX hfMatX::Multiply( const hfVecX &vec ) const {
	assert( numColumns == vec.GetSize() );

	hfVecX dst;
	dst.SetTempSize( numRows );
	Multiply( dst, vec );
	return dst;
}

/*
========================
hfMatX::Multiply
========================
*/
HF_INLINE hfMatX hfMatX::Multiply( const hfMatX &a ) const {
	assert( numColumns == a.numRows );

	hfMatX dst;
	dst.SetTempSize( numRows, a.numColumns );
	Multiply( dst, a );
	return dst;
}

/*
========================
hfMatX::TransposeMultiply
========================
*/
HF_INLINE hfVecX hfMatX::TransposeMultiply( const hfVecX &vec ) const {
	assert( numRows == vec.GetSize() );

	hfVecX dst;
	dst.SetTempSize( numColumns );
	TransposeMultiply( dst, vec );
	return dst;
}

/*
========================
hfMatX::TransposeMultiply
========================
*/
HF_INLINE hfMatX hfMatX::TransposeMultiply( const hfMatX &a ) const {
	assert( numRows == a.numRows );

	hfMatX dst;
	dst.SetTempSize( numColumns, a.numColumns );
	TransposeMultiply( dst, a );
	return dst;
}

/*
========================
hfMatX::Multiply
========================
*/
HF_INLINE void hfMatX::Multiply( hfVecX &dst, const hfVecX &vec ) const {
	dst.SetSize( numRows );
	const float * mPtr = mat;
	const float * vPtr = vec.ToFloatPtr();
	float * dstPtr = dst.ToFloatPtr();
	float * temp = (float *)_alloca16( numRows * sizeof( float ) );
	for ( int i = 0; i < numRows; i++ ) {
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numColumns; j++ ) {
			sum += mPtr[j] * vPtr[j];
		}
		temp[i] = sum;
		mPtr += numColumns;
	}
	for ( int i = 0; i < numRows; i++ ) {
		dstPtr[i] = temp[i];
	}
}

/*
========================
hfMatX::MultiplyAdd
========================
*/
HF_INLINE void hfMatX::MultiplyAdd( hfVecX &dst, const hfVecX &vec ) const {
	assert( dst.GetSize() == numRows );
	const float * mPtr = mat;
	const float * vPtr = vec.ToFloatPtr();
	float * dstPtr = dst.ToFloatPtr();
	float * temp = (float *)_alloca16( numRows * sizeof( float ) );
	for ( int i = 0; i < numRows; i++ ) {
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numColumns; j++ ) {
			sum += mPtr[j] * vPtr[j];
		}
		temp[i] = dstPtr[i] + sum;
		mPtr += numColumns;
	}
	for ( int i = 0; i < numRows; i++ ) {
		dstPtr[i] = temp[i];
	}
}

/*
========================
hfMatX::MultiplySub
========================
*/
HF_INLINE void hfMatX::MultiplySub( hfVecX &dst, const hfVecX &vec ) const {
	assert( dst.GetSize() == numRows );
	const float * mPtr = mat;
	const float * vPtr = vec.ToFloatPtr();
	float * dstPtr = dst.ToFloatPtr();
	float * temp = (float *)_alloca16( numRows * sizeof( float ) );
	for ( int i = 0; i < numRows; i++ ) {
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numColumns; j++ ) {
			sum += mPtr[j] * vPtr[j];
		}
		temp[i] = dstPtr[i] - sum;
		mPtr += numColumns;
	}
	for ( int i = 0; i < numRows; i++ ) {
		dstPtr[i] = temp[i];
	}
}

/*
========================
hfMatX::TransposeMultiply
========================
*/
HF_INLINE void hfMatX::TransposeMultiply( hfVecX &dst, const hfVecX &vec ) const {
	dst.SetSize( numColumns );
	const float * vPtr = vec.ToFloatPtr();
	float * dstPtr = dst.ToFloatPtr();
	float * temp = (float *)_alloca16( numColumns * sizeof( float ) );
	for ( int i = 0; i < numColumns; i++ ) {
		const float * mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numRows; j++ ) {
			mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		temp[i] = sum;
	}
	for ( int i = 0; i < numColumns; i++ ) {
		dstPtr[i] = temp[i];
	}
}

/*
========================
hfMatX::TransposeMultiplyAdd
========================
*/
HF_INLINE void hfMatX::TransposeMultiplyAdd( hfVecX &dst, const hfVecX &vec ) const {
	assert( dst.GetSize() == numColumns );
	const float * vPtr = vec.ToFloatPtr();
	float * dstPtr = dst.ToFloatPtr();
	float * temp = (float *)_alloca16( numColumns * sizeof( float ) );
	for ( int i = 0; i < numColumns; i++ ) {
		const float * mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numRows; j++ ) {
			mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		temp[i] = dstPtr[i] + sum;
	}
	for ( int i = 0; i < numColumns; i++ ) {
		dstPtr[i] = temp[i];
	}
}

/*
========================
hfMatX::TransposeMultiplySub
========================
*/
HF_INLINE void hfMatX::TransposeMultiplySub( hfVecX &dst, const hfVecX &vec ) const {
	assert( dst.GetSize() == numColumns );
	const float * vPtr = vec.ToFloatPtr();
	float * dstPtr = dst.ToFloatPtr();
	float * temp = (float *)_alloca16( numColumns * sizeof( float ) );
	for ( int i = 0; i < numColumns; i++ ) {
		const float * mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numRows; j++ ) {
			mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		temp[i] = dstPtr[i] - sum;
	}
	for ( int i = 0; i < numColumns; i++ ) {
		dstPtr[i] = temp[i];
	}
}

/*
========================
hfMatX::Multiply
========================
*/
HF_INLINE void hfMatX::Multiply( hfMatX &dst, const hfMatX &a ) const {
	assert( numColumns == a.numRows );
	assert( &dst != &a && &dst != this );

	dst.SetSize( numRows, a.numColumns );
	float * dstPtr = dst.ToFloatPtr();
	const float * m1Ptr = ToFloatPtr();
	int k = numRows;
	int l = a.GetNumColumns();
	for ( int i = 0; i < k; i++ ) {
		for ( int j = 0; j < l; j++ ) {
			const float * m2Ptr = a.ToFloatPtr() + j;
			float sum = m1Ptr[0] * m2Ptr[0];
			for ( int n = 1; n < numColumns; n++ ) {
				m2Ptr += l;
				sum += m1Ptr[n] * m2Ptr[0];
			}
			*dstPtr++ = sum;
		}
		m1Ptr += numColumns;
	}
}

/*
========================
hfMatX::TransposeMultiply
========================
*/
HF_INLINE void hfMatX::TransposeMultiply( hfMatX &dst, const hfMatX &a ) const {
	assert( numRows == a.numRows );
	assert( &dst != &a && &dst != this );

	dst.SetSize( numColumns, a.numColumns );
	float * dstPtr = dst.ToFloatPtr();
	int k = numColumns;
	int l = a.numColumns;
	for ( int i = 0; i < k; i++ ) {
		for ( int j = 0; j < l; j++ ) {
			const float * m1Ptr = ToFloatPtr() + i;
			const float * m2Ptr = a.ToFloatPtr() + j;
			float sum = m1Ptr[0] * m2Ptr[0];
			for ( int n = 1; n < numRows; n++ ) {
				m1Ptr += numColumns;
				m2Ptr += a.numColumns;
				sum += m1Ptr[0] * m2Ptr[0];
			}
			*dstPtr++ = sum;
		}
	}
}

/*
========================
hfMatX::GetDimension
========================
*/
HF_INLINE int hfMatX::GetDimension() const {
	return numRows * numColumns;
}

/*
========================
hfMatX::SubVec6
========================
*/
HF_INLINE const hfVec6 &hfMatX::SubVec6( int row ) const {
	assert( numColumns >= 6 && row >= 0 && row < numRows );
	return *reinterpret_cast<const hfVec6 *>(mat + row * numColumns);
}

/*
========================
hfMatX::SubVec6
========================
*/
HF_INLINE hfVec6 &hfMatX::SubVec6( int row ) {
	assert( numColumns >= 6 && row >= 0 && row < numRows );
	return *reinterpret_cast<hfVec6 *>(mat + row * numColumns);
}

/*
========================
hfMatX::SubVecX
========================
*/
HF_INLINE const hfVecX hfMatX::SubVecX( int row ) const {
	hfVecX v;
	assert( row >= 0 && row < numRows );
	v.SetData( numColumns, mat + row * numColumns );
	return v;
}

/*
========================
hfMatX::SubVecX
========================
*/
HF_INLINE hfVecX hfMatX::SubVecX( int row ) {
	hfVecX v;
	assert( row >= 0 && row < numRows );
	v.SetData( numColumns, mat + row * numColumns );
	return v;
}

/*
========================
hfMatX::ToFloatPtr
========================
*/
HF_INLINE const float *hfMatX::ToFloatPtr() const {
	return mat;
}

/*
========================
hfMatX::ToFloatPtr
========================
*/
HF_INLINE float *hfMatX::ToFloatPtr() {
	return mat;
}

#endif // !__MATH_MATRIXX_H__

