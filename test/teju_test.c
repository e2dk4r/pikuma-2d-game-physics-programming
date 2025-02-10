#include "teju.h"

// TODO: Show error pretty error message when a test fails
enum teju_test_error {
  TEJU_TEST_ERROR_NONE = 0,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_0,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_9,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_1_0,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_1_00,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_10,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_9_05,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_2_50,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_2_55,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_4_99,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_10234_293,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_0_9,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_1_0,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_1_00,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_0_10,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_2_50,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_2_55,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_00,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_F32_MAX,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_F32_MIN,
  TEJU_TEST_ERROR_FORMATF32_EXPECTED_F32_LOWEST,

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
  enum teju_test_error errorCode = TEJU_TEST_ERROR_NONE;

  { // setup
    // if () {
    //   errorCode = MESON_TEST_FAILED_TO_SET_UP;
    //   goto end;
    // }
  }

  // FormatF32(struct string *stringBuffer, f32 value, u32 fractionCount)
  {
    u8 buf[64];
    struct string stringBuffer = {.value = buf, .length = sizeof(buf)};
    struct string expected;
    struct string value;

    value = FormatF32(&stringBuffer, 0.0f, 1);
    expected = STRING_FROM_ZERO_TERMINATED("0.0");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_0;
      goto end;
    }

    value = FormatF32(&stringBuffer, 0.99f, 1);
    expected = STRING_FROM_ZERO_TERMINATED("0.9");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_9;
      goto end;
    }

    value = FormatF32(&stringBuffer, 1.0f, 1);
    expected = STRING_FROM_ZERO_TERMINATED("1.0");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_1_0;
      goto end;
    }

    value = FormatF32(&stringBuffer, 1.0f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("1.00");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_1_00;
      goto end;
    }

    value = FormatF32(&stringBuffer, 0.1f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("0.10");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_10;
      goto end;
    }

    value = FormatF32(&stringBuffer, 9.05f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("9.05");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_9_05;
      goto end;
    }

    value = FormatF32(&stringBuffer, 2.50f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("2.50");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_2_50;
      goto end;
    }

    value = FormatF32(&stringBuffer, 2.55999f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("2.55");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_2_55;
      goto end;
    }

    value = FormatF32(&stringBuffer, 4.99966526f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("4.99");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_4_99;
      goto end;
    }

    value = FormatF32(&stringBuffer, 10234.293f, 3);
    expected = STRING_FROM_ZERO_TERMINATED("10234.293");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_10234_293;
      goto end;
    }

    value = FormatF32(&stringBuffer, -0.99f, 1);
    expected = STRING_FROM_ZERO_TERMINATED("-0.9");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_0_9;
      goto end;
    }

    value = FormatF32(&stringBuffer, -1.0f, 1);
    expected = STRING_FROM_ZERO_TERMINATED("-1.0");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_1_0;
      goto end;
    }

    value = FormatF32(&stringBuffer, -1.0f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("-1.00");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_1_00;
      goto end;
    }

    value = FormatF32(&stringBuffer, -0.1f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("-0.10");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_0_10;
      goto end;
    }

    value = FormatF32(&stringBuffer, -2.50f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("-2.50");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_2_50;
      goto end;
    }

    value = FormatF32(&stringBuffer, -2.55999f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("-2.55");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_NEGATIVE_2_55;
      goto end;
    }

    value = FormatF32(&stringBuffer, 3.76991844e-25f, 2);
    expected = STRING_FROM_ZERO_TERMINATED("0.00");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_0_00;
      goto end;
    }

    value = FormatF32(&stringBuffer, F32_MAX, 1);
    expected = STRING_FROM_ZERO_TERMINATED("340282350000000000000000000000000000000.0");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_F32_MAX;
      goto end;
    }

    value = FormatF32(&stringBuffer, F32_MIN, 51);
    expected = STRING_FROM_ZERO_TERMINATED("0.000000000000000000000000000000000000000000011754944");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_F32_MIN;
      goto end;
    }

    value = FormatF32(&stringBuffer, F32_LOWEST, 1);
    expected = STRING_FROM_ZERO_TERMINATED("-340282350000000000000000000000000000000.0");
    if (!IsStringEqual(&value, &expected)) {
      errorCode = TEJU_TEST_ERROR_FORMATF32_EXPECTED_F32_LOWEST;
      goto end;
    }
  }

end:
  return (int)errorCode;
}
