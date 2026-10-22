#pragma hdrstop
#include "../hfprecompiled.h"

const int SMALLEST_NON_DENORMAL					= 1<<IEEE_FLT_MANTISSA_BITS;
const int NAN_VALUE								= 0x7f800000;

const float	hfMath::PI				= 3.14159265358979323846f;
const float	hfMath::TWO_PI			= 2.0f * PI;
const float	hfMath::HALF_PI			= 0.5f * PI;
const float	hfMath::ONEFOURTH_PI	= 0.25f * PI;
const float hfMath::ONEOVER_PI		= 1.0f / hfMath::PI;
const float hfMath::ONEOVER_TWOPI	= 1.0f / hfMath::TWO_PI;
const float hfMath::E				= 2.71828182845904523536f;
const float hfMath::SQRT_TWO		= 1.41421356237309504880f;
const float hfMath::SQRT_THREE		= 1.73205080756887729352f;
const float	hfMath::SQRT_1OVER2		= 0.70710678118654752440f;
const float	hfMath::SQRT_1OVER3		= 0.57735026918962576450f;
const float	hfMath::M_DEG2RAD		= PI / 180.0f;
const float	hfMath::M_RAD2DEG		= 180.0f / PI;
const float	hfMath::M_SEC2MS		= 1000.0f;
const float	hfMath::M_MS2SEC		= 0.001f;
const float	hfMath::INFINITY		= 1e30f;
const float hfMath::FLT_EPSILON		= 1.192092896e-07f;
const float hfMath::FLT_SMALLEST_NON_DENORMAL	= * reinterpret_cast< const float * >( & SMALLEST_NON_DENORMAL );	// 1.1754944e-038f

#if defined( HF_WIN_X86_SSE_INTRIN )
const __m128 hfMath::SIMD_SP_zero				= { 0.0f, 0.0f, 0.0f, 0.0f };
const __m128 hfMath::SIMD_SP_255				= { 255.0f, 255.0f, 255.0f, 255.0f };
const __m128 hfMath::SIMD_SP_min_char			= { -128.0f, -128.0f, -128.0f, -128.0f };
const __m128 hfMath::SIMD_SP_max_char			= { 127.0f, 127.0f, 127.0f, 127.0f };
const __m128 hfMath::SIMD_SP_min_short			= { -32768.0f, -32768.0f, -32768.0f, -32768.0f };
const __m128 hfMath::SIMD_SP_max_short			= { 32767.0f, 32767.0f, 32767.0f, 32767.0f };
const __m128 hfMath::SIMD_SP_smallestNonDenorm	= { FLT_SMALLEST_NON_DENORMAL, FLT_SMALLEST_NON_DENORMAL, FLT_SMALLEST_NON_DENORMAL, FLT_SMALLEST_NON_DENORMAL };
const __m128 hfMath::SIMD_SP_tiny				= { 1e-4f, 1e-4f, 1e-4f, 1e-4f };
const __m128 hfMath::SIMD_SP_rsqrt_c0			= { 3.0f, 3.0f, 3.0f, 3.0f };
const __m128 hfMath::SIMD_SP_rsqrt_c1			= { -0.5f, -0.5f, -0.5f, -0.5f };
#endif

bool		hfMath::initialized		= false;
dword		hfMath::iSqrt[SQRT_TABLE_SIZE];		// inverse square root lookup table

/*
===============
hfMath::Init
===============
*/
void hfMath::Init() {
    union _flint fi, fo;

    for ( int i = 0; i < SQRT_TABLE_SIZE; i++ ) {
        fi.i	 = ((EXP_BIAS-1) << EXP_POS) | (i << LOOKUP_POS);
        fo.f	 = (float)( 1.0 / sqrt( fi.f ) );
        iSqrt[i] = ((dword)(((fo.i + (1<<(SEED_POS-2))) >> SEED_POS) & 0xFF))<<SEED_POS;
    }
    
	iSqrt[SQRT_TABLE_SIZE / 2] = ((dword)(0xFF))<<(SEED_POS); 

	initialized = true;
}

/*
================
hfMath::FloatToBits
================
*/
int hfMath::FloatToBits( float f, int exponentBits, int mantissaBits ) {
	int i, sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	int maxBits = ( ( ( 1 << ( exponentBits - 1 ) ) - 1 ) << mantissaBits ) | ( ( 1 << mantissaBits ) - 1 );
	int minBits = ( ( ( 1 <<   exponentBits       ) - 2 ) << mantissaBits ) | 1;

	float max = BitsToFloat( maxBits, exponentBits, mantissaBits );
	float min = BitsToFloat( minBits, exponentBits, mantissaBits );

	if ( f >= 0.0f ) {
		if ( f >= max ) {
			return maxBits;
		} else if ( f <= min ) {
			return minBits;
		}
	} else {
		if ( f <= -max ) {
			return ( maxBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		} else if ( f >= -min ) {
			return ( minBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		}
	}

	exponentBits--;
	i = *reinterpret_cast<int *>(&f);
	sign = ( i >> IEEE_FLT_SIGN_BIT ) & 1;
	exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	mantissa = i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 );
	value = sign << ( 1 + exponentBits + mantissaBits );
	value |= ( ( INT32_SIGNBITSET( exponent ) << exponentBits ) | ( abs( exponent ) & ( ( 1 << exponentBits ) - 1 ) ) ) << mantissaBits;
	value |= mantissa >> ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	return value;
}

/*
================
hfMath::BitsToFloat
================
*/
float hfMath::BitsToFloat( int i, int exponentBits, int mantissaBits ) {
	static int exponentSign[2] = { 1, -1 };
	int sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	exponentBits--;
	sign = i >> ( 1 + exponentBits + mantissaBits );
	exponent = ( ( i >> mantissaBits ) & ( ( 1 << exponentBits ) - 1 ) ) * exponentSign[( i >> ( exponentBits + mantissaBits ) ) & 1];
	mantissa = ( i & ( ( 1 << mantissaBits ) - 1 ) ) << ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	value = sign << IEEE_FLT_SIGN_BIT | ( exponent + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS | mantissa;
	return *reinterpret_cast<float *>(&value);
}

