#ifndef HFTYPES_H
#define HFTYPES_H 1

#include "hfConfig.h"

// NOTE: the canonical fixed width types (byte, word, dword, qword, uint8,
// int64, ...) are defined by sys/hfsys_types.h. This header only provides
// the engine specific aliases that layer does not cover, and is safe to
// include before or after it.
#include <stdint.h>
#include <stddef.h>

#ifndef HFINT_DEFINED
#define HFINT_DEFINED
typedef int32_t	hfInt;
#endif

#ifndef HFFLOAT_DEFINED
#define HFFLOAT_DEFINED
typedef float	hfFloat;
#endif

#ifndef HFDOUBLE_DEFINED
#define HFDOUBLE_DEFINED
typedef double	hfDouble;
#endif

#endif
