#include "../../hflib/hfprecompiled.h"
#include "hflinux_local.h"

#include <fenv.h>
#include <unistd.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>

/*
================
Sys_GetCPUId
================
*/
cpuid_t Sys_GetCPUId() {
	cpuid_t id = CPUID_GENERIC;

	unsigned int eax, ebx, ecx, edx;
	if ( __get_cpuid( 1, &eax, &ebx, &ecx, &edx ) ) {
		id = CPUID_GENERIC;
		if ( edx & ( 1 << 23 ) ) {
			id = (cpuid_t)( id | CPUID_MMX );
		}
		if ( edx & ( 1 << 25 ) ) {
			id = (cpuid_t)( id | CPUID_SSE );
		}
		if ( edx & ( 1 << 26 ) ) {
			id = (cpuid_t)( id | CPUID_SSE2 );
		}
		if ( ecx & ( 1 << 0 ) ) {
			id = (cpuid_t)( id | CPUID_SSE3 );
		}
		// vendor
		unsigned int vi[4];
		__get_cpuid( 0, &vi[0], &vi[1], &vi[2], &vi[3] );
		if ( vi[1] == 'uneG' && vi[3] == 'Ieni' && vi[2] == 'letn' ) {
			id = (cpuid_t)( id | CPUID_INTEL );
		} else if ( vi[1] == 'htuA' && vi[3] == 'itne' && vi[2] == 'DMAc' ) {
			id = (cpuid_t)( id | CPUID_AMD );
		}
	}

	return id;
}

/*
================
Sys_GetProcessorId
================
*/
cpuid_t Sys_GetProcessorId() {
	return Sys_GetCPUId();
}

/*
================
Sys_GetProcessorString
================
*/
const char *Sys_GetProcessorString() {
	static char buf[128];
	unsigned int eax, ebx, ecx, edx;
	char vendor[13];
	__get_cpuid( 0, &eax, &ebx, &ecx, &edx );
	memcpy( vendor, &ebx, 4 );
	memcpy( vendor + 4, &edx, 4 );
	memcpy( vendor + 8, &ecx, 4 );
	vendor[12] = 0;
	hfStr::snPrintf( buf, sizeof( buf ), "%s", vendor );
	return buf;
}

/*
================
Sys_GetClockTicks
================
*/
double Sys_GetClockTicks() {
	unsigned int lo, hi;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return (double)( ( (uint64)hi << 32 ) | lo );
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond() {
	static double hz = 0;
	if ( hz == 0 ) {
		FILE * f = fopen( "/proc/cpuinfo", "r" );
		if ( f != NULL ) {
			char line[256];
			while ( fgets( line, sizeof( line ), f ) != NULL ) {
				if ( strncmp( line, "cpu MHz", 7 ) == 0 ) {
					double mhz = atof( strchr( line, ':' ) + 1 );
					hz = mhz * 1000000.0;
					break;
				}
			}
			fclose( f );
		}
		if ( hz == 0 ) {
			hz = 2.4e9;
		}
	}
	return hz;
}
#else
/*
================
Sys_GetCPUId
================
*/
cpuid_t Sys_GetCPUId() {
	return CPUID_GENERIC;
}

/*
================
Sys_GetProcessorId
================
*/
cpuid_t Sys_GetProcessorId() {
	return Sys_GetCPUId();
}

/*
================
Sys_GetProcessorString
================
*/
const char *Sys_GetProcessorString() {
	return "aarch64";
}

/*
================
Sys_GetClockTicks
================
*/
double Sys_GetClockTicks() {
	return (double)( (uint64)Sys_Microseconds() * 1000 );
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond() {
	return 1.0e9;
}
#endif

/*
================
Sys_FPU_StackIsEmpty
================
*/
bool Sys_FPU_StackIsEmpty() {
	return true;
}

/*
================
Sys_FPU_ClearStack
================
*/
void Sys_FPU_ClearStack() {
}

/*
================
Sys_FPU_GetState
================
*/
const char *Sys_FPU_GetState() {
	return "";
}

/*
================
Sys_FPU_EnableExceptions
================
*/
void Sys_FPU_EnableExceptions( int exceptions ) {
	feclearexcept( FE_ALL_EXCEPT );
}

/*
================
Sys_FPU_SetPrecision
================
*/
void Sys_FPU_SetPrecision( int precision ) {
	// x86-64 ignores x87 precision control; no-op for portability
}

/*
================
Sys_FPU_SetRounding
================
*/
void Sys_FPU_SetRounding( int rounding ) {
	int mode = FE_TONEAREST;
	switch ( rounding ) {
		case FPU_ROUNDING_DOWN:		mode = FE_DOWNWARD; break;
		case FPU_ROUNDING_UP:		mode = FE_UPWARD; break;
		case FPU_ROUNDING_TO_ZERO:	mode = FE_TOWARDZERO; break;
		default:					mode = FE_TONEAREST; break;
	}
	fesetround( mode );
}

/*
================
Sys_FPU_SetFTZ
================
*/
#if defined(__x86_64__) || defined(__i386__)
void Sys_FPU_SetFTZ( bool enable ) {
	if ( enable ) {
		unsigned int mxcsr;
		__asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr));
		mxcsr |= ( 1 << 15 );  // FTZ
		__asm__ __volatile__("ldmxcsr %0" : : "m"(mxcsr));
	}
}

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable ) {
	if ( enable ) {
		unsigned int mxcsr;
		__asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr));
		mxcsr |= ( 1 << 6 );  // DAZ
		__asm__ __volatile__("ldmxcsr %0" : : "m"(mxcsr));
	}
}
#else
void Sys_FPU_SetFTZ( bool enable ) {
	// ARM FPCR flush-to-zero not needed for engine correctness; no-op for portability
}

void Sys_FPU_SetDAZ( bool enable ) {
	// ARM has no DAZ emulation; no-op for portability
}
#endif

/*
================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
================
*/
void Sys_CPUCount( int & numLogicalCPUCores, int & numPhysicalCPUCores, int & numCPUPackages ) {
	numLogicalCPUCores = (int)sysconf( _SC_NPROCESSORS_ONLN );
	numPhysicalCPUCores = (int)sysconf( _SC_NPROCESSORS_CONF );
	numCPUPackages = 1;
}

