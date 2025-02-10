#include "string_builder.h"

// TODO: Show error pretty error message when a test fails
enum string_builder_test_error {
  STRING_BUILDER_TEST_ERROR_NONE = 0,
  STRING_BUILDER_TEST_ERROR_APPENDZEROTERMINATED,
  STRING_BUILDER_TEST_ERROR_APPENDSTRING,
  STRING_BUILDER_TEST_ERROR_APPENDU64,
  STRING_BUILDER_TEST_ERROR_APPENDHEX,
  STRING_BUILDER_TEST_ERROR_APPENDF32,
  STRING_BUILDER_TEST_ERROR_FLUSH,

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
  enum string_builder_test_error errorCode = STRING_BUILDER_TEST_ERROR_NONE;

  // setup
  u8 outBufferBytes[256];
  string *outBuffer = &(string){
      .value = outBufferBytes,
      .length = sizeof(outBufferBytes),
  };
  u8 stringBufferBytes[32];
  string *stringBuffer = &(string){
      .value = stringBufferBytes,
      .length = sizeof(stringBufferBytes),
  };
  string_builder *sb = &(string_builder){
      .outBuffer = outBuffer,
      .stringBuffer = stringBuffer,
  };

  // StringBuilderAppendZeroTerminated(string_builder *stringBuilder, const char *src, u64 max)
  {
    StringBuilderAppendZeroTerminated(sb, "abc", 3);

    string *expected = &STRING_FROM_ZERO_TERMINATED("abc");
    if (!IsStringStartsWith(outBuffer, expected)) {
      errorCode = STRING_BUILDER_TEST_ERROR_APPENDZEROTERMINATED;
      goto end;
    }
  }
  StringBuilderFlush(sb);

  // StringBuilderAppendString(string_builder *stringBuilder, struct string *string)
  {
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("ZYXTS"));

    string *expected = &STRING_FROM_ZERO_TERMINATED("ZYXTS");
    if (!IsStringStartsWith(outBuffer, expected)) {
      errorCode = STRING_BUILDER_TEST_ERROR_APPENDSTRING;
      goto end;
    }
  }
  StringBuilderFlush(sb);

  // StringBuilderAppendU64(string_builder *stringBuilder, u64 value)
  {
    StringBuilderAppendU64(sb, 5439);

    string *expected = &STRING_FROM_ZERO_TERMINATED("5439");
    if (!IsStringStartsWith(outBuffer, expected)) {
      errorCode = STRING_BUILDER_TEST_ERROR_APPENDU64;
      goto end;
    }
  }
  StringBuilderFlush(sb);

  // StringBuilderAppendHex(string_builder *stringBuilder, u64 value)
  {
    StringBuilderAppendHex(sb, 252457880);

    string *expected = &STRING_FROM_ZERO_TERMINATED("0x0f0c3398");
    if (!IsStringStartsWith(outBuffer, expected)) {
      errorCode = STRING_BUILDER_TEST_ERROR_APPENDHEX;
      goto end;
    }
  }
  StringBuilderFlush(sb);

  // StringBuilderAppendF32(string_builder *stringBuilder, f32 value, u32 fractionCount)
  {
    StringBuilderAppendF32(sb, 4.31f, 2);

    string *expected = &STRING_FROM_ZERO_TERMINATED("4.31");
    if (!IsStringStartsWith(outBuffer, expected)) {
      errorCode = STRING_BUILDER_TEST_ERROR_APPENDF32;
      goto end;
    }
  }
  StringBuilderFlush(sb);

  // StringBuilderFlush(string_builder *stringBuilder)
  {
    StringBuilderAppendZeroTerminated(sb, "abc", 3);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" "));
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("ZYXTS"));
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" "));
    StringBuilderAppendU64(sb, 5439);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" "));
    StringBuilderAppendHex(sb, 252457880);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" "));
    StringBuilderAppendF32(sb, 4.31f, 2);

    string value = StringBuilderFlush(sb);
    string *expected = &STRING_FROM_ZERO_TERMINATED("abc ZYXTS 5439 0x0f0c3398 4.31");
    if (!IsStringEqual(&value, expected)) {
      errorCode = STRING_BUILDER_TEST_ERROR_APPENDHEX;
      goto end;
    }
  }
  StringBuilderFlush(sb);

end:
  return (int)errorCode;
}
