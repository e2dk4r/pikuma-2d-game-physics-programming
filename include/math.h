#pragma once

#include "assert.h"
#include "type.h"

comptime f32 PI = 3.14159265358979323846264338327950288f;

// used for calculating digit count of value
//   digitCount = 1
//   while (digitCount < ARRAY_COUNT(array) && value >= array[digitCount])
//     digitCount++
comptime u64 POWERS_OF_10[] = {
    1ull,                    // 10^0
    10ull,                   // 10^1
    100ull,                  // 10^2
    1000ull,                 // 10^3
    10000ull,                // 10^4
    100000ull,               // 10^5
    1000000ull,              // 10^6
    10000000ull,             // 10^7
    100000000ull,            // 10^8
    1000000000ull,           // 10^9
    10000000000ull,          // 10^10
    100000000000ull,         // 10^11
    1000000000000ull,        // 10^12
    10000000000000ull,       // 10^13
    100000000000000ull,      // 10^14
    1000000000000000ull,     // 10^15
    10000000000000000ull,    // 10^16
    100000000000000000ull,   // 10^17
    1000000000000000000ull,  // 10^18
    10000000000000000000ull, // 10^19
};

static inline u32
ClampU32(u32 value, u32 min, u32 max)
{
  debug_assert(min < max);
  if (value < min)
    return min;
  else if (value > max)
    return max;
  return value;
}

static inline f32
Clamp(f32 value, f32 min, f32 max)
{
  debug_assert(min < max);
  if (value < min)
    return min;
  else if (value > max)
    return max;
  return value;
}

static inline b8
IsPowerOfTwo(u64 value)
{
  return (value & (value - 1)) == 0;
}

#define Maximum(x, y) (x > y ? x : y)
#define Minimum(x, y) (x < y ? x : y)

static inline f32
Absolute(f32 value)
{
  if (value < 0)
    return -value;
  return value;
}

static inline f32
Square(f32 value)
{
  return value * value;
}

static inline f32
Inverse(f32 value)
{
  debug_assert(value != 0.0f);
  return 1.0f / value;
}

/* linear blend
 *    .       .
 *    A       B
 *
 * from A to B delta is
 *    t = B - A
 *
 * for going from A to B is
 *    C = A + (B - A)
 *
 * this can be formulated where t is [0, 1]
 *    C(t) = A + t (B - A)
 *    C(t) = A + t B - t A
 *    C(t) = A (1 - t) + t B
 */
static inline f32
Lerp(f32 a, f32 b, f32 t)
{
  f32 result = (1.0f - t) * a + t * b;
  return result;
}

/*
 * Returns binary logarithm of 𝑥.
 *
 *                           ctz(𝑥)         31^clz(𝑥)   clz(𝑥)
 *       uint32 𝑥   bsf(𝑥) tzcnt(𝑥)   ffs(𝑥)   bsr(𝑥) lzcnt(𝑥)
 *     0x00000000      wut       32        0      wut       32
 *     0x00000001        0        0        1        0       31
 *     0x80000001        0        0        1       31        0
 *     0x80000000       31       31       32       31        0
 *     0x00000010        4        4        5        4       27
 *     0x08000010        4        4        5       27        4
 *     0x08000000       27       27       28       27        4
 *     0xffffffff        0        0        1       31        0
 *
 * @param x is a 64-bit integer
 * @return number in range 0..63 or undefined if 𝑥 is 0
 *
 * @note Adapted from
 * https://github.com/jart/cosmopolitan/blob/master/libc/intrin/bsrl.c
 * @copyright
 * ╒══════════════════════════════════════════════════════════════════════════════╕
 * │ Copyright 2023 Justine Alexandra Roberts Tunney                              │
 * │                                                                              │
 * │ Permission to use, copy, modify, and/or distribute this software for         │
 * │ any purpose with or without fee is hereby granted, provided that the         │
 * │ above copyright notice and this permission notice appear in all copies.      │
 * │                                                                              │
 * │ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
 * │ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
 * │ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
 * │ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
 * │ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
 * │ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
 * │ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
 * │ PERFORMANCE OF THIS SOFTWARE.                                                │
 * └──────────────────────────────────────────────────────────────────────────────┘
 */
static inline u8
bsrl(u64 x)
{
  static const u8 kDebruijn[64] = {
      0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61, 54, 58, 35, 52, 50, 42,
      21, 44, 38, 32, 29, 23, 17, 11, 4,  62, 46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43,
      31, 22, 10, 45, 25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63,
  };

  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return kDebruijn[(x * 0x03f79d71b4cb0a89ull) >> 58];
}

static inline f32
SquareRoot(f32 value)
{
  return __builtin_sqrtf(value);
}

static inline f32
SignOf(f32 value)
{
  return value == 0.0f ? 0.0f : value > 0.0f ? 1.0f : -1.0f;
}

static inline f32
Cos(f32 value)
{
  return __builtin_cosf(value);
}

static inline f32
Sin(f32 value)
{
  return __builtin_sinf(value);
}

typedef struct v2 {
  union {
    struct {
      f32 x;
      f32 y;
    };
    f32 e[2];
  };
} v2;

static inline v2
V2(f32 x, f32 y)
{
  return (v2){.x = x, .y = y};
}

static inline void
v2_add_ref(v2 *a, v2 b)
{
  a->x += b.x;
  a->y += b.y;
}

static inline v2
v2_add(v2 a, v2 b)
{
  v2_add_ref(&a, b);
  return a;
}

static inline v2
v2_add_multiple(u32 vertexCount, v2 *verticies)
{
  v2 sum;

  sum = V2(0.0f, 0.0f);
  for (u32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
    v2_add_ref(&sum, verticies[vertexIndex]);
  }

  return sum;
}

static inline void
v2_sub_ref(v2 *a, v2 b)
{
  a->x -= b.x;
  a->y -= b.y;
}

static inline v2
v2_sub(v2 a, v2 b)
{
  v2_sub_ref(&a, b);
  return a;
}

static inline void
v2_scale_ref(v2 *a, f32 scaler)
{
  a->x *= scaler;
  a->y *= scaler;
}

static inline v2
v2_scale(v2 a, f32 scaler)
{
  v2_scale_ref(&a, scaler);
  return a;
}

static inline f32
v2_dot(v2 a, v2 b)
{
  return a.x * b.x + a.y * b.y;
}

static inline void
v2_hadamard_ref(v2 *a, v2 b)
{
  a->x *= b.x;
  a->y *= b.y;
}

static inline v2
v2_hadamard(v2 a, v2 b)
{
  v2_hadamard_ref(&a, b);
  return a;
}

static inline v2
v2_perp(v2 a)
{
  return (v2){.x = -a.y, .y = a.x};
}

static inline f32
v2_length_square(v2 a)
{
  return v2_dot(a, a);
}

static inline f32
v2_length(v2 a)
{
  return SquareRoot(v2_length_square(a));
}

static inline void
v2_normalize_ref(v2 *a)
{
  f32 length;

  if (v2_length_square(*a) == 0.0f) {
    a->x = 0.0f;
    a->y = 0.0f;
    return;
  }

  length = v2_length(*a);
  a->x /= length;
  a->y /= length;
}

static inline v2
v2_normalize(v2 a)
{
  v2_normalize_ref(&a);
  return a;
}

static inline void
v2_neg_ref(v2 *a)
{
  v2_scale_ref(a, -1.0f);
}

static inline v2
v2_neg(v2 a)
{
  v2_neg_ref(&a);
  return a;
}

typedef struct v3 {
  union {
    struct {
      f32 x;
      f32 y;
      f32 z;
    };
    struct {
      v2 xy;
      f32 _unused0;
    };
    struct {
      f32 _unused1;
      v2 yz;
    };
    f32 e[3];
  };
} v3;

static inline v3
V3(f32 x, f32 y, f32 z)
{
  return (v3){.x = x, .y = y, .z = z};
}

static inline void
v3_add_ref(v3 *a, v3 b)
{
  a->x += b.x;
  a->y += b.y;
  a->z += b.z;
}

static inline v3
v3_add(v3 a, v3 b)
{
  v3_add_ref(&a, b);
  return a;
}

static inline v3
v3_add_multiple(u32 vertexCount, v3 *verticies)
{
  v3 sum;

  sum = V3(0.0f, 0.0f, 0.0f);
  for (u32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
    v3_add_ref(&sum, verticies[vertexIndex]);
  }

  return sum;
}

static inline void
v3_sub_ref(v3 *a, v3 b)
{
  a->x -= b.x;
  a->y -= b.y;
  a->z -= b.z;
}

static inline v3
v3_sub(v3 a, v3 b)
{
  v3_sub_ref(&a, b);
  return a;
}

static inline void
v3_scale_ref(v3 *a, f32 scaler)
{
  a->x *= scaler;
  a->y *= scaler;
  a->z *= scaler;
}

static inline v3
v3_scale(v3 a, f32 scaler)
{
  v3_scale_ref(&a, scaler);
  return a;
}

static inline f32
v3_dot(v3 a, v3 b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline void
v3_hadamard_ref(v3 *a, v3 b)
{
  a->x *= b.x;
  a->y *= b.y;
  a->z *= b.z;
}

static inline v3
v3_hadamard(v3 a, v3 b)
{
  v3_hadamard_ref(&a, b);
  return a;
}

static inline f32
v3_length_square(v3 a)
{
  return v3_dot(a, a);
}

static inline f32
v3_length(v3 a)
{
  return SquareRoot(v3_length_square(a));
}

static inline void
v3_normalize_ref(v3 *a)
{
  f32 length;

  if (v3_length_square(*a) == 0.0f) {
    a->x = 0.0f;
    a->y = 0.0f;
    a->z = 0.0f;
    return;
  }

  length = v3_length(*a);
  a->x /= length;
  a->y /= length;
  a->z /= length;
}

static inline v3
v3_normalize(v3 a)
{
  v3_normalize_ref(&a);
  return a;
}

static inline void
v3_neg_ref(v3 *a)
{
  v3_scale_ref(a, -1.0f);
}

static inline v3
v3_neg(v3 a)
{
  v3_neg_ref(&a);
  return a;
}

static inline void
v3_cross_ref(v3 *a, v3 b)
{
  /* see:
   * - https://www.youtube.com/watch?v=eu6i7WJeinw "Cross products | Chapter 10,
   * Essence of linear algebra"
   */
  v3 crossProduct = {
      .x = a->y * b.z - a->z * b.y,
      .y = a->z * b.x - a->x * b.z,
      .z = a->x * b.y - a->y * b.x,
  };
  *a = crossProduct;
}

static inline v3
v3_cross(v3 a, v3 b)
{
  v3_cross_ref(&a, b);
  return a;
}

static inline f32
v3_absolute_norm(v3 a)
{
  return Absolute(a.x) + Absolute(a.y) + Absolute(a.z);
}

typedef struct v4 {
  union {
    struct {
      f32 x;
      f32 y;
      f32 z;
      f32 w;
    };
    struct {
      v3 xyz;
      f32 _unused0;
    };

    struct {
      f32 r;
      f32 g;
      f32 b;
      f32 a;
    };
    struct {
      v3 rgb;
      f32 _unused1;
    };
    f32 e[4];
  };
} v4;

typedef struct rect {
  struct v2 min; // left bottom
  struct v2 max; // right top
} rect;

static inline struct rect
RectCenterHalfDim(v2 center, v2 halfDim)
{
  return (struct rect){
      .min = v2_sub(center, halfDim),
      .max = v2_add(center, halfDim),
  };
}

static inline struct rect
RectCenterDim(v2 center, v2 dim)
{
  v2 halfDim = v2_scale(dim, 0.5f);
  return RectCenterHalfDim(center, halfDim);
}

static inline struct v2
RectGetDim(struct rect rect)
{
  return v2_sub(rect.max, rect.min);
}

static inline struct v2
RectGetHalfDim(struct rect rect)
{
  return v2_scale(RectGetDim(rect), 0.5f);
}

static inline b8
IsPointInsideRect(struct v2 point, struct rect rect)
{
  debug_assert(rect.min.x < rect.max.x && rect.min.y < rect.max.y && "invalid rect");
  return
      // x axis
      point.x >= rect.min.x &&
      point.x < rect.max.x
      // y axis
      && point.y >= rect.min.y && point.y < rect.max.y
      //
      ;
}

static inline b8
IsAABBOverlapping(struct rect a, struct rect b)
{
  debug_assert(a.min.x < a.max.x && a.min.y < a.max.y && "invalid rect");
  debug_assert(b.min.x < b.max.x && b.min.y < b.max.y && "invalid rect");

  // see:
  // https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#aabb_vs._aabb
  return
      // --AL--BL--AR--BR--
      // x axis, a's left less than b's right and b's right less than a's left
      (a.min.x <= b.max.x && b.min.x < a.max.x)
      // y axis, a's bottom less than b's top and b's bottom less than a's top
      && (a.min.y <= b.max.y && b.min.y < a.max.y);
}
