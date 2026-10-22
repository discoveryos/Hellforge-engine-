#ifndef __MATH_VECX_H__
#define __MATH_VECX_H__

/*
===============================================================================

hfVecX - arbitrary sized vector

The vector lives on 16 byte aligned and 16 byte padded memory.

NOTE: due to the temporary memory pool hfVecX cannot be used by multiple threads

===============================================================================
*/

#define VECX_MAX_TEMP		1024
#define VECX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define VECX_CLEAREND()		int s = size; while( s < ( ( s + 3) & ~3 ) ) { p[s++] = 0.0f; }
#define VECX_ALLOCA( n )	( (float *) _alloca16( VECX_QUAD( n ) ) )
#define VECX_SIMD

class hfVecX {
	friend class hfMatX;

public:	
	HF_INLINE					hfVecX();
	HF_INLINE					explicit hfVecX( int length );
	HF_INLINE					explicit hfVecX( int length, float *data );
	HF_INLINE					~hfVecX();

	HF_INLINE	float			Get( int index ) const;
	HF_INLINE	float &			Get( int index );

	HF_INLINE	float			operator[]( const int index ) const;
	HF_INLINE	float &			operator[]( const int index );
	HF_INLINE	hfVecX			operator-() const;
	HF_INLINE	hfVecX &		operator=( const hfVecX &a );
	HF_INLINE	hfVecX			operator*( const float a ) const;
	HF_INLINE	hfVecX			operator/( const float a ) const;
	HF_INLINE	float			operator*( const hfVecX &a ) const;
	HF_INLINE	hfVecX			operator-( const hfVecX &a ) const;
	HF_INLINE	hfVecX			operator+( const hfVecX &a ) const;
	HF_INLINE	hfVecX &		operator*=( const float a );
	HF_INLINE	hfVecX &		operator/=( const float a );
	HF_INLINE	hfVecX &		operator+=( const hfVecX &a );
	HF_INLINE	hfVecX &		operator-=( const hfVecX &a );

	friend HF_INLINE	hfVecX	operator*( const float a, const hfVecX &b );

	HF_INLINE	bool			Compare( const hfVecX &a ) const;							// exact compare, no epsilon
	HF_INLINE	bool			Compare( const hfVecX &a, const float epsilon ) const;		// compare with epsilon
	HF_INLINE	bool			operator==(	const hfVecX &a ) const;						// exact compare, no epsilon
	HF_INLINE	bool			operator!=(	const hfVecX &a ) const;						// exact compare, no epsilon

	HF_INLINE	void			SetSize( int size );
	HF_INLINE	void			ChangeSize( int size, bool makeZero = false );
	HF_INLINE	int				GetSize() const { return size; }
	HF_INLINE	void			SetData( int length, float *data );
	HF_INLINE	void			Zero();
	HF_INLINE	void			Zero( int length );
	HF_INLINE	void			Random( int seed, float l = 0.0f, float u = 1.0f );
	HF_INLINE	void			Random( int length, int seed, float l = 0.0f, float u = 1.0f );
	HF_INLINE	void			Negate();
	HF_INLINE	void			Clamp( float min, float max );
	HF_INLINE	hfVecX &		SwapElements( int e1, int e2 );

	HF_INLINE	float			Length() const;
	HF_INLINE	float			LengthSqr() const;
	HF_INLINE	hfVecX			Normalize() const;
	HF_INLINE	float			NormalizeSelf();

	HF_INLINE	int				GetDimension() const;

	HF_INLINE	void			AddScaleAdd( const float scale, const hfVecX & v0, const hfVecX & v1 );

	HF_INLINE	const hfVec3 &	SubVec3( int index ) const;
	HF_INLINE	hfVec3 &		SubVec3( int index );
	HF_INLINE	const hfVec6 &	SubVec6( int index = 0 ) const;
	HF_INLINE	hfVec6 &		SubVec6( int index = 0 );
	HF_INLINE	const float *	ToFloatPtr() const;
	HF_INLINE	float *			ToFloatPtr();
	const char *	ToString( int precision = 2 ) const;

private:
	int				size;					// size of the vector
	int				alloced;				// if -1 p points to data set with SetData
	float *			p;						// memory the vector is stored

	static float	temp[VECX_MAX_TEMP+4];	// used to store intermediate results
	static float *	tempPtr;				// pointer to 16 byte aligned temporary memory
	static int		tempIndex;				// index into memory pool, wraps around

	HF_INLINE void	SetTempSize( int size );
};


/*
========================
hfVecX::hfVecX
========================
*/
HF_INLINE hfVecX::hfVecX() {
	size = alloced = 0;
	p = NULL;
}

/*
========================
hfVecX::hfVecX
========================
*/
HF_INLINE hfVecX::hfVecX( int length ) {
	size = alloced = 0;
	p = NULL;
	SetSize( length );
}

/*
========================
hfVecX::hfVecX
========================
*/
HF_INLINE hfVecX::hfVecX( int length, float *data ) {
	size = alloced = 0;
	p = NULL;
	SetData( length, data );
}

/*
========================
hfVecX::~hfVecX
========================
*/
HF_INLINE hfVecX::~hfVecX() {
	// if not temp memory
	if ( p && ( p < hfVecX::tempPtr || p >= hfVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( p );
	}
}

/*
========================
hfVecX::Get
========================
*/
HF_INLINE float hfVecX::Get( int index ) const {
	assert( index >= 0 && index < size );
	return p[index];
}

/*
========================
hfVecX::Get
========================
*/
HF_INLINE float & hfVecX::Get( int index ) {
	assert( index >= 0 && index < size );
	return p[index];
}

/*
========================
hfVecX::operator[]
========================
*/
HF_INLINE float hfVecX::operator[]( int index ) const {
	return Get( index );
}

/*
========================
hfVecX::operator[]
========================
*/
HF_INLINE float & hfVecX::operator[]( int index ) {
	return Get( index );
}

/*
========================
hfVecX::operator-
========================
*/
HF_INLINE hfVecX hfVecX::operator-() const {
	hfVecX m;

	m.SetTempSize( size );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	ALIGN16( unsigned int signBit[4] ) = { IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK };
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( m.p + i, _mm_xor_ps( _mm_load_ps( p + i ), (__m128 &) signBit[0] ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = -p[i];
	}
#endif
	return m;
}

/*
========================
hfVecX::operator=
========================
*/
HF_INLINE hfVecX &hfVecX::operator=( const hfVecX &a ) { 
	SetSize( a.size );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	for ( int i = 0; i < a.size; i += 4 ) {
		_mm_store_ps( p + i, _mm_load_ps( a.p + i ) );
	}
#else
	memcpy( p, a.p, a.size * sizeof( float ) );
#endif
	hfVecX::tempIndex = 0;
	return *this;
}

/*
========================
hfVecX::operator+
========================
*/
HF_INLINE hfVecX hfVecX::operator+( const hfVecX &a ) const {
	hfVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( m.p + i, _mm_add_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] + a.p[i];
	}
#endif
	return m;
}

/*
========================
hfVecX::operator-
========================
*/
HF_INLINE hfVecX hfVecX::operator-( const hfVecX &a ) const {
	hfVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( m.p + i, _mm_sub_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] - a.p[i];
	}
#endif
	return m;
}

/*
========================
hfVecX::operator+=
========================
*/
HF_INLINE hfVecX &hfVecX::operator+=( const hfVecX &a ) {
	assert( size == a.size );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( p + i, _mm_add_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] += a.p[i];
	}
#endif
	hfVecX::tempIndex = 0;
	return *this;
}

/*
========================
hfVecX::operator-=
========================
*/
HF_INLINE hfVecX &hfVecX::operator-=( const hfVecX &a ) {
	assert( size == a.size );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( p + i, _mm_sub_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] -= a.p[i];
	}
#endif
	hfVecX::tempIndex = 0;
	return *this;
}

/*
========================
hfVecX::operator*
========================
*/
HF_INLINE hfVecX hfVecX::operator*( const float a ) const {
	hfVecX m;

	m.SetTempSize( size );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	__m128 va = _mm_load1_ps( & a );
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( m.p + i, _mm_mul_ps( _mm_load_ps( p + i ), va ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] * a;
	}
#endif
	return m;
}

/*
========================
hfVecX::operator*=
========================
*/
HF_INLINE hfVecX &hfVecX::operator*=( const float a ) {
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	__m128 va = _mm_load1_ps( & a );
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( p + i, _mm_mul_ps( _mm_load_ps( p + i ), va ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] *= a;
	}
#endif
	return *this;
}

/*
========================
hfVecX::operator/
========================
*/
HF_INLINE hfVecX hfVecX::operator/( const float a ) const {
	assert( fabs( a ) > hfMath::FLT_SMALLEST_NON_DENORMAL );
	return (*this) * ( 1.0f / a );
}

/*
========================
hfVecX::operator/=
========================
*/
HF_INLINE hfVecX &hfVecX::operator/=( const float a ) {
	assert( fabs( a ) > hfMath::FLT_SMALLEST_NON_DENORMAL );
	(*this) *= ( 1.0f / a );
	return *this;
}

/*
========================
operator*
========================
*/
HF_INLINE hfVecX operator*( const float a, const hfVecX &b ) {
	return b * a;
}

/*
========================
hfVecX::operator*
========================
*/
HF_INLINE float hfVecX::operator*( const hfVecX &a ) const {
	assert( size == a.size );
	float sum = 0.0f;
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * a.p[i];
	}
	return sum;
}

/*
========================
hfVecX::Compare
========================
*/
HF_INLINE bool hfVecX::Compare( const hfVecX &a ) const {
	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		if ( p[i] != a.p[i] ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfVecX::Compare
========================
*/
HF_INLINE bool hfVecX::Compare( const hfVecX &a, const float epsilon ) const {
	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		if ( hfMath::Fabs( p[i] - a.p[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfVecX::operator==
========================
*/
HF_INLINE bool hfVecX::operator==( const hfVecX &a ) const {
	return Compare( a );
}

/*
========================
hfVecX::operator!=
========================
*/
HF_INLINE bool hfVecX::operator!=( const hfVecX &a ) const {
	return !Compare( a );
}

/*
========================
hfVecX::SetSize
========================
*/
HF_INLINE void hfVecX::SetSize( int newSize ) {
	//assert( p < hfVecX::tempPtr || p > hfVecX::tempPtr + VECX_MAX_TEMP );
	if ( newSize != size || p == NULL ) {
		int alloc = ( newSize + 3 ) & ~3;
		if ( alloc > alloced && alloced != -1 ) {
			if ( p ) {
				Mem_Free16( p );
			}
			p = (float *) Mem_Alloc16( alloc * sizeof( float ), TAG_MATH );
			alloced = alloc;
		}
		size = newSize;
		VECX_CLEAREND();
	}
}

/*
========================
hfVecX::ChangeSize
========================
*/
HF_INLINE void hfVecX::ChangeSize( int newSize, bool makeZero ) {
	if ( newSize != size ) {
		int alloc = ( newSize + 3 ) & ~3;
		if ( alloc > alloced && alloced != -1 ) {
			float *oldVec = p;
			p = (float *) Mem_Alloc16( alloc * sizeof( float ), TAG_MATH );
			alloced = alloc;
			if ( oldVec ) {
				for ( int i = 0; i < size; i++ ) {
					p[i] = oldVec[i];
				}
				Mem_Free16( oldVec );
			}
			if ( makeZero ) {
				// zero any new elements
				for ( int i = size; i < newSize; i++ ) {
					p[i] = 0.0f;
				}
			}
		}
		size = newSize;
		VECX_CLEAREND();
	}
}

/*
========================
hfVecX::SetTempSize
========================
*/
HF_INLINE void hfVecX::SetTempSize( int newSize ) {
	size = newSize;
	alloced = ( newSize + 3 ) & ~3;
	assert( alloced < VECX_MAX_TEMP );
	if ( hfVecX::tempIndex + alloced > VECX_MAX_TEMP ) {
		hfVecX::tempIndex = 0;
	}
	p = hfVecX::tempPtr + hfVecX::tempIndex;
	hfVecX::tempIndex += alloced;
	VECX_CLEAREND();
}

/*
========================
hfVecX::SetData
========================
*/
HF_INLINE void hfVecX::SetData( int length, float *data ) {
	if ( p != NULL && ( p < hfVecX::tempPtr || p >= hfVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( p );
	}
	assert_16_byte_aligned( data ); // data must be 16 byte aligned
	p = data;
	size = length;
	alloced = -1;
	VECX_CLEAREND();
}

/*
========================
hfVecX::Zero
========================
*/
HF_INLINE void hfVecX::Zero() {
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( p + i, _mm_setzero_ps() );
	}
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

/*
========================
hfVecX::Zero
========================
*/
HF_INLINE void hfVecX::Zero( int length ) {
	SetSize( length );
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	for ( int i = 0; i < length; i += 4 ) {
		_mm_store_ps( p + i, _mm_setzero_ps() );
	}
#else
	memset( p, 0, length * sizeof( float ) );
#endif
}

/*
========================
hfVecX::Random
========================
*/
HF_INLINE void hfVecX::Random( int seed, float l, float u ) {
	hfRandom rnd( seed );

	float c = u - l;
	for ( int i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
hfVecX::Random
========================
*/
HF_INLINE void hfVecX::Random( int length, int seed, float l, float u ) {
	hfRandom rnd( seed );

	SetSize( length );
	float c = u - l;
	for ( int i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
hfVecX::Negate
========================
*/
HF_INLINE void hfVecX::Negate() {
#if defined(HF_WIN_X86_SSE_INTRIN) && defined(VECX_SIMD)
	ALIGN16( const unsigned int signBit[4] ) = { IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK };
	for ( int i = 0; i < size; i += 4 ) {
		_mm_store_ps( p + i, _mm_xor_ps( _mm_load_ps( p + i ), (__m128 &) signBit[0] ) );
	}
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] = -p[i];
	}
#endif
}

/*
========================
hfVecX::Clamp
========================
*/
HF_INLINE void hfVecX::Clamp( float min, float max ) {
	for ( int i = 0; i < size; i++ ) {
		if ( p[i] < min ) {
			p[i] = min;
		} else if ( p[i] > max ) {
			p[i] = max;
		}
	}
}

/*
========================
hfVecX::SwapElements
========================
*/
HF_INLINE hfVecX &hfVecX::SwapElements( int e1, int e2 ) {
	float tmp;
	tmp = p[e1];
	p[e1] = p[e2];
	p[e2] = tmp;
	return *this;
}

/*
========================
hfVecX::Length
========================
*/
HF_INLINE float hfVecX::Length() const {
	float sum = 0.0f;
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return hfMath::Sqrt( sum );
}

/*
========================
hfVecX::LengthSqr
========================
*/
HF_INLINE float hfVecX::LengthSqr() const {
	float sum = 0.0f;
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return sum;
}

/*
========================
hfVecX::Normalize
========================
*/
HF_INLINE hfVecX hfVecX::Normalize() const {
	hfVecX m;

	m.SetTempSize( size );
	float sum = 0.0f;
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	float invSqrt = hfMath::InvSqrt( sum );
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] * invSqrt;
	}
	return m;
}

/*
========================
hfVecX::NormalizeSelf
========================
*/
HF_INLINE float hfVecX::NormalizeSelf() {
	float sum = 0.0f;
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	float invSqrt = hfMath::InvSqrt( sum );
	for ( int i = 0; i < size; i++ ) {
		p[i] *= invSqrt;
	}
	return invSqrt * sum;
}

/*
========================
hfVecX::GetDimension
========================
*/
HF_INLINE int hfVecX::GetDimension() const {
	return size;
}

/*
========================
hfVecX::SubVec3
========================
*/
HF_INLINE hfVec3 &hfVecX::SubVec3( int index ) {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<hfVec3 *>(p + index * 3);
}

/*
========================
hfVecX::SubVec3
========================
*/
HF_INLINE const hfVec3 &hfVecX::SubVec3( int index ) const {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<const hfVec3 *>(p + index * 3);
}

/*
========================
hfVecX::SubVec6
========================
*/
HF_INLINE hfVec6 &hfVecX::SubVec6( int index ) {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<hfVec6 *>(p + index * 6);
}

/*
========================
hfVecX::SubVec6
========================
*/
HF_INLINE const hfVec6 &hfVecX::SubVec6( int index ) const {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<const hfVec6 *>(p + index * 6);
}

/*
========================
hfVecX::ToFloatPtr
========================
*/
HF_INLINE const float *hfVecX::ToFloatPtr() const {
	return p;
}

/*
========================
hfVecX::ToFloatPtr
========================
*/
HF_INLINE float *hfVecX::ToFloatPtr() {
	return p;
}

/*
========================
hfVecX::AddScaleAdd
========================
*/
HF_INLINE void hfVecX::AddScaleAdd( const float scale, const hfVecX &v0, const hfVecX &v1 ) {
	assert( GetSize() == v0.GetSize() );
	assert( GetSize() == v1.GetSize() );

	const float * v0Ptr = v0.ToFloatPtr();
	const float * v1Ptr = v1.ToFloatPtr();
	float * dstPtr = ToFloatPtr();

	for ( int i = 0; i < size; i++ ) {
		dstPtr[i] += scale * ( v0Ptr[i] + v1Ptr[i] );
	}
}

#endif // !__MATH_VECTORX_H__

