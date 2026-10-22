#ifndef __PARALLELJOBLIST_JOBHEADERS_H__
#define __PARALLELJOBLIST_JOBHEADERS_H__

/*
================================================================================================

	Minimum set of headers needed to compile the code for a job.

================================================================================================
*/

#include "sys/hfsys_defines.h"

#include <stddef.h>					// for offsetof
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#if defined(_WIN32)
#include <basetsd.h>				// for UINT_PTR
#include <intrin.h>
#pragma warning( disable : 4100 )	// unreferenced formal parameter
#pragma warning( disable : 4127 )	// conditional expression is constant
#endif





#include "sys/hfsys_assert.h"
#include "sys/hfsys_types.h"
#include "sys/hfsys_intrinsics.h"
#include "math/hfMath.h"
#include "hfParallelJobList.h"

#if defined(_MSC_VER) && _MSC_VER >= 1600
#undef NULL
#define NULL 0
#endif

#endif // !__PARALLELJOBLIST_JOBHEADERS_H__

