#include "log.h"
#include "string_builder.h"
#include "text.h"

#define TEST_ERROR_LIST                                                                                                \
  TEST_ERROR(TEXT_TEST_ERROR_STRING_FROM_ZERO_TERMINATED, "Failed to create string from zero terminated c-string")     \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_TRUE, "Strings must be equal")                                    \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_FALSE, "Strings must NOT be equal")                               \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_TRUE, "Strings that are case ignored must be equal")  \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_FALSE,                                                \
             "Strings that are case ignored must NOT be equal")                                                        \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_TRUE, "String must contain search string")                    \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_FALSE, "String must NOT contain search string")               \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_TRUE, "String must start with search string")              \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_FALSE, "String must NOT start with search string")         \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_TRUE, "String must end with search string")                  \
  TEST_ERROR(TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_FALSE, "String must NOT end with search string")             \
  TEST_ERROR(TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_TRUE, "Parsing duration string must be successful")               \
  TEST_ERROR(TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_FALSE, "Parsing duration string must fail")                       \
  TEST_ERROR(TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_TRUE, "lhs duration must be less then rhs")                \
  TEST_ERROR(TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_FALSE, "lhs duration must NOT be less then rhs")           \
  TEST_ERROR(TEXT_TEST_ERROR_IS_DURATION_GRATER_THAN_EXPECTED_TRUE, "lhs duration must be grater then rhs")            \
  TEST_ERROR(TEXT_TEST_ERROR_IS_DURATION_GRATER_THAN_EXPECTED_FALSE, "lhs duration must NOT be grater then rhs")       \
  TEST_ERROR(TEXT_TEST_ERROR_FORMATU64_EXPECTED, "Formatting u64 value must be successful")                            \
  TEST_ERROR(TEXT_TEST_ERROR_FORMATF32SLOW_EXPECTED, "Formatting f32 value must be successful")                        \
  TEST_ERROR(TEXT_TEST_ERROR_FORMATHEX_EXPECTED, "Formatting value to hex must be successful")                         \
  TEST_ERROR(TEXT_TEST_ERROR_PATHGETDIRECTORY, "Extracting path's parent directory must be successful")                \
  TEST_ERROR(TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_TRUE, "Splitting string into parts must be successful")              \
  TEST_ERROR(TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_FALSE, "Splitting string into parts must be fail")

enum text_test_error {
  TEXT_TEST_ERROR_NONE = 0,
#define TEST_ERROR(tag, message) tag,
  TEST_ERROR_LIST

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

comptime struct text_test_error_info {
  enum text_test_error code;
  struct string message;
} TEXT_TEST_ERRORS[] = {
#undef TEST_ERROR
#define TEST_ERROR(tag, msg) {.code = tag, .message = STRING_FROM_ZERO_TERMINATED(msg)},
    TEST_ERROR_LIST};

internalfn string *
GetTextTestErrorMessage(enum text_test_error errorCode)
{
  for (u32 index = 0; index < ARRAY_COUNT(TEXT_TEST_ERRORS); index++) {
    const struct text_test_error_info *info = TEXT_TEST_ERRORS + index;
    if (info->code == errorCode)
      return (struct string *)&info->message;
  }
  return 0;
}

internalfn void
StringBuilderAppendBool(string_builder *sb, b8 value)
{
  StringBuilderAppendString(sb, value ? &STRING_FROM_ZERO_TERMINATED("true") : &STRING_FROM_ZERO_TERMINATED("false"));
}

internalfn void
StringBuilderAppendPrintableString(string_builder *sb, struct string *string)
{
  if (string->value == 0)
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("(NULL)"));
  else if (string->value != 0 && string->length == 0)
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("(EMPTY)"));
  else if (string->length == 1 && string->value[0] == ' ')
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("(SPACE)"));
  else
    StringBuilderAppendString(sb, string);
}

int
main(void)
{
  enum text_test_error errorCode = TEXT_TEST_ERROR_NONE;

  // setup
  u32 KILOBYTES = 1 << 10;
  u8 stackBuffer[8 * KILOBYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MemoryArenaPushUnaligned(&stackMemory, sizeof(*sb));
  {
    string *outBuffer = MemoryArenaPushUnaligned(&stackMemory, sizeof(*outBuffer));
    outBuffer->length = 1024;
    outBuffer->value = MemoryArenaPushUnaligned(&stackMemory, outBuffer->length);
    sb->outBuffer = outBuffer;

    string *stringBuffer = MemoryArenaPushUnaligned(&stackMemory, sizeof(*stringBuffer));
    stringBuffer->length = 32;
    stringBuffer->value = MemoryArenaPushUnaligned(&stackMemory, stringBuffer->length);
    sb->stringBuffer = stringBuffer;

    sb->length = 0;
  }

  // struct string StringFromZeroTerminated(u8 *src, u64 max)
  {
    struct test_case {
      char *input;
      u64 length;
      u64 expected;
    } testCases[] = {
        {
            .input = "abc",
            .length = 1024,
            .expected = 3,
        },
        {
            .input = "abcdefghijklm",
            .length = 3,
            .expected = 3,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u64 expected = testCase->expected;
      char *input = testCase->input;
      u64 length = testCase->length;

      struct string got = StringFromZeroTerminated((u8 *)input, length);
      if (got.value != (u8 *)input || got.length != expected) {
        errorCode = TEXT_TEST_ERROR_STRING_FROM_ZERO_TERMINATED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendU64(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendU64(sb, got.length);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // b8 IsStringEqual(struct string *left, struct string *right)
  b8 IsStringEqualOK = 1;
  {
    struct test_case {
      struct string *left;
      struct string *right;
      b8 expected;
    } testCases[] = {
        {
            .left = &STRING_FROM_ZERO_TERMINATED("abc"),
            .right = &STRING_FROM_ZERO_TERMINATED("abc"),
            .expected = 1,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED("abc"),
            .right = &STRING_FROM_ZERO_TERMINATED("ABC"),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED("abc"),
            .right = &STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .expected = 0,
        },
        // NULL
        {
            .left = &(struct string){.value = 0},
            .right = &STRING_FROM_ZERO_TERMINATED("foo"),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED("foo"),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &(struct string){.value = 0},
            .expected = 1,
        },
        // EMPTY
        {
            .left = &STRING_FROM_ZERO_TERMINATED(""),
            .right = &STRING_FROM_ZERO_TERMINATED(""),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &STRING_FROM_ZERO_TERMINATED(""),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(""),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        // SPACE
        {
            .left = &STRING_FROM_ZERO_TERMINATED(" "),
            .right = &STRING_FROM_ZERO_TERMINATED(" "),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &STRING_FROM_ZERO_TERMINATED(" "),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(" "),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(""),
            .right = &STRING_FROM_ZERO_TERMINATED(" "),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(" "),
            .right = &STRING_FROM_ZERO_TERMINATED(""),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *left = testCase->left;
      struct string *right = testCase->right;
      b8 expected = testCase->expected;
      b8 value = IsStringEqual(left, right);
      if (value != expected) {
        IsStringEqualOK = 0;
        errorCode =
            expected ? TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_TRUE : TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  left:     "));
        StringBuilderAppendPrintableString(sb, left);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  right:    "));
        StringBuilderAppendPrintableString(sb, right);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // b8 IsStringEqualIgnoreCase(struct string *left, struct string *right)
  {
    struct test_case {
      struct string *left;
      struct string *right;
      b8 expected;
    } testCases[] = {
        {
            .left = &STRING_FROM_ZERO_TERMINATED("abc"),
            .right = &STRING_FROM_ZERO_TERMINATED("ABC"),
            .expected = 1,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED("ABC"),
            .right = &STRING_FROM_ZERO_TERMINATED("abc"),
            .expected = 1,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED("abc"),
            .right = &STRING_FROM_ZERO_TERMINATED("abc"),
            .expected = 1,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED("abc"),
            .right = &STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .expected = 0,
        },
        // NULL
        {
            .left = &(struct string){.value = 0},
            .right = &STRING_FROM_ZERO_TERMINATED("foo"),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED("foo"),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &(struct string){.value = 0},
            .expected = 1,
        },
        // EMPTY
        {
            .left = &STRING_FROM_ZERO_TERMINATED(""),
            .right = &STRING_FROM_ZERO_TERMINATED(""),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &STRING_FROM_ZERO_TERMINATED(""),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(""),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        // SPACE
        {
            .left = &STRING_FROM_ZERO_TERMINATED(" "),
            .right = &STRING_FROM_ZERO_TERMINATED(" "),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &STRING_FROM_ZERO_TERMINATED(" "),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(" "),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(""),
            .right = &STRING_FROM_ZERO_TERMINATED(" "),
            .expected = 0,
        },
        {
            .left = &STRING_FROM_ZERO_TERMINATED(" "),
            .right = &STRING_FROM_ZERO_TERMINATED(""),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *left = testCase->left;
      struct string *right = testCase->right;
      b8 expected = testCase->expected;
      b8 value = IsStringEqualIgnoreCase(left, right);
      if (value != expected) {
        IsStringEqualOK = 0;
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  left:     "));
        StringBuilderAppendPrintableString(sb, left);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  right:    "));
        StringBuilderAppendPrintableString(sb, right);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // IsStringContains(struct string *string, struct string *search)
  {
    struct test_case {
      struct string string;
      struct string search;
      b8 expected;
    } testCases[] = {
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("abc"),
            .expected = 1,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("def"),
            .expected = 1,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("ghi"),
            .expected = 1,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("ghijkl"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("jkl"),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *string = &testCase->string;
      struct string *search = &testCase->search;
      b8 expected = testCase->expected;
      b8 value = IsStringContains(string, search);
      if (value != expected) {
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  string:   "));
        StringBuilderAppendString(sb, string);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  search:   "));
        StringBuilderAppendString(sb, search);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // IsStringStartsWith(struct string *string, struct string *search)
  {
    struct test_case {
      struct string string;
      struct string search;
      b8 expected;
      enum text_test_error error;
    } testCases[] = {
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("abc"),
            .expected = 1,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("def"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("ghi"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("ghijkl"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("jkl"),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *string = &testCase->string;
      struct string *search = &testCase->search;
      b8 expected = testCase->expected;
      b8 value = IsStringStartsWith(string, search);
      if (value != expected) {
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  string:   "));
        StringBuilderAppendString(sb, string);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  search:   "));
        StringBuilderAppendString(sb, search);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // b8 IsStringEndsWith(struct string *string, struct string *search)
  {
    struct test_case {
      struct string string;
      struct string search;
      b8 expected;
      enum text_test_error error;
    } testCases[] = {
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("ghi"),
            .expected = 1,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("abc"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("def"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("abc def"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc def ghi"),
            .search = STRING_FROM_ZERO_TERMINATED("jkl"),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *string = &testCase->string;
      struct string *search = &testCase->search;
      b8 expected = testCase->expected;
      b8 value = IsStringEndsWith(string, search);
      if (value != expected) {
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  string:   "));
        StringBuilderAppendString(sb, string);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  search:   "));
        StringBuilderAppendString(sb, search);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // ParseDuration(struct string *string, struct duration *duration)
  {
    struct test_case {
      struct string string;
      u64 expectedDurationInNanoseconds;
      b8 expected;
    } testCases[] = {
        {
            .string = STRING_FROM_ZERO_TERMINATED("1ns"),
            .expected = 1,
            .expectedDurationInNanoseconds = 1,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("1ns"),
            .expected = 1,
            .expectedDurationInNanoseconds = 1,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("1sec"),
            .expected = 1,
            .expectedDurationInNanoseconds = 1 * 1000000000ull /* 1e9 */,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("5sec"),
            .expected = 1,
            .expectedDurationInNanoseconds = 5 * 1000000000ull /* 1e9 */,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("7min"),
            .expected = 1,
            .expectedDurationInNanoseconds = 1000000000ull /* 1e9 */ * 60 * 7,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("1hr5min"),
            .expected = 1,
            .expectedDurationInNanoseconds =
                (1000000000ull /* 1e9 */ * 60 * 60 * 1) + (1000000000ull /* 1e9 */ * 60 * 5),
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("1hr5min"),
            .expected = 1,
            .expectedDurationInNanoseconds =
                (1000000000ull /* 1e9 */ * 60 * 60 * 1) + (1000000000ull /* 1e9 */ * 60 * 5),
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("10day"),
            .expected = 1,
            .expectedDurationInNanoseconds = 1000000000ULL /* 1e9 */ * 60 * 60 * 24 * 10,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("10day1sec"),
            .expected = 1,
            .expectedDurationInNanoseconds =
                (1000000000ull /* 1e9 */ * 60 * 60 * 24 * 10) + (1000000000ull /* 1e9 */ * 1),
        },
        {
            .string = (struct string){}, // NULL
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED(""), // EMPTY
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED(" "), // SPACE
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("abc"),
            .expected = 0,
        },
        {
            .string = STRING_FROM_ZERO_TERMINATED("5m5s"),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;
      struct string *string = &testCase->string;
      struct duration duration;

      u64 expectedDurationInNanoseconds = testCase->expectedDurationInNanoseconds;
      b8 expected = testCase->expected;
      b8 value = ParseDuration(string, &duration);
      if (value != expected || (expected && duration.ns != expectedDurationInNanoseconds)) {
        errorCode =
            expected ? TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_TRUE : TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  string:   "));
        StringBuilderAppendPrintableString(sb, string);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendBool(sb, expected);
        if (expected) {
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" duration: "));
          StringBuilderAppendU64(sb, expectedDurationInNanoseconds);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("ns"));
        }
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendBool(sb, value);
        if (expected) {
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" duration: "));
          StringBuilderAppendU64(sb, duration.ns);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("ns"));
        }
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // IsDurationLessThan(struct duration *left, struct duration *right)
  // IsDurationGraterThan(struct duration *left, struct duration *right)
  {
    struct duration left;
    struct duration right;
    b8 expectedLessThan;
    b8 expectedGraterThan;

    left = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1};
    right = (struct duration){.ns = 1000000000ull /* 1e9 */ * 5};
    expectedLessThan = 1;
    if (IsDurationLessThan(&left, &right) != expectedLessThan) {
      errorCode = TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_TRUE;
      goto end;
    }
    expectedGraterThan = 0;
    if (IsDurationGraterThan(&left, &right) != expectedGraterThan) {
      errorCode = TEXT_TEST_ERROR_IS_DURATION_GRATER_THAN_EXPECTED_FALSE;
      goto end;
    }

    left = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1};
    right = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1};
    expectedLessThan = 0;
    if (IsDurationLessThan(&left, &right) != expectedLessThan) {
      errorCode = TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_FALSE;
      goto end;
    }
    expectedGraterThan = 0;
    if (IsDurationGraterThan(&left, &right) != expectedGraterThan) {
      errorCode = TEXT_TEST_ERROR_IS_DURATION_GRATER_THAN_EXPECTED_FALSE;
      goto end;
    }

    left = (struct duration){.ns = 1000000000ull /* 1e9 */ * 5};
    right = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1};
    expectedLessThan = 0;
    if (IsDurationLessThan(&left, &right) != expectedLessThan) {
      errorCode = TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_FALSE;
      goto end;
    }
    expectedGraterThan = 1;
    if (IsDurationGraterThan(&left, &right) != expectedGraterThan) {
      errorCode = TEXT_TEST_ERROR_IS_DURATION_GRATER_THAN_EXPECTED_TRUE;
      goto end;
    }
  }

  // struct string FormatU64(struct string *stringBuffer, u64 value)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      u64 input;
      struct string expected;
    } testCases[] = {
        {
            .input = 0,
            .expected = STRING_FROM_ZERO_TERMINATED("0"),
        },
        {
            .input = 1,
            .expected = STRING_FROM_ZERO_TERMINATED("1"),
        },
        {
            .input = 10,
            .expected = STRING_FROM_ZERO_TERMINATED("10"),
        },
        {
            .input = 3912,
            .expected = STRING_FROM_ZERO_TERMINATED("3912"),
        },
        {
            .input = 18446744073709551615UL,
            .expected = STRING_FROM_ZERO_TERMINATED("18446744073709551615"),
        },
        // TODO: fail cases for FormatU64()
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u8 buf[20];
      struct string stringBuffer = {.value = buf, .length = sizeof(buf)};

      u64 input = testCase->input;
      struct string *expected = &testCase->expected;
      struct string value = FormatU64(&stringBuffer, input);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_FORMATU64_EXPECTED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  input:    "));
        StringBuilderAppendU64(sb, input);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendString(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendString(sb, &value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // struct string FormatF32Slow(struct string *stringBuffer, f32 value, u32 fractionCount)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      f32 input;
      u32 fractionCount;
      struct string expected;
    } testCases[] = {
        {
            .input = 0.99f,
            .fractionCount = 1,
            .expected = STRING_FROM_ZERO_TERMINATED("0.9"),
        },
        {
            .input = 0.99f,
            .fractionCount = 1,
            .expected = STRING_FROM_ZERO_TERMINATED("0.9"),
        },
        {
            .input = 1.0f,
            .fractionCount = 1,
            .expected = STRING_FROM_ZERO_TERMINATED("1.0"),
        },
        {
            .input = 1.0f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("1.00"),
        },
        {
            .input = 9.05f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("9.05"),
        },
        {
            .input = 2.50f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("2.50"),
        },
        {
            .input = 2.55999f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("2.56"),
        },
        {
            .input = 4.99966526f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("4.99"),
        },
        {
            .input = 10234.293f,
            .fractionCount = 3,
            .expected = STRING_FROM_ZERO_TERMINATED("10234.293"),
        },
        {
            .input = -0.99f,
            .fractionCount = 1,
            .expected = STRING_FROM_ZERO_TERMINATED("-0.9"),
        },
        {
            .input = -1.0f,
            .fractionCount = 1,
            .expected = STRING_FROM_ZERO_TERMINATED("-1.0"),
        },
        {
            .input = -1.0f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("-1.00"),
        },
        {
            .input = -2.50f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("-2.50"),
        },
        {
            .input = -2.55999f,
            .fractionCount = 2,
            .expected = STRING_FROM_ZERO_TERMINATED("-2.56"),
        },
        // TODO: fail cases for FormatF32Slow()
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u8 buf[20];
      struct string stringBuffer = {.value = buf, .length = sizeof(buf)};

      f32 input = testCase->input;
      u32 fractionCount = testCase->fractionCount;
      struct string *expected = &testCase->expected;
      struct string value = FormatF32Slow(&stringBuffer, input, fractionCount);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_FORMATF32SLOW_EXPECTED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  input (with 12 fraction count): "));
        StringBuilderAppendF32(sb, input, 12);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n                   fractionCount: "));
        StringBuilderAppendU64(sb, fractionCount);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendString(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendString(sb, &value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // struct string FormatHex(struct string *stringBuffer, u64 value)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      u64 input;
      struct string expected;
    } testCases[] = {
        {
            .input = 0x0,
            .expected = STRING_FROM_ZERO_TERMINATED("0x00"),
        },
        {
            .input = 0x4,
            .expected = STRING_FROM_ZERO_TERMINATED("0x04"),
        },
        {
            .input = 0x00f2aa499b9028eaUL,
            .expected = STRING_FROM_ZERO_TERMINATED("0x00f2aa499b9028ea"),
        },
        // TODO: fail cases for FormatHex()
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u8 buf[18];
      struct string stringBuffer = {.value = buf, .length = sizeof(buf)};

      u64 input = testCase->input;
      struct string *expected = &testCase->expected;
      struct string value = FormatHex(&stringBuffer, input);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_FORMATHEX_EXPECTED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  input:    "));
        StringBuilderAppendU64(sb, input);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendString(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendString(sb, &value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // struct string PathGetDirectory(struct string *path)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      struct string input;
      struct string expected;
    } testCases[] = {
        {
            .input = STRING_FROM_ZERO_TERMINATED("/usr/bin/ls"),
            .expected = STRING_FROM_ZERO_TERMINATED("/usr/bin"),
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED("/usr"),
            .expected = STRING_FROM_ZERO_TERMINATED("/"),
        },
        {
            .input = (struct string){.value = 0},
            .expected = (struct string){.value = 0},
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED(""),
            .expected = (struct string){.value = 0},
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED(" "),
            .expected = (struct string){.value = 0},
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED("no directory"),
            .expected = (struct string){.value = 0},
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *input = &testCase->input;
      struct string *expected = &testCase->expected;
      struct string value = PathGetDirectory(input);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_PATHGETDIRECTORY;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  input:    "));
        StringBuilderAppendPrintableString(sb, input);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
        StringBuilderAppendPrintableString(sb, &value);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      }
    }
  }

  // b8 StringSplit(struct string *string, struct string *seperator, u64 *splitCount, struct string *splits)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    __cleanup_memory_temp__ memory_temp tempMemory = MemoryTempBegin(&stackMemory);

    struct test_case {
      struct string input;
      struct string seperator;
      struct {
        b8 value;
        u64 splitCount;
        struct string *splits;
      } expected;
    } testCases[] = {
        {
            .input = STRING_FROM_ZERO_TERMINATED("1 2 3"),
            .seperator = STRING_FROM_ZERO_TERMINATED(" "),
            .expected =
                {
                    .value = 1,
                    .splitCount = 3,
                    .splits =
                        (struct string[]){
                            STRING_FROM_ZERO_TERMINATED("1"),
                            STRING_FROM_ZERO_TERMINATED("2"),
                            STRING_FROM_ZERO_TERMINATED("3"),
                        },
                },
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED("1xx2xx3"),
            .seperator = STRING_FROM_ZERO_TERMINATED("xx"),
            .expected =
                {
                    .value = 1,
                    .splitCount = 3,
                    .splits =
                        (struct string[]){
                            STRING_FROM_ZERO_TERMINATED("1"),
                            STRING_FROM_ZERO_TERMINATED("2"),
                            STRING_FROM_ZERO_TERMINATED("3"),
                        },
                },
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED("1xoxo2xo3"),
            .seperator = STRING_FROM_ZERO_TERMINATED("xo"),
            .expected =
                {
                    .value = 1,
                    .splitCount = 4,
                    .splits =
                        (struct string[]){
                            STRING_FROM_ZERO_TERMINATED("1"),
                            {},
                            STRING_FROM_ZERO_TERMINATED("2"),
                            STRING_FROM_ZERO_TERMINATED("3"),
                        },
                },
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED("1xo2xo3xo"),
            .seperator = STRING_FROM_ZERO_TERMINATED("xo"),
            .expected =
                {
                    .value = 1,
                    .splitCount = 4,
                    .splits =
                        (struct string[]){
                            STRING_FROM_ZERO_TERMINATED("1"),
                            STRING_FROM_ZERO_TERMINATED("2"),
                            STRING_FROM_ZERO_TERMINATED("3"),
                            {},
                        },
                },
        },
        {
            .input = STRING_FROM_ZERO_TERMINATED("Lorem ipsum dolor sit amet, consectetur adipiscing elit"),
            .seperator = STRING_FROM_ZERO_TERMINATED(" "),
            .expected =
                {
                    .value = 1,
                    .splitCount = 8,
                    .splits =
                        (struct string[]){
                            STRING_FROM_ZERO_TERMINATED("Lorem"),
                            STRING_FROM_ZERO_TERMINATED("ipsum"),
                            STRING_FROM_ZERO_TERMINATED("dolor"),
                            STRING_FROM_ZERO_TERMINATED("sit"),
                            STRING_FROM_ZERO_TERMINATED("amet,"),
                            STRING_FROM_ZERO_TERMINATED("consectetur"),
                            STRING_FROM_ZERO_TERMINATED("adipiscing"),
                            STRING_FROM_ZERO_TERMINATED("elit"),
                        },
                },
        },
        // TODO: fail cases for StringSplit();
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *input = &testCase->input;
      struct string *seperator = &testCase->seperator;

      if (IsStringEqual(input, &STRING_FROM_ZERO_TERMINATED("1xoxo2xo3"))) {
        u32 breakHere = 1;
      }

      u64 expectedSplitCount = testCase->expected.splitCount;
      b8 expected = testCase->expected.value;
      u64 splitCount;
      b8 value = StringSplit(input, seperator, &splitCount, 0);
      if (value != expected || splitCount != expectedSplitCount) {
        errorCode = expected ? TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_TRUE : TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  input:    "));
        StringBuilderAppendString(sb, input);
        if (value != expected) {
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected: "));
          StringBuilderAppendBool(sb, expected);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n       got: "));
          StringBuilderAppendBool(sb, value);
        } else {
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  expected split count: "));
          StringBuilderAppendU64(sb, expectedSplitCount);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n                   got: "));
          StringBuilderAppendU64(sb, splitCount);
        }
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        struct string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);
      } else {
        struct string *expectedSplits = testCase->expected.splits;
        struct string *splits = MemoryArenaPushUnaligned(tempMemory.arena, sizeof(*splits) * splitCount);
        StringSplit(input, seperator, &splitCount, splits);

        for (u32 splitIndex = 0; splitIndex < splitCount; splitIndex++) {
          struct string *expectedSplit = expectedSplits + splitIndex;
          struct string *split = splits + splitIndex;
          if (!IsStringEqual(split, expectedSplit)) {
            errorCode =
                expected ? TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_TRUE : TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_FALSE;

            StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
            StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  input:       "));
            StringBuilderAppendString(sb, input);
            StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  split count: "));
            StringBuilderAppendU64(sb, splitCount);
            StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  index "));
            StringBuilderAppendU64(sb, splitIndex);
            StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" is wrong"));
            StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n     expected: "));
            StringBuilderAppendPrintableString(sb, expectedSplit);
            StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n          got: "));
            StringBuilderAppendPrintableString(sb, split);
            StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
            struct string errorMessage = StringBuilderFlush(sb);
            LogMessage(&errorMessage);
          }
        }
      }
    }
  }

end:
  return (int)errorCode;
}
