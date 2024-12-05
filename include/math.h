#pragma once

#include "assert.h"
#include "type.h"

static inline u32
Clamp(u32 value, u32 min, u32 max)
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

static inline u64
Absolute(s64 value)
{
  if (value < 0)
    return (u64)(-value);
  return (u64)value;
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
 * @note Adapted from https://github.com/jart/cosmopolitan/blob/master/libc/intrin/bsrl.c
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

extern f32
sqrtf(f32 value);

static inline f32
SquareRoot(f32 value)
{
  return sqrtf(value);
}

typedef struct {
  union {
    struct {
      f32 x;
      f32 y;
    };
    f32 e[2];
  };
} v2;

static inline v2
v2_add(v2 a, v2 b)
{
  return (v2){a.x + b.x, a.y + b.y};
}

static inline v2
v2_sub(v2 a, v2 b)
{
  return (v2){a.x - b.x, a.y - b.y};
}

static inline v2
v2_scale(v2 a, f32 scaler)
{
  return (v2){a.x * scaler, a.y * scaler};
}

static inline f32
v2_dot(v2 a, v2 b)
{
  return a.x * b.x + a.y * b.y;
}

static inline v2
v2_perp(v2 a)
{
  return (v2){-a.y, a.x};
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

static inline v2
v2_normalize(v2 a)
{
  f32 length = v2_length(a);
  if (length == 0)
    return (v2){0, 0};
  return v2_scale(a, length);
}

typedef struct {
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

typedef struct {
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
