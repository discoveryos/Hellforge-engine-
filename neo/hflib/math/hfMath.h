#ifndef __MATH_MATH_H__
#define __MATH_MATH_H__

#ifdef MACOS_X
// for square root estimate instruction
#include <ppc_intrinsics.h>
// for FLT_MIN
#include <float.h>
#endif

/*
===============================================================================

  Math

===============================================================================
*/

#ifdef INFINITY
#undef INFINITY
#endif

#ifdef FLT_EPSILON
#undef FLT_EPSILON
#endif

#define DEG2RAD(a)				( (a) * hfMath::M_DEG2RAD )
#define RAD2DEG(a)				( (a) * hfMath::M_RAD2DEG )

#define SEC2MS(t)				( hfMath::Ftoi( (t) * hfMath::M_SEC2MS ) )
#define MS2SEC(t)				( (t) * hfMath::M_MS2SEC )

#define	ANGLE2SHORT(x)			( hfMath::Ftoi( (x) * 65536.0f / 360.0f ) & 65535 )
#define	SHORT2ANGLE(x)			( (x) * ( 360.0f / 65536.0f ) )

#define	ANGLE2BYTE(x)			( hfMath::Ftoi( (x) * 256.0f / 360.0f ) & 255 )
#define	BYTE2ANGLE(x)			( (x) * ( 360.0f / 256.0f ) )

#define C_FLOAT_TO_INT( x )		(int)(x)

/*
================================================================================================

	two-complements integer bit layouts

================================================================================================
*/

#define INT8_SIGN_BIT		7
#define INT16_SIGN_BIT		15
#define INT32_SIGN_BIT		31
#define INT64_SIGN_BIT		63

#define INT8_SIGN_MASK		( 1 << INT8_SIGN_BIT )
#define INT16_SIGN_MASK		( 1 << INT16_SIGN_BIT )
#define INT32_SIGN_MASK		( 1UL << INT32_SIGN_BIT )
#define INT64_SIGN_MASK		( 1ULL << INT64_SIGN_BIT )

/*
================================================================================================

	integer sign bit tests

================================================================================================
*/

// If this was ever compiled on a system that had 64 bit unsigned ints,
// it would fail.
compile_time_assert( sizeof( unsigned int ) == 4 );

#define OLD_INT32_SIGNBITSET(i)		(static_cast<const unsigned int>(i) >> INT32_SIGN_BIT)
#define OLD_INT32_SIGNBITNOTSET(i)	((~static_cast<const unsigned int>(i)) >> INT32_SIGN_BIT)

// Unfortunately, /analyze can't figure out that these always return
// either 0 or 1, so this extra wrapper is needed to avoid the static
// alaysis warning.

HF_INLINE_EXTERN int INT32_SIGNBITSET( int i ) {
	int	r = OLD_INT32_SIGNBITSET( i );
	assert( r == 0 || r == 1 );
	return r;
}

HF_INLINE_EXTERN int INT32_SIGNBITNOTSET( int i ) {
	int	r = OLD_INT32_SIGNBITNOTSET( i );
	assert( r == 0 || r == 1 );
	return r;
}

/*
================================================================================================

	floating point bit layouts according to the IEEE 754-1985 and 754-2008 standard

================================================================================================
*/

#define IEEE_FLT16_MANTISSA_BITS	10
#define IEEE_FLT16_EXPONENT_BITS	5
#define IEEE_FLT16_EXPONENT_BIAS	15
#define IEEE_FLT16_SIGN_BIT			15
#define IEEE_FLT16_SIGN_MASK		( 1U << IEEE_FLT16_SIGN_BIT )

#define IEEE_FLT_MANTISSA_BITS		23
#define IEEE_FLT_EXPONENT_BITS		8
#define IEEE_FLT_EXPONENT_BIAS		127
#define IEEE_FLT_SIGN_BIT			31
#define IEEE_FLT_SIGN_MASK			( 1UL << IEEE_FLT_SIGN_BIT )

#define IEEE_DBL_MANTISSA_BITS		52
#define IEEE_DBL_EXPONENT_BITS		11
#define IEEE_DBL_EXPONENT_BIAS		1023
#define IEEE_DBL_SIGN_BIT			63
#define IEEE_DBL_SIGN_MASK			( 1ULL << IEEE_DBL_SIGN_BIT )

#define IEEE_DBLE_MANTISSA_BITS		63
#define IEEE_DBLE_EXPONENT_BITS		15
#define IEEE_DBLE_EXPONENT_BIAS		0
#define IEEE_DBLE_SIGN_BIT			79

/*
================================================================================================

	floating point sign bit tests

================================================================================================
*/

#define IEEE_FLT_SIGNBITSET( a )	(reinterpret_cast<const unsigned int &>(a) >> IEEE_FLT_SIGN_BIT)
#define IEEE_FLT_SIGNBITNOTSET( a )	((~reinterpret_cast<const unsigned int &>(a)) >> IEEE_FLT_SIGN_BIT)
#define IEEE_FLT_ISNOTZERO( a )		(reinterpret_cast<const unsigned int &>(a) & ~(1u<<IEEE_FLT_SIGN_BIT))

/*
================================================================================================

	floating point special value tests

================================================================================================
*/

/*
========================
IEEE_FLT_IS_NAN
========================
*/
HF_INLINE_EXTERN bool IEEE_FLT_IS_NAN( float x ) {
	return x != x;
}

/*
========================
IEEE_FLT_IS_INF
========================
*/
HF_INLINE_EXTERN bool IEEE_FLT_IS_INF( float x ) {
	return x == x && x * 0 != x * 0;
}

/*
========================
IEEE_FLT_IS_INF_NAN
========================
*/
HF_INLINE_EXTERN bool IEEE_FLT_IS_INF_NAN( float x ) {
	return x * 0 != x * 0;
}

/*
========================
IEEE_FLT_IS_IND
========================
*/
HF_INLINE_EXTERN bool IEEE_FLT_IS_IND( float x ) {
	return	(reinterpret_cast<const unsigned int &>(x) == 0xffc00000); 
}

/*
========================
IEEE_FLT_IS_DENORMAL
========================
*/
HF_INLINE_EXTERN bool IEEE_FLT_IS_DENORMAL( float x ) {
	return ((reinterpret_cast<const unsigned int &>(x) & 0x7f800000) == 0x00000000 &&
			(reinterpret_cast<const unsigned int &>(x) & 0x007fffff) != 0x00000000 ); 
}


/*
========================
IsNAN
========================
*/template<class type>
HF_INLINE_EXTERN bool IsNAN( const type &v ) {
	for ( int i = 0; i < v.GetDimension(); i++ ) {
		const float f = v.ToFloatPtr()[i];
		if ( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) ) {
			return true;
		}
	}
	return false;
}

/*
========================
IsValid
========================
*/
template<class type>
HF_INLINE_EXTERN bool IsValid( const type &v ) {
	for ( int i = 0; i < v.GetDimension(); i++ ) {
		const float f = v.ToFloatPtr()[i];
		if ( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) || IEEE_FLT_IS_DENORMAL( f ) ) {
			return false;
		}
	}
	return true;
}

/*
========================
IsValid
========================
*/
template<>
HF_INLINE_EXTERN bool IsValid( const float & f ) {	// these parameter must be a reference for the function to be considered a specialization
	return !( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) || IEEE_FLT_IS_DENORMAL( f ) );
}

/*
========================
IsNAN
========================
*/
template<>
HF_INLINE_EXTERN bool IsNAN( const float & f ) {	// these parameter must be a reference for the function to be considered a specialization
	if ( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) ) {
		return true;
	}
	return false;
}

/*
========================
IsInRange

Returns true if any scalar is greater than the range or less than the negative range.
========================
*/
template<class type>
HF_INLINE_EXTERN bool IsInRange( const type &v, const float range ) {
	for ( int i = 0; i < v.GetDimension(); i++ ) {
		const float f = v.ToFloatPtr()[i];
		if ( f > range || f < -range ) {
			return false;
		}
	}
	return true;
}


/*
================================================================================================

	MinIndex/MaxIndex

================================================================================================
*/
template<class T> HF_INLINE int	MaxIndex( T x, T y ) { return  ( x > y ) ? 0 : 1; }
template<class T> HF_INLINE int	MinIndex( T x, T y ) { return ( x < y ) ? 0 : 1; }

template<class T> HF_INLINE T	Max3( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? x : z ) : ( ( y > z ) ? y : z ); }
template<class T> HF_INLINE T	Min3( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? x : z ) : ( ( y < z ) ? y : z ); }
template<class T> HF_INLINE int	Max3Index( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? 0 : 2 ) : ( ( y > z ) ? 1 : 2 ); }
template<class T> HF_INLINE int	Min3Index( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? 0 : 2 ) : ( ( y < z ) ? 1 : 2 ); }

/*
================================================================================================

	Sign/Square/Cube

================================================================================================
*/
template<class T> HF_INLINE T	Sign( T f ) { return ( f > 0 ) ? 1 : ( ( f < 0 ) ? -1 : 0 ); }
template<class T> HF_INLINE T	Square( T x ) { return x * x; }
template<class T> HF_INLINE T	Cube( T x ) { return x * x * x; }

class hfMath {
public:

	static void					Init();

	static float				InvSqrt( float x );			// inverse square root with 32 bits precision, returns huge number when x == 0.0
	static float				InvSqrt16( float x );		// inverse square root with 16 bits precision, returns huge number when x == 0.0

	static float				Sqrt( float x );			// square root with 32 bits precision
	static float				Sqrt16( float x );			// square root with 16 bits precision

	static float				Sin( float a );				// sine with 32 bits precision
	static float				Sin16( float a );			// sine with 16 bits precision, maximum absolute error is 2.3082e-09

	static float				Cos( float a );				// cosine with 32 bits precision
	static float				Cos16( float a );			// cosine with 16 bits precision, maximum absolute error is 2.3082e-09

	static void					SinCos( float a, float &s, float &c );		// sine and cosine with 32 bits precision
	static void					SinCos16( float a, float &s, float &c );	// sine and cosine with 16 bits precision

	static float				Tan( float a );				// tangent with 32 bits precision
	static float				Tan16( float a );			// tangent with 16 bits precision, maximum absolute error is 1.8897e-08

	static float				ASin( float a );			// arc sine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
	static float				ASin16( float a );			// arc sine with 16 bits precision, maximum absolute error is 6.7626e-05

	static float				ACos( float a );			// arc cosine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
	static float				ACos16( float a );			// arc cosine with 16 bits precision, maximum absolute error is 6.7626e-05

	static float				ATan( float a );			// arc tangent with 32 bits precision
	static float				ATan16( float a );			// arc tangent with 16 bits precision, maximum absolute error is 1.3593e-08

	static float				ATan( float y, float x );	// arc tangent with 32 bits precision
	static float				ATan16( float y, float x );	// arc tangent with 16 bits precision, maximum absolute error is 1.3593e-08

	static float				Pow( float x, float y );	// x raised to the power y with 32 bits precision
	static float				Pow16( float x, float y );	// x raised to the power y with 16 bits precision

	static float				Exp( float f );				// e raised to the power f with 32 bits precision
	static float				Exp16( float f );			// e raised to the power f with 16 bits precision

	static float				Log( float f );				// natural logarithm with 32 bits precision
	static float				Log16( float f );			// natural logarithm with 16 bits precision

	static int					IPow( int x, int y );		// integral x raised to the power y
	static int					ILog2( float f );			// integral base-2 logarithm of the floating point value
	static int					ILog2( int i );				// integral base-2 logarithm of the integer value

	static int					BitsForFloat( float f );	// minumum number of bits required to represent ceil( f )
	static int					BitsForInteger( int i );	// minumum number of bits required to represent i
	static int					MaskForFloatSign( float f );// returns 0x00000000 if x >= 0.0f and returns 0xFFFFFFFF if x <= -0.0f
	static int					MaskForIntegerSign( int i );// returns 0x00000000 if x >= 0 and returns 0xFFFFFFFF if x < 0
	static int					FloorPowerOfTwo( int x );	// round x down to the nearest power of 2
	static int					CeilPowerOfTwo( int x );	// round x up to the nearest power of 2
	static bool					IsPowerOfTwo( int x );		// returns true if x is a power of 2
	static int					BitCount( int x );			// returns the number of 1 bits in x
	static int					BitReverse( int x );		// returns the bit reverse of x

	static int					Abs( int x );				// returns the absolute value of the integer value (for reference only)
	static float				Fabs( float f );			// returns the absolute value of the floating point value
	static float				Floor( float f );			// returns the largest integer that is less than or equal to the given value
	static float				Ceil( float f );			// returns the smallest integer that is greater than or equal to the given value
	static float				Rint( float f );			// returns the nearest integer

	static float				Frac( float f );			// f - Floor( f )

	static int					Ftoi( float f );			// float to int conversion
	static char					Ftoi8( float f );			// float to char conversion
	static short				Ftoi16( float f );			// float to short conversion
	static unsigned short		Ftoui16( float f );			// float to unsigned short conversion
	static byte					Ftob( float f );			// float to byte conversion, the result is clamped to the range [0-255]

	static signed char			ClampChar( int i );
	static signed short			ClampShort( int i );
	static int					ClampInt( int min, int max, int value );
	static float				ClampFloat( float min, float max, float value );

	static float				AngleNormalize360( float angle );
	static float				AngleNormalize180( float angle );
	static float				AngleDelta( float angle1, float angle2 );

	static int					FloatToBits( float f, int exponentBits, int mantissaBits );
	static float				BitsToFloat( int i, int exponentBits, int mantissaBits );

	static int					FloatHash( const float *array, const int numFloats );

	static float				LerpToWithScale( const float cur, const float dest, const float scale );

	static const float			PI;							// pi
	static const float			TWO_PI;						// pi * 2
	static const float			HALF_PI;					// pi / 2
	static const float			ONEFOURTH_PI;				// pi / 4
	static const float			ONEOVER_PI;					// 1 / pi
	static const float			ONEOVER_TWOPI;				// 1 / pi * 2
	static const float			E;							// e
	static const float			SQRT_TWO;					// sqrt( 2 )
	static const float			SQRT_THREE;					// sqrt( 3 )
	static const float			SQRT_1OVER2;				// sqrt( 1 / 2 )
	static const float			SQRT_1OVER3;				// sqrt( 1 / 3 )
	static const float			M_DEG2RAD;					// degrees to radians multiplier
	static const float			M_RAD2DEG;					// radians to degrees multiplier
	static const float			M_SEC2MS;					// seconds to milliseconds multiplier
	static const float			M_MS2SEC;					// milliseconds to seconds multiplier
	static const float			INFINITY;					// huge number which should be larger than any valid number used
	static const float			FLT_EPSILON;				// smallest positive number such that 1.0+FLT_EPSILON != 1.0
	static const float			FLT_SMALLEST_NON_DENORMAL;	// smallest non-denormal 32-bit floating point value

#if defined( HF_WIN_X86_SSE_INTRIN )
	static const __m128				SIMD_SP_zero;
	static const __m128				SIMD_SP_255;
	static const __m128				SIMD_SP_min_char;
	static const __m128				SIMD_SP_max_char;
	static const __m128				SIMD_SP_min_short;
	static const __m128				SIMD_SP_max_short;
	static const __m128				SIMD_SP_smallestNonDenorm;
	static const __m128				SIMD_SP_tiny;
	static const __m128				SIMD_SP_rsqrt_c0;
	static const __m128				SIMD_SP_rsqrt_c1;
#endif

private:
	enum {
		LOOKUP_BITS				= 8,							
		EXP_POS					= 23,							
		EXP_BIAS				= 127,							
		LOOKUP_POS				= (EXP_POS-LOOKUP_BITS),
		SEED_POS				= (EXP_POS-8),
		SQRT_TABLE_SIZE			= (2<<LOOKUP_BITS),
		LOOKUP_MASK				= (SQRT_TABLE_SIZE-1)
	};

	union _flint {
		dword					i;
		float					f;
	};

	static dword				iSqrt[SQRT_TABLE_SIZE];
	static bool					initialized;
};

HF_INLINE byte CLAMP_BYTE( int x )	{ 
	return ( (x) < 0 ? (0) : ( (x) > 255 ? 255 : (byte)(x) ) ); 
}

/*
========================
hfMath::InvSqrt
========================
*/
HF_INLINE float hfMath::InvSqrt( float x ) {
#ifdef HF_WIN_X86_SSE_INTRIN

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#else

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#endif
}

/*
========================
hfMath::InvSqrt16
========================
*/
HF_INLINE float hfMath::InvSqrt16( float x ) {
#ifdef HF_WIN_X86_SSE_INTRIN

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#else

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#endif
}

/*
========================
hfMath::Sqrt
========================
*/
HF_INLINE float hfMath::Sqrt( float x ) {
#ifdef HF_WIN_X86_SSE_INTRIN
	return ( x >= 0.0f ) ?  x * InvSqrt( x ) : 0.0f;
#else
	return ( x >= 0.0f ) ? sqrtf( x ) : 0.0f;
#endif
}

/*
========================
hfMath::Sqrt16
========================
*/
HF_INLINE float hfMath::Sqrt16( float x ) {
#ifdef HF_WIN_X86_SSE_INTRIN
	return ( x >= 0.0f ) ?  x * InvSqrt16( x ) : 0.0f;
#else
	return ( x >= 0.0f ) ? sqrtf( x ) : 0.0f;
#endif
}

/*
========================
hfMath::Frac
========================
*/
HF_INLINE float hfMath::Frac( float f ) {
	return f - floorf( f );
}

/*
========================
hfMath::Sin
========================
*/
HF_INLINE float hfMath::Sin( float a ) {
	return sinf( a );
}

/*
========================
hfMath::Sin16
========================
*/
HF_INLINE float hfMath::Sin16( float a ) {
	float s;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a * ONEOVER_TWOPI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
		} else {
			a = PI - a;
		}
	}
#else
	a = PI - a;
	if ( fabsf( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
	}
#endif
	s = a * a;
	return a * ( ( ( ( ( -2.39e-08f * s + 2.7526e-06f ) * s - 1.98409e-04f ) * s + 8.3333315e-03f ) * s - 1.666666664e-01f ) * s + 1.0f );
}

/*
========================
hfMath::Cos
========================
*/
HF_INLINE float hfMath::Cos( float a ) {
	return cosf( a );
}

/*
========================
hfMath::Cos16
========================
*/
HF_INLINE float hfMath::Cos16( float a ) {
	float s, d;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a * ONEOVER_TWOPI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
			d = -1.0f;
		} else {
			d = 1.0f;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
			d = 1.0f;
		} else {
			a = PI - a;
			d = -1.0f;
		}
	}
#else
	a = PI - a;
	if ( fabsf( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
		d = 1.0f;
	} else {
		d = -1.0f;
	}
#endif
	s = a * a;
	return d * ( ( ( ( ( -2.605e-07f * s + 2.47609e-05f ) * s - 1.3888397e-03f ) * s + 4.16666418e-02f ) * s - 4.999999963e-01f ) * s + 1.0f );
}

/*
========================
hfMath::SinCos
========================
*/
HF_INLINE void hfMath::SinCos( float a, float &s, float &c ) {
#if defined( HF_WIN_X86_ASM )
	_asm {
		fld		a
		fsincos
		mov		ecx, c
		mov		edx, s
		fstp	dword ptr [ecx]
		fstp	dword ptr [edx]
	}
#else
	s = sinf( a );
	c = cosf( a );
#endif
}

/*
========================
hfMath::SinCos16
========================
*/
HF_INLINE void hfMath::SinCos16( float a, float &s, float &c ) {
	float t, d;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a * ONEOVER_TWOPI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
			d = -1.0f;
		} else {
			d = 1.0f;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
			d = 1.0f;
		} else {
			a = PI - a;
			d = -1.0f;
		}
	}
#else
	a = PI - a;
	if ( fabsf( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
		d = 1.0f;
	} else {
		d = -1.0f;
	}
#endif
	t = a * a;
	s = a * ( ( ( ( ( -2.39e-08f * t + 2.7526e-06f ) * t - 1.98409e-04f ) * t + 8.3333315e-03f ) * t - 1.666666664e-01f ) * t + 1.0f );
	c = d * ( ( ( ( ( -2.605e-07f * t + 2.47609e-05f ) * t - 1.3888397e-03f ) * t + 4.16666418e-02f ) * t - 4.999999963e-01f ) * t + 1.0f );
}

/*
========================
hfMath::Tan
========================
*/
HF_INLINE float hfMath::Tan( float a ) {
	return tanf( a );
}

/*
========================
hfMath::Tan16
========================
*/
HF_INLINE float hfMath::Tan16( float a ) {
	float s;
	bool reciprocal;

	if ( ( a < 0.0f ) || ( a >= PI ) ) {
		a -= floorf( a * ONEOVER_PI ) * PI;
	}
#if 1
	if ( a < HALF_PI ) {
		if ( a > ONEFOURTH_PI ) {
			a = HALF_PI - a;
			reciprocal = true;
		} else {
			reciprocal = false;
		}
	} else {
		if ( a > HALF_PI + ONEFOURTH_PI ) {
			a = a - PI;
			reciprocal = false;
		} else {
			a = HALF_PI - a;
			reciprocal = true;
		}
	}
#else
	a = HALF_PI - a;
	if ( fabsf( a ) >= ONEFOURTH_PI ) {
		a = ( ( a < 0.0f ) ? -HALF_PI : HALF_PI ) - a;
		reciprocal = false;
	} else {
		reciprocal = true;
	}
#endif
	s = a * a;
	s = a * ( ( ( ( ( ( 9.5168091e-03f * s + 2.900525e-03f ) * s + 2.45650893e-02f ) * s + 5.33740603e-02f ) * s + 1.333923995e-01f ) * s + 3.333314036e-01f ) * s + 1.0f );
	if ( reciprocal ) {
		return 1.0f / s;
	} else {
		return s;
	}
}

/*
========================
hfMath::ASin
========================
*/
HF_INLINE float hfMath::ASin( float a ) {
	if ( a <= -1.0f ) {
		return -HALF_PI;
	}
	if ( a >= 1.0f ) {
		return HALF_PI;
	}
	return asinf( a );
}

/*
========================
hfMath::ASin16
========================
*/
HF_INLINE float hfMath::ASin16( float a ) {
	if ( a < 0.0f ) {
		if ( a <= -1.0f ) {
			return -HALF_PI;
		}
		a = fabsf( a );
		return ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * hfMath::Sqrt( 1.0f - a ) - HALF_PI;
	} else {
		if ( a >= 1.0f ) {
			return HALF_PI;
		}
		return HALF_PI - ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * hfMath::Sqrt( 1.0f - a );
	}
}

/*
========================
hfMath::ACos
========================
*/
HF_INLINE float hfMath::ACos( float a ) {
	if ( a <= -1.0f ) {
		return PI;
	}
	if ( a >= 1.0f ) {
		return 0.0f;
	}
	return acosf( a );
}

/*
========================
hfMath::ACos16
========================
*/
HF_INLINE float hfMath::ACos16( float a ) {
	if ( a < 0.0f ) {
		if ( a <= -1.0f ) {
			return PI;
		}
		a = fabsf( a );
		return PI - ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * hfMath::Sqrt( 1.0f - a );
	} else {
		if ( a >= 1.0f ) {
			return 0.0f;
		}
		return ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * hfMath::Sqrt( 1.0f - a );
	}
}

/*
========================
hfMath::ATan
========================
*/
HF_INLINE float hfMath::ATan( float a ) {
	return atanf( a );
}

/*
========================
hfMath::ATan16
========================
*/
HF_INLINE float hfMath::ATan16( float a ) {
	float s;
	if ( fabsf( a ) > 1.0f ) {
		a = 1.0f / a;
		s = a * a;
		s = - ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
				* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
		if ( a < 0.0f ) {
			return s - HALF_PI;
		} else {
			return s + HALF_PI;
		}
	} else {
		s = a * a;
		return ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
			* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
	}
}

/*
========================
hfMath::ATan
========================
*/
HF_INLINE float hfMath::ATan( float y, float x ) {
	assert( fabs( y ) > hfMath::FLT_SMALLEST_NON_DENORMAL || fabs( x ) > hfMath::FLT_SMALLEST_NON_DENORMAL );
	return atan2f( y, x );
}

/*
========================
hfMath::ATan16
========================
*/
HF_INLINE float hfMath::ATan16( float y, float x ) {
	assert( fabs( y ) > hfMath::FLT_SMALLEST_NON_DENORMAL || fabs( x ) > hfMath::FLT_SMALLEST_NON_DENORMAL );

	float a, s;
	if ( fabsf( y ) > fabsf( x ) ) {
		a = x / y;
		s = a * a;
		s = - ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
				* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
		if ( a < 0.0f ) {
			return s - HALF_PI;
		} else {
			return s + HALF_PI;
		}
	} else {
		a = y / x;
		s = a * a;
		return ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
			* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
	}
}

/*
========================
hfMath::Pow
========================
*/
HF_INLINE float hfMath::Pow( float x, float y ) {
	return powf( x, y );
}

/*
========================
hfMath::Pow16
========================
*/
HF_INLINE float hfMath::Pow16( float x, float y ) {
	return Exp16( y * Log16( x ) );
}

/*
========================
hfMath::Exp
========================
*/
HF_INLINE float hfMath::Exp( float f ) {
	return expf( f );
}

/*
========================
hfMath::Exp16
========================
*/
HF_INLINE float hfMath::Exp16( float f ) {
	float x = f * 1.44269504088896340f;		// multiply with ( 1 / log( 2 ) )
#if 1
	int i = *reinterpret_cast<int *>(&x);
	int s = ( i >> IEEE_FLT_SIGN_BIT );
	int e = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	int m = ( i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 ) ) | ( 1 << IEEE_FLT_MANTISSA_BITS );
	i = ( ( m >> ( IEEE_FLT_MANTISSA_BITS - e ) ) & ~( e >> INT32_SIGN_BIT ) ) ^ s;
#else
	int i = (int) x;
	if ( x < 0.0f ) {
		i--;
	}
#endif
	int exponent = ( i + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS;
	float y = *reinterpret_cast<float *>(&exponent);
	x -= (float) i;
	if ( x >= 0.5f ) {
		x -= 0.5f;
		y *= 1.4142135623730950488f;	// multiply with sqrt( 2 )
	}
	float x2 = x * x;
	float p = x * ( 7.2152891511493f + x2 * 0.0576900723731f );
	float q = 20.8189237930062f + x2;
	x = y * ( q + p ) / ( q - p );
	return x;
}

/*
========================
hfMath::Log
========================
*/
HF_INLINE float hfMath::Log( float f ) {
	return logf( f );
}

/*
========================
hfMath::Log16
========================
*/
HF_INLINE float hfMath::Log16( float f ) {
	int i = *reinterpret_cast<int *>(&f);
	int exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	i -= ( exponent + 1 ) << IEEE_FLT_MANTISSA_BITS;	// get value in the range [.5, 1>
	float y = *reinterpret_cast<float *>(&i);
	y *= 1.4142135623730950488f;						// multiply with sqrt( 2 )
	y = ( y - 1.0f ) / ( y + 1.0f );
	float y2 = y * y;
	y = y * ( 2.000000000046727f + y2 * ( 0.666666635059382f + y2 * ( 0.4000059794795f + y2 * ( 0.28525381498f + y2 * 0.2376245609f ) ) ) );
	y += 0.693147180559945f * ( (float)exponent + 0.5f );
	return y;
}

/*
========================
hfMath::IPow
========================
*/
HF_INLINE int hfMath::IPow( int x, int y ) {
	int r; for( r = x; y > 1; y-- ) { r *= x; } return r;
}

/*
========================
hfMath::ILog2
========================
*/
HF_INLINE int hfMath::ILog2( float f ) {
	return ( ( (*reinterpret_cast<int *>(&f)) >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
}

/*
========================
hfMath::ILog2
========================
*/
HF_INLINE int hfMath::ILog2( int i ) {
	return ILog2( (float)i );
}

/*
========================
hfMath::BitsForFloat
========================
*/
HF_INLINE int hfMath::BitsForFloat( float f ) {
	return ILog2( f ) + 1;
}

/*
========================
hfMath::BitsForInteger
========================
*/
HF_INLINE int hfMath::BitsForInteger( int i ) {
	return ILog2( (float)i ) + 1;
}

/*
========================
hfMath::MaskForFloatSign
========================
*/
HF_INLINE int hfMath::MaskForFloatSign( float f ) {
	return ( (*reinterpret_cast<int *>(&f)) >> IEEE_FLT_SIGN_BIT );
}

/*
========================
hfMath::MaskForIntegerSign
========================
*/
HF_INLINE int hfMath::MaskForIntegerSign( int i ) {
	return ( i >> INT32_SIGN_BIT );
}

/*
========================
hfMath::FloorPowerOfTwo
========================
*/
HF_INLINE int hfMath::FloorPowerOfTwo( int x ) {
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x >> 1;
}

/*
========================
hfMath::CeilPowerOfTwo
========================
*/
HF_INLINE int hfMath::CeilPowerOfTwo( int x ) {
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;
}

/*
========================
hfMath::IsPowerOfTwo
========================
*/
HF_INLINE bool hfMath::IsPowerOfTwo( int x ) {
	return ( x & ( x - 1 ) ) == 0 && x > 0;
}

/*
========================
hfMath::BitCount
========================
*/
HF_INLINE int hfMath::BitCount( int x ) {
	x -= ( ( x >> 1 ) & 0x55555555 );
	x = ( ( ( x >> 2 ) & 0x33333333 ) + ( x & 0x33333333 ) );
	x = ( ( ( x >> 4 ) + x ) & 0x0f0f0f0f );
	x += ( x >> 8 );
	return ( ( x + ( x >> 16 ) ) & 0x0000003f );
}

/*
========================
hfMath::BitReverse
========================
*/
HF_INLINE int hfMath::BitReverse( int x ) {
	x = ( ( ( x >> 1 ) & 0x55555555 ) | ( ( x & 0x55555555 ) << 1 ) );
	x = ( ( ( x >> 2 ) & 0x33333333 ) | ( ( x & 0x33333333 ) << 2 ) );
	x = ( ( ( x >> 4 ) & 0x0f0f0f0f ) | ( ( x & 0x0f0f0f0f ) << 4 ) );
	x = ( ( ( x >> 8 ) & 0x00ff00ff ) | ( ( x & 0x00ff00ff ) << 8 ) );
	return ( ( x >> 16 ) | ( x << 16 ) );
}

/*
========================
hfMath::Abs
========================
*/
HF_INLINE int hfMath::Abs( int x ) {
#if 1
	return abs( x );
#else
   int y = x >> INT32_SIGN_BIT;
   return ( ( x ^ y ) - y );
#endif
}

/*
========================
hfMath::Fabs
========================
*/
HF_INLINE float hfMath::Fabs( float f ) {
#if 1
	return fabsf( f );
#else
	int tmp = *reinterpret_cast<int *>( &f );
	tmp &= 0x7FFFFFFF;
	return *reinterpret_cast<float *>( &tmp );
#endif
}

/*
========================
hfMath::Floor
========================
*/
HF_INLINE float hfMath::Floor( float f ) {
	return floorf( f );
}

/*
========================
hfMath::Ceil
========================
*/
HF_INLINE float hfMath::Ceil( float f ) {
	return ceilf( f );
}

/*
========================
hfMath::Rint
========================
*/
HF_INLINE float hfMath::Rint( float f ) {
	return floorf( f + 0.5f );
}


/*
========================
hfMath::Ftoi
========================
*/
HF_INLINE int hfMath::Ftoi( float f ) {
#ifdef HF_WIN_X86_SSE_INTRIN
	// If a converted result is larger than the maximum signed doubleword integer,
	// the floating-point invalid exception is raised, and if this exception is masked,
	// the indefinite integer value (80000000H) is returned.
	__m128 x = _mm_load_ss( &f );
	return _mm_cvttss_si32( x );
#elif 0 // round chop (C/C++ standard)
	int i, s, e, m, shift;
	i = *reinterpret_cast<int *>(&f);
	s = i >> IEEE_FLT_SIGN_BIT;
	e = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	m = ( i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 ) ) | ( 1 << IEEE_FLT_MANTISSA_BITS );
	shift = e - IEEE_FLT_MANTISSA_BITS;
	return ( ( ( ( m >> -shift ) | ( m << shift ) ) & ~( e >> INT32_SIGN_BIT ) ) ^ s ) - s;
#else
	// If a converted result is larger than the maximum signed doubleword integer the result is undefined.
	return C_FLOAT_TO_INT( f );
#endif
}

/*
========================
hfMath::Ftoi8
========================
*/
HF_INLINE char hfMath::Ftoi8( float f ) {
#ifdef HF_WIN_X86_SSE_INTRIN
	__m128 x = _mm_load_ss( &f );
	x = _mm_max_ss( x, SIMD_SP_min_char );
	x = _mm_min_ss( x, SIMD_SP_max_char );
	return static_cast<char>( _mm_cvttss_si32( x ) );
#else
	// The converted result is clamped to the range [-128,127].
	int i = C_FLOAT_TO_INT( f );
	if ( i < -128 ) {
		return -128;
	} else if ( i > 127 ) {
		return 127;
	}
	return static_cast<char>( i );
#endif
}

/*
========================
hfMath::Ftoi16
========================
*/
HF_INLINE short hfMath::Ftoi16( float f ) {
#ifdef HF_WIN_X86_SSE_INTRIN
	__m128 x = _mm_load_ss( &f );
	x = _mm_max_ss( x, SIMD_SP_min_short );
	x = _mm_min_ss( x, SIMD_SP_max_short );
	return static_cast<short>( _mm_cvttss_si32( x ) );
#else
	// The converted result is clamped to the range [-32768,32767].
	int i = C_FLOAT_TO_INT( f );
	if ( i < -32768 ) {
		return -32768;
	} else if ( i > 32767 ) {
		return 32767;
	}
	return static_cast<short>( i );
#endif
}

/*
========================
hfMath::Ftoui16
========================
*/
HF_INLINE unsigned short hfMath::Ftoui16( float f ) {
	// TO DO - SSE ??

	// The converted result is clamped to the range [-32768,32767].
	int i = C_FLOAT_TO_INT( f );
	if ( i < 0 ) {
		return 0;
	} else if ( i > 65535 ) {
		return 65535;
	}
	return static_cast<unsigned short>( i );
}

/*
========================
hfMath::Ftob
========================
*/
HF_INLINE byte hfMath::Ftob( float f ) {
#ifdef HF_WIN_X86_SSE_INTRIN
	// If a converted result is negative the value (0) is returned and if the
	// converted result is larger than the maximum byte the value (255) is returned.
	__m128 x = _mm_load_ss( &f );
	x = _mm_max_ss( x, SIMD_SP_zero );
	x = _mm_min_ss( x, SIMD_SP_255 );
	return static_cast<byte>( _mm_cvttss_si32( x ) );
#else
	// The converted result is clamped to the range [0,255].
	int i = C_FLOAT_TO_INT( f );
	if ( i < 0 ) {
		return 0;
	} else if ( i > 255 ) {
		return 255;
	}
	return static_cast<byte>( i );
#endif
}

/*
========================
hfMath::ClampChar
========================
*/
HF_INLINE signed char hfMath::ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return static_cast<signed char>( i );
}

/*
========================
hfMath::ClampShort
========================
*/
HF_INLINE signed short hfMath::ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 32767 ) {
		return 32767;
	}
	return static_cast<signed short>( i );
}

/*
========================
hfMath::ClampInt
========================
*/
HF_INLINE int hfMath::ClampInt( int min, int max, int value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

/*
========================
hfMath::ClampFloat
========================
*/
HF_INLINE float hfMath::ClampFloat( float min, float max, float value ) {
	return Max( min, Min( max, value ) );
}

/*
========================
hfMath::AngleNormalize360
========================
*/
HF_INLINE float hfMath::AngleNormalize360( float angle ) {
	if ( ( angle >= 360.0f ) || ( angle < 0.0f ) ) {
		angle -= floorf( angle * ( 1.0f / 360.0f ) ) * 360.0f;
	}
	return angle;
}

/*
========================
hfMath::AngleNormalize180
========================
*/
HF_INLINE float hfMath::AngleNormalize180( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0f ) {
		angle -= 360.0f;
	}
	return angle;
}

/*
========================
hfMath::AngleDelta
========================
*/
HF_INLINE float hfMath::AngleDelta( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

/*
========================
hfMath::FloatHash
========================
*/
HF_INLINE int hfMath::FloatHash( const float *array, const int numFloats ) {
	int i, hash = 0;
	const int *ptr;

	ptr = reinterpret_cast<const int *>( array );
	for ( i = 0; i < numFloats; i++ ) {
		hash ^= ptr[i];
	}
	return hash;
}

template< typename T >
HF_INLINE_EXTERN T Lerp( const T from, const T to, float f ) { 
	return from + ( ( to - from ) * f );
}

template<>
HF_INLINE_EXTERN int Lerp( const int from, const int to, float f ) { 
	return hfMath::Ftoi( (float) from + ( ( (float) to - (float) from ) * f ) );
}


/*
========================
LerpToWithScale

Lerps from "cur" to "dest", scaling the delta to change by "scale"
If the delta between "cur" and "dest" is very small, dest is returned to prevent denormals.
========================
*/
inline float hfMath::LerpToWithScale( const float cur, const float dest, const float scale ) {
	float delta = dest - cur;
	if ( delta > -1.0e-6f && delta < 1.0e-6f ) {
		return dest;
	}
	return cur + ( dest - cur ) * scale;
}


#endif /* !__MATH_MATH_H__ */

