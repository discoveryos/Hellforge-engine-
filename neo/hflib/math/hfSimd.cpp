#pragma hdrstop
#include "../hfprecompiled.h"

#include "hfSimd_Generic.h"
#include "hfSimd_SSE.h"

hfSIMDProcessor	*	processor = NULL;			// pointer to SIMD processor
hfSIMDProcessor *	generic = NULL;				// pointer to generic SIMD implementation
hfSIMDProcessor *	SIMDProcessor = NULL;

/*
================
hfSIMD::Init
================
*/
void hfSIMD::Init() {
	generic = new (TAG_MATH) hfSIMD_Generic;
	generic->cpuid = CPUID_GENERIC;
	processor = NULL;
	SIMDProcessor = generic;
}

/*
============
hfSIMD::InitProcessor
============
*/
void hfSIMD::InitProcessor( const char *module, bool forceGeneric ) {
	cpuid_t cpuid;
	hfSIMDProcessor *newProcessor;

	cpuid = hfLib::sys->GetProcessorId();

	if ( forceGeneric ) {

		newProcessor = generic;

	} else {

		if ( processor == NULL ) {
#if defined(__x86_64__) || defined(__i386__)
			if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) ) {
				processor = new (TAG_MATH) hfSIMD_SSE;
			} else {
				processor = generic;
			}
#else
			processor = generic;
#endif
			processor->cpuid = cpuid;
		}

		newProcessor = processor;
	}

	if ( newProcessor != SIMDProcessor ) {
		SIMDProcessor = newProcessor;
		hfLib::common->Printf( "%s using %s for SIMD processing\n", module, SIMDProcessor->GetName() );
	}

	if ( cpuid & CPUID_FTZ ) {
		hfLib::sys->FPU_SetFTZ( true );
		hfLib::common->Printf( "enabled Flush-To-Zero mode\n" );
	}

	if ( cpuid & CPUID_DAZ ) {
		hfLib::sys->FPU_SetDAZ( true );
		hfLib::common->Printf( "enabled Denormals-Are-Zero mode\n" );
	}
}

/*
================
hfSIMD::Shutdown
================
*/
void hfSIMD::Shutdown() {
	if ( processor != generic ) {
		delete processor;
	}
	delete generic;
	generic = NULL;
	processor = NULL;
	SIMDProcessor = NULL;
}


//===============================================================
//
// Test code
//
//===============================================================

#define COUNT		999			// data count (odd to catch edge cases)
#define BIG_COUNT	COUNT*5		// Some tests need a larger count
#define NUMTESTS	2048		// number of tests

#define RANDOM_SEED		1013904223L	//((int)hfLib::sys->GetClockTicks())

hfSIMDProcessor *p_simd;
hfSIMDProcessor *p_generic;
long baseClocks = 0;


#define TIME_TYPE int

#if defined(_MSC_VER)
#pragma warning(disable : 4731)     // frame pointer register 'ebx' modified by inline assembly code

long saved_ebx = 0;

#define StartRecordTime( start )			\
	__asm mov saved_ebx, ebx				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov start, eax					\
	__asm xor eax, eax						\
	__asm cpuid

#define StopRecordTime( end )				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov end, eax						\
	__asm mov ebx, saved_ebx				\
	__asm xor eax, eax						\
	__asm cpuid
#elif defined(__x86_64__) || defined(__i386__)
// GCC/Clang inline assembly RDTSC timing
#define StartRecordTime( start )			\
	__asm__ __volatile__( "cpuid\n\trdtsc\n\tmov %%eax, %0\n\txor %%eax,%%eax\n\tcpuid" : "=r"( start ) :: "eax", "ebx", "ecx", "edx" )

#define StopRecordTime( end )				\
	__asm__ __volatile__( "xor %%eax,%%eax\n\tcpuid\n\trdtsc\n\tmov %%eax, %0\n\tcpuid" : "=r"( end ) :: "eax", "ebx", "ecx", "edx" )
#else
#include <chrono>

#define StartRecordTime( start )			\
	{ std::chrono::steady_clock::time_point _t = std::chrono::steady_clock::now(); (start) = (int)std::chrono::duration_cast<std::chrono::nanoseconds>( _t.time_since_epoch() ).count(); }

#define StopRecordTime( end )				\
	{ std::chrono::steady_clock::time_point _t = std::chrono::steady_clock::now(); (end) = (int)std::chrono::duration_cast<std::chrono::nanoseconds>( _t.time_since_epoch() ).count(); }
#endif


#define GetBest( start, end, best )			\
	if ( !best || end - start < best ) {	\
		best = end - start;					\
	}


/*
============
PrintClocks
============
*/
void PrintClocks( char *string, int dataCount, int clocks, int otherClocks = 0 ) {
	int i;

	hfLib::common->Printf( string );
	for ( i = hfStr::LengthWithoutColors(string); i < 48; i++ ) {
		hfLib::common->Printf(" ");
	}
	clocks -= baseClocks;
	if ( otherClocks && clocks ) {
		otherClocks -= baseClocks;
		float p = (float)otherClocks / (float)clocks;
		hfLib::common->Printf( "c = %4d, clcks = %5d, %.1fX\n", dataCount, clocks, p );
	} else {
		hfLib::common->Printf( "c = %4d, clcks = %5d\n", dataCount, clocks );
	}
}

/*
============
GetBaseClocks
============
*/
void GetBaseClocks() {
	int i, start, end, bestClocks;

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	baseClocks = bestClocks;
}

/*
============
TestMinMax
============
*/
void TestMinMax() {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( hfVec2 v2src0[COUNT] );
	ALIGN16( hfVec3 v3src0[COUNT] );
	ALIGN16( hfDrawVert drawVerts[COUNT] );
	ALIGN16( triIndex_t indexes[COUNT] );
	float min = 0.0f, max = 0.0f, min2 = 0.0f, max2 = 0.0f;
	hfVec2 v2min, v2max, v2min2, v2max2;
	hfVec3 vmin, vmax, vmin2, vmax2;
	const char *result;

	hfRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][2] = srnd.CRandomFloat() * 10.0f;
		drawVerts[i].xyz = v3src0[i];
		indexes[i] = i;
	}

	hfLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		min = hfMath::INFINITY;
		max = -hfMath::INFINITY;
		StartRecordTime( start );
		p_generic->MinMax( min, max, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( min2, max2, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( min == min2 && max == max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( v2min, v2max, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( hfVec2[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( v2min2, v2max2, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( v2min == v2min2 && v2max == v2max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( hfVec2[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( hfVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( hfVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( hfDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( hfDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( hfDrawVert[], indexes[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( hfDrawVert[], indexes[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemcpy
============
*/
void TestMemcpy() {
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	int i;
	byte test0[BIG_COUNT];
	byte test1[BIG_COUNT];
	const char * result;

	hfRandom random( RANDOM_SEED );
	for ( i = 0; i < BIG_COUNT; i++ ) {
		test0[i] = random.RandomInt( 255 );
	}

	hfLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Memcpy( test1, test0, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memcpy()", BIG_COUNT, bestClocksGeneric );

	for ( i = 0; i < BIG_COUNT; i++ ) {
		test0[i] = random.RandomInt( 255 );
	}

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Memcpy( test1, test0, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for ( i = 0; i < BIG_COUNT; i++ ) {
		if ( test1[i] != test0[i] ) {
			break;
		}
	}
	result = ( i >= BIG_COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memcpy() %s", result), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemset
============
*/
void TestMemset() {
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	int i, j;
	const char * result;
	byte test0[BIG_COUNT];

	hfRandom random( RANDOM_SEED );
	j = 1 + random.RandomInt( 254 );

	hfLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memset()", BIG_COUNT, bestClocksGeneric );

	j = 1 + random.RandomInt( 254 );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for ( i = 0; i < BIG_COUNT; i++ ) {
		if ( test0[i] != j ) {
			break;
		}
	}
	result = ( i >= BIG_COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memset() %s", result), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );

	j = 0;

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memset( 0 )", BIG_COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for ( i = 0; i < BIG_COUNT; i++ ) {
		if ( test0[i] != j ) {
			break;
		}
	}
	result = ( i >= BIG_COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memset( 0 ) %s", result), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestBlendJoints
============
*/
void TestBlendJoints() {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	hfTempArray< hfJointQuat > baseJoints( COUNT );
	hfTempArray< hfJointQuat > joints1( COUNT );
	hfTempArray< hfJointQuat > joints2( COUNT );
	hfTempArray< hfJointQuat > blendJoints( COUNT );
	hfTempArray< int > index( COUNT );
	float lerp = 0.3f;
	const char *result;

	hfRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		hfAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].w = 0.0f;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].w = 0.0f;
		index[i] = i;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJoints( joints1.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJoints( joints2.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestBlendJoints
============
*/
void TestBlendJointsFast() {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	hfTempArray< hfJointQuat > baseJoints( COUNT );
	hfTempArray< hfJointQuat > joints1( COUNT );
	hfTempArray< hfJointQuat > joints2( COUNT );
	hfTempArray< hfJointQuat > blendJoints( COUNT );
	hfTempArray< int > index( COUNT );
	float lerp = 0.3f;
	const char *result;

	hfRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		hfAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].w = 0.0f;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].w = 0.0f;
		index[i] = i;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJointsFast( joints1.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJointsFast()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJointsFast( joints2.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJointsFast() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointQuatsToJointMats
============
*/
void TestConvertJointQuatsToJointMats() {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	hfTempArray< hfJointQuat > baseJoints( COUNT );
	hfTempArray< hfJointMat > joints1( COUNT );
	hfTempArray< hfJointMat > joints2( COUNT );
	const char *result;

	hfRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		hfAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointQuatsToJointMats( joints1.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointQuatsToJointMats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointQuatsToJointMats( joints2.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointQuatsToJointMats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointMatsToJointQuats
============
*/
void TestConvertJointMatsToJointQuats() {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	hfTempArray< hfJointMat > baseJoints( COUNT );
	hfTempArray< hfJointQuat > joints1( COUNT );
	hfTempArray< hfJointQuat > joints2( COUNT );
	const char *result;

	hfRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		hfAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].SetRotation( angles.ToMat3() );
		hfVec3 v;
		v[0] = srnd.CRandomFloat() * 10.0f;
		v[1] = srnd.CRandomFloat() * 10.0f;
		v[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].SetTranslation( v );
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointMatsToJointQuats( joints1.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointMatsToJointQuats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointMatsToJointQuats( joints2.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-4f ) ) {
			break;
		}
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointMatsToJointQuats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTransformJoints
============
*/
void TestTransformJoints() {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	hfTempArray< hfJointMat > joints( COUNT+1 );
	hfTempArray< hfJointMat > joints1( COUNT+1 );
	hfTempArray< hfJointMat > joints2( COUNT+1 );
	hfTempArray< int > parents( COUNT+1 );
	const char *result;

	hfRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		hfAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		hfVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->TransformJoints( joints1.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->TransformJoints( joints2.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 1; i <= COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-3f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestUntransformJoints
============
*/
void TestUntransformJoints() {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	hfTempArray< hfJointMat > joints( COUNT+1 );
	hfTempArray< hfJointMat > joints1( COUNT+1 );
	hfTempArray< hfJointMat > joints2( COUNT+1 );
	hfTempArray< int > parents( COUNT+1 );
	const char *result;

	hfRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		hfAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		hfVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->UntransformJoints( joints1.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->UntransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->UntransformJoints( joints2.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 1; i <= COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-3f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->UntransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMath
============
*/
void TestMath() {
	int i;
	TIME_TYPE start, end, bestClocks;

	hfLib::common->Printf("====================================\n" );

	float tst = -1.0f;
	float tst2 = 1.0f;
	float testvar = 1.0f;
	hfRandom rnd;

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = fabs( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "            fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		int tmp = * ( int * ) &tst;
		tmp &= 0x7FFFFFFF;
		tst = * ( float * ) &tmp;
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    hfMath::Fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = 10.0f + 100.0f * rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.01f;
		tst = 10.0f + 100.0f * rnd.RandomFloat();
	}
	PrintClocks( "            sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "    hfMath::Sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Sqrt16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  hfMath::Sqrt16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Sin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     hfMath::Sin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Sin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   hfMath::Sin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Cos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     hfMath::Cos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Cos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   hfMath::Cos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		hfMath::SinCos( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  hfMath::SinCos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		hfMath::SinCos16( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "hfMath::SinCos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Tan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     hfMath::Tan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Tan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   hfMath::Tan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::ASin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / hfMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    hfMath::ASin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::ASin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / hfMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  hfMath::ASin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::ACos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / hfMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    hfMath::ACos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::ACos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / hfMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  hfMath::ACos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::ATan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    hfMath::ATan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::ATan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  hfMath::ATan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Pow( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    hfMath::Pow( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Pow16( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  hfMath::Pow16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Exp( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    hfMath::Exp( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = hfMath::Exp16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  hfMath::Exp16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = hfMath::Log( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    hfMath::Log( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = hfMath::Log16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  hfMath::Log16( tst )", 1, bestClocks );

	hfLib::common->Printf( "testvar = %f\n", testvar );

	hfMat3 resultMat3;
	hfQuat fromQuat, toQuat, resultQuat;
	hfCQuat cq;
	hfAngles ang;

	fromQuat = hfAngles( 30, 45, 0 ).ToQuat();
	toQuat = hfAngles( 45, 0, 0 ).ToQuat();
	cq = hfAngles( 30, 45, 0 ).ToQuat().ToCQuat();
	ang = hfAngles( 30, 40, 50 );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = fromQuat.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "       hfQuat::ToMat3()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat.Slerp( fromQuat, toQuat, 0.3f );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "        hfQuat::Slerp()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = cq.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "      hfCQuat::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = ang.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     hfAngles::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = ang.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     hfAngles::ToMat3()", 1, bestClocks );
}

/*
============
hfSIMD::Test_f
============
*/
void hfSIMD::Test_f( const hfCmdArgs &args ) {

#if defined(_WIN32)
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
#endif

	p_simd = processor;
	p_generic = generic;

	if ( hfStr::Length( args.Argv( 1 ) ) != 0 ) {
		cpuid_t cpuid = hfLib::sys->GetProcessorId();
		hfStr argString = args.Args();

		argString.Replace( " ", "" );

		if ( hfStr::Icmp( argString, "SSE" ) == 0 ) {
#if defined(__x86_64__) || defined(__i386__)
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) ) {
				common->Printf( "CPU does not support MMX & SSE\n" );
				return;
			}
			p_simd = new (TAG_MATH) hfSIMD_SSE;
#else
			common->Printf( "SSE not supported on this platform\n" );
			return;
#endif
		} else {
			common->Printf( "invalid argument, use: MMX, 3DNow, SSE, SSE2, SSE3, AltiVec\n" );
			return;
		}
	}

	hfLib::common->SetRefreshOnPrint( true );

	hfLib::common->Printf( "using %s for SIMD processing\n", p_simd->GetName() );

	GetBaseClocks();

	TestMath();
	TestMinMax();
	TestMemcpy();
	TestMemset();

	hfLib::common->Printf("====================================\n" );

	TestBlendJoints();
	TestBlendJointsFast();
	TestConvertJointQuatsToJointMats();
	TestConvertJointMatsToJointQuats();
	TestTransformJoints();
	TestUntransformJoints();

	hfLib::common->Printf("====================================\n" );

	hfLib::common->SetRefreshOnPrint( false );

	if ( p_simd != processor ) {
		delete p_simd;
	}
	p_simd = NULL;
	p_generic = NULL;

#if defined(_WIN32)
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
#endif
}

