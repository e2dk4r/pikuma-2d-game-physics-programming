#include "math.h"

// TODO: Show error pretty error message when a test fails
enum math_test_error {
  MATH_TEST_ERROR_NONE = 0,
  MATH_TEST_ERROR_CLAMP_EXPECTED_INPUT,
  MATH_TEST_ERROR_CLAMP_EXPECTED_MIN,
  MATH_TEST_ERROR_CLAMP_EXPECTED_MAX,
  MATH_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_TRUE,
  MATH_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_FALSE,
  MATH_TEST_ERROR_V2_ADD,
  MATH_TEST_ERROR_V2_SUB,
  MATH_TEST_ERROR_V2_SCALE,
  MATH_TEST_ERROR_V2_DOT,
  MATH_TEST_ERROR_V2_HADAMARD,
  MATH_TEST_ERROR_V2_PERP,
  MATH_TEST_ERROR_V2_LENGTH_SQUARE,
  MATH_TEST_ERROR_V2_LENGTH,
  MATH_TEST_ERROR_V2_NORMALIZE,
  MATH_TEST_ERROR_V2_NEG,
  MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_TRUE,
  MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_FALSE,
  MATH_TEST_ERROR_IS_AABB_OVERLAPPING_EXPECTED_TRUE,
  MATH_TEST_ERROR_IS_AABB_OVERLAPPING_EXPECTED_FALSE,

  // src: https://mesonbuild.com/Unit-tests.html#skipped-tests-and-hard-errors
  // For the default exitcode testing protocol, the GNU standard approach in
  // this case is to exit the program with error code 77. Meson will detect this
  // and report these tests as skipped rather than failed. This behavior was
  // added in version 0.37.0.
  MESON_TEST_SKIP = 77,
  // In addition, sometimes a test fails set up so that it should fail even if
  // it is marked as an expected failure. The GNU standard approach in this case
  // is to exit the program with error code 99. Again, Meson will detect this
  // and report these tests as ERROR, ignoring the setting of should_fail. This
  // behavior was added in version 0.50.0.
  MESON_TEST_FAILED_TO_SET_UP = 99,
};

int
main(void)
{
  enum math_test_error errorCode = MATH_TEST_ERROR_NONE;

  // setup
  {
  }

  // ClampU32(u32 value, u32 min, u32 max)
  {
    u32 min, max, input, expected, value;

    min = 3;
    max = 5;
    input = 4;
    expected = input;
    value = ClampU32(input, min, max);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_CLAMP_EXPECTED_INPUT;
      goto end;
    }

    input = min - 1;
    expected = min;
    value = ClampU32(input, min, max);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_CLAMP_EXPECTED_MIN;
      goto end;
    }

    input = max + 1;
    expected = max;
    value = ClampU32(input, min, max);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_CLAMP_EXPECTED_MAX;
      goto end;
    }
  }

  // IsPowerOfTwo(u64 value)
  {
    u64 input;
    b8 expected, value;

    input = 32;
    expected = 1;
    value = IsPowerOfTwo(input);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_TRUE;
      goto end;
    }

    input = 37;
    expected = 0;
    value = IsPowerOfTwo(input);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_FALSE;
      goto end;
    }
  }

  // v2_add(v2 a, v2 b)
  {
    struct v2 a = {3.0f, 4.0f};
    struct v2 b = {9.0f, 12.0f};
    struct v2 expected = {12.0f, 16.0f};
    struct v2 value = v2_add(a, b);
    if (value.x != expected.x || value.y != expected.y) {
      errorCode = MATH_TEST_ERROR_V2_ADD;
      goto end;
    }
  }

  // v2_sub(v2 a, v2 b)
  {
    struct v2 a = {9.0f, 12.0f};
    struct v2 b = {3.0f, 4.0f};
    struct v2 expected = {6.0f, 8.0f};
    struct v2 value = v2_sub(a, b);
    if (value.x != expected.x || value.y != expected.y) {
      errorCode = MATH_TEST_ERROR_V2_SUB;
      goto end;
    }
  }

  // v2_scale(v2 a, f32 scaler)
  {
    struct v2 a = {3.0f, 4.0f};
    f32 scaler = 5.0f;
    struct v2 expected = {15.0f, 20.0f};
    struct v2 value = v2_scale(a, scaler);
    if (value.x != expected.x || value.y != expected.y) {
      errorCode = MATH_TEST_ERROR_V2_DOT;
      goto end;
    }
  }

  // v2_dot(v2 a, v2 b)
  {
    struct v2 a = {3.0f, 4.0f};
    struct v2 b = {9.0f, 12.0f};
    f32 expected = 3.0f * 9.0f + 4.0f * 12.0f;
    f32 value = v2_dot(a, b);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_V2_DOT;
      goto end;
    }
  }

  // v2_hadamard(v2 a, v2 b)
  {
    struct v2 a = {3.0f, 4.0f};
    struct v2 b = {9.0f, 12.0f};
    struct v2 expected = {27.0f, 48.0f};
    struct v2 value = v2_hadamard(a, b);
    if (value.x != expected.x || value.y != expected.y) {
      errorCode = MATH_TEST_ERROR_V2_HADAMARD;
      goto end;
    }
  }

  // v2_perp(v2 a)
  {
    struct v2 a = {1.0f, 2.0f};
    struct v2 expected = {-2.0f, 1.0f};
    struct v2 value = v2_perp(a);
    if (value.x != expected.x || value.y != expected.y) {
      errorCode = MATH_TEST_ERROR_V2_PERP;
      goto end;
    }
  }

  // v2_length_square(v2 a)
  {
    struct v2 a = {3.0f, 4.0f};
    f32 expected = 5.0f * 5.0f;
    f32 value = v2_length_square(a);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_V2_LENGTH_SQUARE;
      goto end;
    }
  }

  // v2_length(v2 a)
  {
    struct v2 a = {3.0f, 4.0f};
    f32 expected = 5.0f;
    f32 value = v2_length(a);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_V2_LENGTH;
      goto end;
    }
  }

  // v2_normalize(v2 a)
  {
    struct v2 a = {3.0f, 4.0f};
    struct v2 expected = {3.0f / 5.0f, 4.0f / 5.0f};
    struct v2 value = v2_normalize(a);
    if (value.x != expected.x || value.y != expected.y) {
      errorCode = MATH_TEST_ERROR_V2_NORMALIZE;
      goto end;
    }
  }

  // v2_neg(v2 a)
  {
    struct v2 a = {3.0f, 4.0f};
    struct v2 expected = {-3.0f, -4.0f};
    struct v2 value = v2_neg(a);
    if (value.x != expected.x || value.y != expected.y) {
      errorCode = MATH_TEST_ERROR_V2_NEG;
      goto end;
    }
  }

  // IsPointInsideRect(struct rect rect, v2 point)
  {
    struct rect rect = {
        .min = {0.0f, 0.0f},
        .max = {10.0f, 10.0f},
    };
    struct v2 point;
    b8 expected, value;

    point = (struct v2){0.0f, 0.0f};
    expected = 1;
    value = IsPointInsideRect(point, rect);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_TRUE;
      goto end;
    }

    point = (struct v2){1.0f, 1.0f};
    expected = 1;
    value = IsPointInsideRect(point, rect);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_TRUE;
      goto end;
    }

    point = (struct v2){10.0f, 10.0f};
    expected = 0;
    value = IsPointInsideRect(point, rect);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_FALSE;
      goto end;
    }

    point = (struct v2){50.0f, 50.0f};
    expected = 0;
    value = IsPointInsideRect(point, rect);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_FALSE;
      goto end;
    }
  }

  // IsAABBOverlapping(struct rect a, struct rect b)
  {
    struct rect a = RectCenterDim(V2(0.0f, 0.0f), V2(10.0f, 10.0f));
    struct rect b = RectCenterDim(V2(0.0f, 0.0f), V2(5.0f, 5.0f));
    struct rect c = RectCenterDim(V2(100.0f, 100.0f), V2(5.0f, 5.0f));
    b8 expected, value;

    expected = 1;
    value = IsAABBOverlapping(a, b);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_AABB_OVERLAPPING_EXPECTED_TRUE;
      goto end;
    }

    expected = 0;
    value = IsAABBOverlapping(a, c);
    if (value != expected) {
      errorCode = MATH_TEST_ERROR_IS_AABB_OVERLAPPING_EXPECTED_FALSE;
      goto end;
    }
  }

end:
  return (int)errorCode;
}
