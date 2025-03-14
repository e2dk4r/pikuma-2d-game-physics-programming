#pragma once

typedef __INT8_TYPE__ s8;
typedef __INT16_TYPE__ s16;
typedef __INT32_TYPE__ s32;
typedef __INT64_TYPE__ s64;

typedef __UINT8_TYPE__ u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;
typedef u8 b8;

typedef float f32;
typedef double f64;

#define S8_MIN (-128)
#define S16_MIN (-32767 - 1)
#define S32_MIN (-2147483647 - 1)
#define S64_MIN (-9223372036854775807L - 1)

#define S8_MAX (127)
#define S16_MAX (32767)
#define S32_MAX (2147483647)
#define S64_MAX (9223372036854775807L)

#define U8_MAX (255)
#define U16_MAX (65535)
#define U32_MAX (4294967295U)
#define U64_MAX (18446744073709551615UL)

#define F32_MIN __FLT_MIN__
#define F32_MAX __FLT_MAX__
#define F32_LOWEST (-F32_MAX)
#define F32_EPSILON (1.192092896e-07F)
#define F64_MIN __DBL_MIN__
#define F64_MAX __DBL_MAX__
#define F64_LOWEST (-F64_MAX)
#define F64_EPSILON (1.1920928955078125e-07F) /* 0x0.000002p0 */

#define comptime const static
#define internalfn static
#define globalvar static
#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))
