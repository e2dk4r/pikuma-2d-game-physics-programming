#pragma once

#include "text.h"
#include "type.h"

#if IS_PLATFORM_WINDOWS
#include <intrin.h>
#endif

/*
 * Implemented by Cassio Neri
 * Modifications:
 *   - Add sign to teju32_fields_t
 *   - Prefix some functions with "teju_"
 *   - Make it single header file
 * see: https://github.com/cassioneri/teju_jagua
 * license:
 * https://github.com/cassioneri/teju_jagua/blob/8b284dccbb5f2d63fb1702220b9339b64c5652e9/LICENSES/APACHE-2.0.txt
 */

/**
 * @file teju/config.h
 *
 * Platform configurations, notably, multiplication capabilities.
 */
#if defined(teju_has_uint128)
typedef __uint128_t uint128_t;
#else
// Cannot support float128_t if uint128_t is not defined.
#undef teju_has_float128
#endif

#if defined(teju_has_float128)
typedef __float128 float128_t;
#endif

//------------------------------------------------------------------------------
// Flags indicating the platform's multiplication capability.
//------------------------------------------------------------------------------

// The size in bits of the limb is teju_size. For instance, if teju_size == 32,
// then 1-, 2- and 4-limb integers have size 32, 64 and 128, respectively.
// Similarly, if teju_size == 64, then 1-, 2- and 4-limb integers have size 64,
// 128 and 256 respectively. The platform is required to support 1-limb integers
// but not necessarily 2- and 4-limb integers.

// Macros teju_calculation_div10 and teju_calculation_mshift define the
// algorithm used in teju_div10 and teju_mshift, respectively. They are set to
// one of the values below depending on the platform's capability as explained
// in their documentation.

/**
 * @brief The platform provides operator * for 1-limb unsigned integers that
 * yields the lower 1-limb of the 2-limb product. For instance, assuming 1-limb
 * is 32-bits:
 * \code{.cpp}
 *     uint32_t a, b;
 *     uint32_t lower = a * b;
 * \endcode
 */
#define teju_built_in_1 0

/**
 * @brief The platform implements teju_multiply() which takes two 1-limb
 * unsigned integers and returns the lower 1-limb of the 2-limb product. It also
 * takes a third argument of pointer type where the upper 1-limb of the product
 * is stored on exit. For instance, assuming 1-limb is 32-bits:
 * \code{.cpp}
 *     uint32_t a, b, upper;
 *     uint32_t lower = teju_multiply(a, b, &upper);
 * \endcode
 */
#define teju_synthetic_1 1

/**
 * @brief The platform provides operator * for 2-limb unsigned integers that
 * yields the lower 2-limb of the 4-limb product. For instance, assuming 1-limb
 * is 32-bits:
 * \code{.cpp}
 *     uint64_t a, b;
 *     uint64_t lower = a * b;
 * \endcode
 */
#define teju_built_in_2 2

/**
 * @brief The platform implements teju_multiply() which takes two 2-limb
 * unsigned integers and returns the lower 2-limb of the 4-limb product. It also
 * takes a third argument of pointer type where the upper 2-limb of the product
 * is stored on exit. For instance, assuming 1-limb is 32-bits:
 * \code{.cpp}
 *     uint64_t a, b, upper;
 *     uint64_t lower = teju_multiply(a, b, &upper);
 * \endcode
 */
#define teju_synthetic_2 3

/**
 * @brief The platform provides operator * for 4-limb unsigned integers that
 * yields the lower 4-limb of the 8-limb product. For instance, assuming 1-limb
 * is 32-bits, gcc and clang support the following:
 * \code{.cpp}
 *     uint128_t a, b;
 *     uint128_t lower = a * b;
 * \endcode
 */
#define teju_built_in_4 4

//------------------------------------------------------------------------------
// Limbs
//------------------------------------------------------------------------------

//----------//
//  32 bits //
//----------//

#define teju32_u1_t u32
#define teju32_u2_t u64

typedef struct {
  teju32_u1_t mantissa;
  s32 exponent;
} teju32_fields_t;

//------------------------------------------------------------------------------
// teju_multiply
//------------------------------------------------------------------------------

// One might want to disabled the provided implementations of teju_multiply to
// be able to implement their own (e.g., for testing). For this, it suffices to
// define macro teju_do_not_define_teju_multiply prior to including this file.

#if defined(_MSC_VER) && !defined(teju_do_not_define_teju_multiply)

/**
 * @brief Calculates the 128-bits product of two 64-bits unsigned numbers.
 *
 * @param a The 1st multiplicand.
 * @param b The 2nd multiplicand.
 * @param upper On exit the value of the highest 64-bits of the product.
 *
 * @returns The lower 64-bits value of the product.
 */
inline static u64
teju_multiply(u64 const a, u64 const b, u64 *upper)
{
  return _umul128(a, b, upper);
}

#endif // defined(_MSC_VER)

/**
 * @file teju/common.h
 * @brief Gets the k least significant bits of n (i.e., e % 2^k.)
 *
 * @pre k < sizeof(n) * CHAR_BIT.
 *
 * @param n                 The value of n.
 * @param k                 The value of k.
 *
 * @returns The k least significant bits of n.
 */
#define teju_lsb(n, k) ((n) & (((0 * (n) + 1) << (k)) - 1))

/**
 * @brief Returns 2^e as a given type.
 *
 * @pre e < sizeof(type) * CHAR_BIT.
 *
 * @param type              The type.
 * @param e                 The exponent e.
 *
 * @returns 2^e.
 */
#define teju_pow2(type, e) (((type)1) << (e))
// Argument bounds of teju_log10_pow2.
#define teju_log10_pow2_min -112815
#define teju_log10_pow2_max 112815

/**
 * @brief Returns the largest exponent f such that 10^f <= 2^e.
 *
 * @pre teju_log10_pow2_min <= e && e <= teju_log10_pow2_max.
 *
 * @param e                 The exponent e.
 *
 * @returns The exponent f.
 */
static inline s32
teju_log10_pow2(s32 const e)
{
  return (s32)((((s64)1292913987) * e) >> 32);
}

/**
 * @brief Returns the residual r = e - e_0, where e_0 is the smallest exponent
 * such that the integer part of log_10(2^e_0) matches that of log_10(2^e).
 *
 * @pre teju_log10_pow2_min <= e && e <= teju_log10_pow2_max.
 *
 * @param e                 The exponent e.
 *
 * @returns The residual r.
 */
static inline u32
teju_log10_pow2_residual(s32 const e)
{
  return ((u32)(((s64)1292913987) * e)) / 1292913987;
}

/**
 * @file teju/ieee754.h
 *
 * Constants defined by the IEEE-754 standard.
 *
 * https://en.wikipedia.org/wiki/IEEE_754#Basic_and_interchange_formats
 */
enum {
  teju_ieee754_binary16_exponent_size = 5,
  teju_ieee754_binary16_mantissa_size = 10,
  teju_ieee754_binary16_exponent_min = -14,
  teju_ieee754_binary16_exponent_max = 15,

  teju_ieee754_binary32_exponent_size = 8,
  teju_ieee754_binary32_mantissa_size = 23,
  teju_ieee754_binary32_exponent_min = -126,
  teju_ieee754_binary32_exponent_max = 127,

  teju_ieee754_binary64_exponent_size = 11,
  teju_ieee754_binary64_mantissa_size = 52,
  teju_ieee754_binary64_exponent_min = -1022,
  teju_ieee754_binary64_exponent_max = 1023,

  teju_ieee754_binary128_exponent_size = 15,
  teju_ieee754_binary128_mantissa_size = 112,
  teju_ieee754_binary128_exponent_min = -16382,
  teju_ieee754_binary128_exponent_max = 16383,

  teju_ieee754_binary256_exponent_size = 19,
  teju_ieee754_binary256_mantissa_size = 236,
  teju_ieee754_binary256_exponent_min = -262142,
  teju_ieee754_binary256_exponent_max = 262143,
};

/**
 * @file teju/generated/ieee32_no_uint128.c
 */

#define teju_size 32
#define teju_exponent_minimum -149
#define teju_mantissa_size 23
#define teju_storage_index_offset -45
// #define teju_calculation_div10 teju_built_in_2
// #define teju_calculation_mshift teju_built_in_2
#define teju_calculation_shift 64

#define teju_function teju_ieee32_no_uint128
#define teju_fields_t teju32_fields_t
#define teju_u1_t teju32_u1_t

#if defined(teju32_u2_t)
#define teju_u2_t teju32_u2_t
#endif

#if defined(teju32_u4_t)
#define teju_u4_t teju32_u4_t
#endif

static struct {
  teju_u1_t const upper;
  teju_u1_t const lower;
} const teju_multipliers[] = {
    {0xb35dbf82, 0x1ae4f38c}, // -45
    {0x8f7e32ce, 0x7bea5c70}, // -44
    {0xe596b7b0, 0xc643c71a}, // -43
    {0xb7abc627, 0x050305ae}, // -42
    {0x92efd1b8, 0xd0cf37bf}, // -41
    {0xeb194f8e, 0x1ae525fe}, // -40
    {0xbc143fa4, 0xe250eb32}, // -39
    {0x96769950, 0xb50d88f5}, // -38
    {0xf0bdc21a, 0xbb48db21}, // -37
    {0xc097ce7b, 0xc90715b4}, // -36
    {0x9a130b96, 0x3a6c115d}, // -35
    {0xf684df56, 0xc3e01bc7}, // -34
    {0xc5371912, 0x364ce306}, // -33
    {0x9dc5ada8, 0x2b70b59e}, // -32
    {0xfc6f7c40, 0x45812297}, // -31
    {0xc9f2c9cd, 0x04674edf}, // -30
    {0xa18f07d7, 0x36b90be6}, // -29
    {0x813f3978, 0xf8940985}, // -28
    {0xcecb8f27, 0xf4200f3b}, // -27
    {0xa56fa5b9, 0x9019a5c9}, // -26
    {0x84595161, 0x401484a1}, // -25
    {0xd3c21bce, 0xcceda101}, // -24
    {0xa968163f, 0x0a57b401}, // -23
    {0x87867832, 0x6eac9001}, // -22
    {0xd8d726b7, 0x177a8001}, // -21
    {0xad78ebc5, 0xac620001}, // -20
    {0x8ac72304, 0x89e80001}, // -19
    {0xde0b6b3a, 0x76400001}, // -18
    {0xb1a2bc2e, 0xc5000001}, // -17
    {0x8e1bc9bf, 0x04000001}, // -16
    {0xe35fa931, 0xa0000001}, // -15
    {0xb5e620f4, 0x80000001}, // -14
    {0x9184e72a, 0x00000001}, // -13
    {0xe8d4a510, 0x00000001}, // -12
    {0xba43b740, 0x00000001}, // -11
    {0x9502f900, 0x00000001}, // -10
    {0xee6b2800, 0x00000001}, // -9
    {0xbebc2000, 0x00000001}, // -8
    {0x98968000, 0x00000001}, // -7
    {0xf4240000, 0x00000001}, // -6
    {0xc3500000, 0x00000001}, // -5
    {0x9c400000, 0x00000001}, // -4
    {0xfa000000, 0x00000001}, // -3
    {0xc8000000, 0x00000001}, // -2
    {0xa0000000, 0x00000001}, // -1
    {0x80000000, 0x00000001}, // 0
    {0xcccccccc, 0xcccccccd}, // 1
    {0xa3d70a3d, 0x70a3d70b}, // 2
    {0x83126e97, 0x8d4fdf3c}, // 3
    {0xd1b71758, 0xe219652c}, // 4
    {0xa7c5ac47, 0x1b478424}, // 5
    {0x8637bd05, 0xaf6c69b6}, // 6
    {0xd6bf94d5, 0xe57a42bd}, // 7
    {0xabcc7711, 0x8461cefd}, // 8
    {0x89705f41, 0x36b4a598}, // 9
    {0xdbe6fece, 0xbdedd5bf}, // 10
    {0xafebff0b, 0xcb24aaff}, // 11
    {0x8cbccc09, 0x6f5088cc}, // 12
    {0xe12e1342, 0x4bb40e14}, // 13
    {0xb424dc35, 0x095cd810}, // 14
    {0x901d7cf7, 0x3ab0acda}, // 15
    {0xe69594be, 0xc44de15c}, // 16
    {0xb877aa32, 0x36a4b44a}, // 17
    {0x9392ee8e, 0x921d5d08}, // 18
    {0xec1e4a7d, 0xb69561a6}, // 19
    {0xbce50864, 0x92111aeb}, // 20
    {0x971da050, 0x74da7bef}, // 21
    {0xf1c90080, 0xbaf72cb2}, // 22
    {0xc16d9a00, 0x95928a28}, // 23
    {0x9abe14cd, 0x44753b53}, // 24
    {0xf79687ae, 0xd3eec552}, // 25
    {0xc6120625, 0x76589ddb}, // 26
    {0x9e74d1b7, 0x91e07e49}, // 27
    {0xfd87b5f2, 0x8300ca0e}, // 28
    {0xcad2f7f5, 0x359a3b3f}, // 29
    {0xa2425ff7, 0x5e14fc32}, // 30
    {0x81ceb32c, 0x4b43fcf5}, // 31
};

static struct {
  teju_u1_t const multiplier;
  teju_u1_t const bound;
} const teju_minverse[] = {
    {0x00000001, 0xffffffff}, {0xcccccccd, 0x33333333}, {0xc28f5c29, 0x0a3d70a3}, {0x26e978d5, 0x020c49ba},
    {0x3afb7e91, 0x0068db8b}, {0x0bcbe61d, 0x0014f8b5}, {0x68c26139, 0x000431bd}, {0xae8d46a5, 0x0000d6bf},
    {0x22e90e21, 0x00002af3}, {0x3a2e9c6d, 0x00000897}, {0x3ed61f49, 0x000001b7}, {0x0c913975, 0x00000057},
    {0xcf503eb1, 0x00000011}, {0xf6433fbd, 0x00000003},
};

/**
 * @file teju/mshift.h
 *
 * Multiply-and-shift operations.
 */

/**
 * @brief Returns x + y sets a carry flag if the addition has wrapped up.
 *
 * @param x                 The value of x.
 * @param y                 The value of y.
 * @param c                 The addess of the carry flag to be set.
 *
 * @returns The sum x + y.
 */
static inline teju_u1_t
teju_add_and_carry(teju_u1_t x, teju_u1_t y, teju_u1_t *c)
{

#if defined(_MSC_VER) && !defined(__clang__)

#if teju_size == 16
  *c = _addcarry_u16(0, x, y, &x);
#elif teju_size == 32
  *c = _addcarry_u32(0, x, y, &x);
#elif teju_size == 64
  *c = _addcarry_u64(0, x, y, &x);
#else
#error "Size not supported by msvc."
#endif

#else

  x += y;
  *c = x < y;

#endif

  return x;
}

/**
 * @brief Returns the quotient q = ((u * 2^N + l) * m) / 2^s, where
 * N = teju_size and s = teju_calculation_shift.
 *
 * @param m                 The 1st multiplicand m.
 * @param u                 The 2nd multiplicand upper half u.
 * @param l                 The 2nd multiplicand lower half l.
 *
 * @returns The quotient q.
 */
static inline teju_u1_t
teju_mshift(teju_u1_t const m, teju_u1_t const u, teju_u1_t const l)
{

  // Let x := 2^N.

  //// #elif teju_calculation_mshift == teju_built_in_2

  // (u * x + l) * m = s1 * x + s0,
  //                       with s1 := u * m, s0 := l * m in [0, x^2[,
  //                 = s1 * x + (s01 * x + s00)
  //                       with s01 := s0 / x, s00 := s0 % x in [0, x[,
  //                 = (s1 + s01) * x + s00.

  teju_u2_t const s0 = ((teju_u2_t)l) * m;
  teju_u2_t const s1 = ((teju_u2_t)u) * m;
  return (u32)((s1 + (s0 >> teju_size)) >> (teju_calculation_shift - teju_size));
}

/**
 * @brief Returns the quotient q = ((u * 2^N + l) * 2^k) / 2^s, where
 * N = teju_size_size and s = teju_calculation_shift.
 *
 * @param k                 The exponent k.
 * @param u                 The upper part of the multiplicand u.
 * @param l                 The lower part of the multiplicand l.
 *
 * @returns The value of q.
 */
static inline teju_u1_t
teju_mshift_pow2(u32 const k, teju_u1_t const u, teju_u1_t const l)
{
  s32 const s = (s32)(k - (teju_calculation_shift - teju_size));
  if (s <= 0)
    return u >> -s;
  return (u << s) | (l >> (teju_size - s));
}

/**
 * @file teju/div10.h
 *
 * Different algorithms for division by 10.
 */

/**
 * @brief Gets the quotient of the division by 10.
 *
 * @param m The dividend.
 *
 * @returns m / 10.
 */
static inline teju_u1_t
teju_div10(teju_u1_t const m)
{

  // TODO (CN): Assert that m is in the range of validity.

  //// #elif teju_calculation_div10 == teju_built_in_2

  teju_u1_t const inv10 = ((teju_u1_t)-1) / 10 + 1;
  return (teju_u1_t)((((teju_u2_t)inv10) * m) >> teju_size);
}

/**
 * @file teju/teju.h
 *
 * The implementation of Teju Jagua and some of its helpers.
 */

/**
 * @brief Checks whether mantissa m is multiple of 2^e.
 *
 * @pre 0 <= e && e <= teju_mantissa_size.
 *
 * @param m                 The mantissa m.
 * @param e                 The exponent e.
 */
static inline b8
teju_is_multiple_of_pow2(teju_u1_t const m, s32 const e)
{
  return ((m >> e) << e) == m;
}

/**
 * @brief Checks whether the number m * 2^e is a small integer.
 *
 * @param m                 The mantissa m.
 * @param e                 The exponent e.
 */
static inline b8
teju_is_small_integer(teju_u1_t const m, s32 const e)
{
  return (-teju_mantissa_size <= e && e <= 0) && teju_is_multiple_of_pow2(m, -e);
}

/**
 * @brief Checks whether mantissa m is multiple of 5^f.
 *
 * @pre minverse[f] is well defined.
 *
 * @param m                 The mantissa m.
 * @param f                 The exponent f.
 */
static inline b8
teju_is_multiple_of_pow5(teju_u1_t const m, s32 const f)
{
  return m * teju_minverse[f].multiplier <= teju_minverse[f].bound;
}

/**
 * @brief Checks whether m, for m in { m_a, m_b, c_2 }, yields a tie.
 *
 * @param m                 The number m.
 * @param f                 The exponent f (for m == m_a and m == m_b) or
 *                          its negation -f for (m == c_2).
 */
static inline b8
teju_is_tie(teju_u1_t const m, s32 const f)
{
  return 0 <= f && f < (s32)(sizeof(teju_minverse) / sizeof(teju_minverse[0])) && teju_is_multiple_of_pow5(m, f);
}

/**
 * @brief Checks whether the mantissa of an uncentred value (whose decimal
 * exponent is f) yields a tie.
 *
 * @param f                 The exponent f.
 */
static inline b8
teju_is_tie_uncentred(s32 const f)
{
  return f > 0 && teju_mantissa_size % 4 == 2;
}

/**
 * @brief Creates teju_fields_t from exponent and mantissa.
 *
 * @param m                 The mantissa.
 * @param e                 The exponent.
 */
static inline teju_fields_t
teju_make_fields(teju_u1_t const m, s32 const e)
{
  teju_fields_t const fields = {m, e};
  return fields;
}

/**
 * @brief Rotates bits of a given number 1 position to the right.
 *
 * @param m                 The number.
 */
static inline teju_u1_t
teju_ror(teju_u1_t m)
{
  return m << (teju_size - 1) | m >> 1;
}

/**
 * @brief Shortens the decimal representation m dot 10^e by removing trailing
 * zeros of m and increasing e.
 *
 * @param m                 The mantissa m.
 * @param e                 The exponent e.
 *
 * @returns The fields of the shortest close decimal representation.
 */
static inline teju_fields_t
teju_remove_trailing_zeros(teju_u1_t m, s32 e)
{
  teju_u1_t const multiplier = teju_minverse[1].multiplier;
  teju_u1_t const bound = teju_minverse[1].bound / 2;
  while (1) {
    teju_u1_t const q = teju_ror(m * multiplier);
    if (q >= bound)
      return teju_make_fields(m, e);
    ++e;
    m = q;
  }
}

/**
 * @brief Teju Jagua itself.
 *
 * Finds the shortest decimal representation of m * 2^e.
 *
 * @param e                 The exponent e.
 * @param m                 The mantissa m.
 *
 * @returns The fields of the shortest decimal representation.
 */
static inline teju_fields_t
teju_function(teju_fields_t const binary)
{
  s32 const e = binary.exponent;
  teju_u1_t const m = binary.mantissa;

  if (teju_is_small_integer(m, e))
    return teju_remove_trailing_zeros(m >> -e, 0);

  teju_u1_t const m_0 = teju_pow2(teju_u1_t, teju_mantissa_size);
  s32 const f = teju_log10_pow2(e);
  u32 const r = teju_log10_pow2_residual(e);
  u32 const i = (u32)(f - teju_storage_index_offset);
  teju_u1_t const u = teju_multipliers[i].upper;
  teju_u1_t const l = teju_multipliers[i].lower;

  if (m != m_0 || e == teju_exponent_minimum) {

    teju_u1_t const m_a = (2 * m - 1) << r;
    teju_u1_t const a = teju_mshift(m_a, u, l);
    teju_u1_t const m_b = (2 * m + 1) << r;
    teju_u1_t const b = teju_mshift(m_b, u, l);
    teju_u1_t const q = teju_div10(b);
    teju_u1_t const s = 10 * q;

    if (s >= a) {
      if (s == b) {
        if (m % 2 == 0 || !teju_is_tie(m_b, f))
          return teju_remove_trailing_zeros(q, f + 1);
      } else if (s > a || (m % 2 == 0 && teju_is_tie(m_a, f)))
        return teju_remove_trailing_zeros(q, f + 1);
    }

    if ((a + b) % 2 == 1)
      return teju_make_fields((a + b) / 2 + 1, f);

    teju_u1_t const m_c = (2 * 2 * m) << r;
    teju_u1_t const c_2 = teju_mshift(m_c, u, l);
    teju_u1_t const c = c_2 / 2;

    if (c_2 % 2 == 0 || (c % 2 == 0 && teju_is_tie(c_2, -f)))
      return teju_make_fields(c, f);

    return teju_make_fields(c + 1, f);
  }

  teju_u1_t const m_b = 2 * m_0 + 1;
  teju_u1_t const b = teju_mshift(m_b << r, u, l);

  teju_u1_t const m_a = 4 * m_0 - 1;
  teju_u1_t const a = teju_mshift(m_a << r, u, l) / 2;

  if (b > a) {

    teju_u1_t const q = teju_div10(b);
    teju_u1_t const s = 10 * q;

    if (s > a || (s == a && teju_is_tie_uncentred(f)))
      return teju_remove_trailing_zeros(q, f + 1);

    // m_c = 2 * 2 * m_0 = 2 * 2 * 2^{teju_mantissa_size}
    // c_2 = teju_mshift(m_c << r, upper, lower);
    u32 const log2_m_c = teju_mantissa_size + 2;
    teju_u1_t const c_2 = teju_mshift_pow2(log2_m_c + r, u, l);
    teju_u1_t const c = c_2 / 2;

    if (c == a && !teju_is_tie_uncentred(f))
      return teju_make_fields(c + 1, f);

    if (c_2 % 2 == 0 || (c % 2 == 0 && teju_is_tie(c_2, -f)))
      return teju_make_fields(c, f);

    return teju_make_fields(c + 1, f);
  }

  else if (teju_is_tie_uncentred(f))
    return teju_remove_trailing_zeros(a, f);

  teju_u1_t const m_c = 10 * 2 * 2 * m_0;
  teju_u1_t const c_2 = teju_mshift(m_c << r, u, l);
  teju_u1_t const c = c_2 / 2;

  if (c_2 % 2 == 0 || (c % 2 == 0 && teju_is_tie(c_2, -f)))
    return teju_make_fields(c, f - 1);

  return teju_make_fields(c + 1, f - 1);
}

/**
 * @file teju/float.c
 * @brief Gets IEEE-754's binary32 representation of a float.
 *
 * See https://en.wikipedia.org/wiki/Single-precision_floating-point_format
 *
 * @pre value > 0.
 *
 * @param value             The given float.
 *
 * @returns IEEE-754's binary32 representation of value.
 */
static inline teju32_fields_t
teju_float_to_ieee32(f32 value)
{
  /**
   * @file teju/float.c
   *
   * Conversion from IEEE-754's parameters to Teju Jagua's.
   */
  enum {
    exponent_size = teju_ieee754_binary32_exponent_size,
    mantissa_size = teju_ieee754_binary32_mantissa_size,
    exponent_min = teju_ieee754_binary32_exponent_min - mantissa_size,
  };

  union f32u32 {
    f32 f;
    u32 u;
  };
  union f32u32 f = {value};
  u32 bits = f.u;

  teju32_fields_t binary;
  binary.mantissa = teju_lsb(bits, mantissa_size);
  bits >>= mantissa_size;
  binary.exponent = (s32)teju_lsb(bits, exponent_size);

  return binary;
}

static inline teju32_fields_t
teju_ieee32_to_binary(teju32_fields_t ieee32)
{
  /**
   * @file teju/float.c
   *
   * Conversion from IEEE-754's parameters to Teju Jagua's.
   */
  enum {
    exponent_size = teju_ieee754_binary32_exponent_size,
    mantissa_size = teju_ieee754_binary32_mantissa_size,
    exponent_min = teju_ieee754_binary32_exponent_min - mantissa_size,
  };

  s32 e = ieee32.exponent + exponent_min;
  u32 m = ieee32.mantissa;

  if (ieee32.exponent != 0) {
    e -= 1;
    m += teju_pow2(u64, mantissa_size);
  }

  teju32_fields_t teju_binary = {m, e};
  return teju_binary;
}

static teju32_fields_t
teju_float_to_decimal(f32 const value)
{
  teju32_fields_t ieee32 = teju_float_to_ieee32(value);
  teju32_fields_t teju_binary = teju_ieee32_to_binary(ieee32);
  return teju_ieee32_no_uint128(teju_binary);
}

/*
 * string buffer must at least able to hold 3 bytes.
 * fractionCount [1,51]
 *   minimum value for f32 requires 51 digits after decimal point
 */
static inline struct string
FormatF32(struct string *stringBuffer, f32 value, u32 fractionCount)
{
  debug_assert(fractionCount >= 1 && fractionCount <= 51);

  /*****************************************************************
   * INITIAL BUFFER CAPACITY CHECK
   *****************************************************************/
  struct string result = {.value = 0, .length = 0};
  if (!stringBuffer
      // 1 for integer, 1 for point, plus fraction count is minimal
      || stringBuffer->length < 2 + fractionCount)
    return result;

  // EDGE CASE: if value is 0.0f
  if (value == 0) {
    result.value = stringBuffer->value;
    result.length = 2 + fractionCount;
    for (u64 index = 0; index < result.length; index++) {
      result.value[index] = '0';
    }
    result.value[1] = '.';
    return result;
  }

  /*****************************************************************
   * CALCULATING LENGTH OF FLOAT
   *****************************************************************/
  // PERFORMANCE
  // TODO: Q: Is writing mantissa then adding decimal point more performant?
  // A: ?

  // Convert float to mantissa/exponent/sign
  teju32_fields_t fields = teju_float_to_decimal(value);
  b8 isNegative = value < 0.0f;
  u64 mantissa = fields.mantissa;
  s32 exponent = fields.exponent;

  // Determine mantissa digit count
  u32 mantissaDigitCount = 1;
  while (mantissaDigitCount < ARRAY_COUNT(POWERS_OF_10) && mantissa >= POWERS_OF_10[mantissaDigitCount])
    mantissaDigitCount++;

  // Calculate positions for point and zero padding
  u32 zeroBeforeCount = 0, zeroAfterCount = 0;
  u32 pointIndex;
  pointIndex = mantissaDigitCount + (u32)(exponent);

  if (exponent < 0) {
    u32 absExponent = (u32)-exponent;
    if (absExponent > mantissaDigitCount)
      zeroBeforeCount = absExponent - 1;
    else if (absExponent == mantissaDigitCount)
      zeroBeforeCount = 1;

    if (absExponent >= mantissaDigitCount)
      pointIndex = 1;
  } else {
    zeroAfterCount = (u32)exponent;
  }

  pointIndex += isNegative;
  debug_assert(zeroBeforeCount < 45 && "overflow");
  debug_assert(zeroAfterCount < 45 && "overflow");
  debug_assert(pointIndex > isNegative && "must be 1 min" && pointIndex < 45 && "overflow");

  // Adjust for fraction count
  u32 lengthLimit = zeroBeforeCount + mantissaDigitCount + zeroAfterCount + 1; // 1 is for point
  s32 fractionDifference = (s32)(
      // fraction count wanted
      fractionCount
      // minus the fraction count interpret from mantissa
      - (lengthLimit - (pointIndex + 1)));
  if (fractionDifference < 0) {
    lengthLimit += (u32)fractionDifference;
  } else if (fractionDifference > 0) {
    zeroAfterCount += (u32)fractionDifference;
    lengthLimit += (u32)fractionDifference;
  }
  // else perfect match

  /*****************************************************************
   * BUFFER CAPACITY CHECK
   *****************************************************************/
  // Check final buffer capacity
  if (stringBuffer->length < lengthLimit)
    return result;

  /*****************************************************************
   * CONVERTING VALUE TO STRING
   *****************************************************************/
  // Construct the formatted string
  u32 index = 0;

  // Add sign if negative
  if (isNegative) {
    stringBuffer->value[index] = '-';
    index++;
  }

  // Add decimal point
  stringBuffer->value[pointIndex] = '.';

  /**
   * Add leading zeros
   * 0.99f
   * └── put zero to integer's place
   */
  for (u32 zeroIndex = 0; zeroIndex < zeroBeforeCount && index < lengthLimit; zeroIndex++) {
    // skip point
    if (index == pointIndex)
      index++;

    stringBuffer->value[index] = '0';
    index++;
  }

  // Add mantissa digits
  while (mantissaDigitCount > 0 && index < lengthLimit) {
    // skip point
    if (index == pointIndex)
      index++;

    u64 power = POWERS_OF_10[mantissaDigitCount - 1];
    u64 digit = mantissa / power;

    stringBuffer->value[index] = (u8)digit + '0';

    mantissa -= digit * power;
    mantissaDigitCount--;
    index++;
  }

  /**
   * Add trailing zeros
   * 0.50f
   *    └── put zeros after putting fraction value
   */
  for (u32 zeroIndex = 0; zeroIndex < zeroAfterCount && index < lengthLimit; zeroIndex++) {
    // skip point
    if (index == pointIndex)
      index++;

    stringBuffer->value[index] = '0';
    index++;
  }

  // completed
  result.value = stringBuffer->value;
  result.length = index;
  return result;
}
