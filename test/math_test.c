#include "math.h"

// TODO: Show error pretty error message when a test fails
enum math_test_error {
  MATH_TEST_ERROR_NONE = 0,
  MEMORY_TEST_ERROR_CLAMP_EXPECTED_INPUT,
  MEMORY_TEST_ERROR_CLAMP_EXPECTED_MIN,
  MEMORY_TEST_ERROR_CLAMP_EXPECTED_MAX,
  MEMORY_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_TRUE,
  MEMORY_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_FALSE,

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

  // Clamp(u32 value, u32 min, u32 max)
  {
    u32 min, max, input, expected, value;

    min = 3;
    max = 5;
    input = 4;
    expected = input;
    value = Clamp(input, min, max);
    if (value != expected) {
      errorCode = MEMORY_TEST_ERROR_CLAMP_EXPECTED_INPUT;
      goto end;
    }

    input = min - 1;
    expected = min;
    value = Clamp(input, min, max);
    if (value != expected) {
      errorCode = MEMORY_TEST_ERROR_CLAMP_EXPECTED_MIN;
      goto end;
    }

    input = max + 1;
    expected = max;
    value = Clamp(input, min, max);
    if (value != expected) {
      errorCode = MEMORY_TEST_ERROR_CLAMP_EXPECTED_MAX;
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
      errorCode = MEMORY_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_TRUE;
      goto end;
    }

    input = 37;
    expected = 0;
    value = IsPowerOfTwo(input);
    if (value != expected) {
      errorCode = MEMORY_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_FALSE;
      goto end;
    }
  }

end:
  return (int)errorCode;
}
