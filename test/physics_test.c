#include "log.h"
#include "physics.c"
#include "string_builder.h"

#define TEST_ERROR_LIST(XX)                                                                                            \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_TOP_RIGHT,                                                               \
     "Finding furthest point for box volume in direction of top right failed.")                                        \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_TOP_LEFT,                                                                \
     "Finding furthest point for box volume in direction of top left failed.")                                         \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_BOTTOM_LEFT,                                                             \
     "Finding furthest point for box volume in direction of bottom left failed.")                                      \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_BOTTOM_RIGHT,                                                            \
     "Finding furthest point for box volume in direction of bottom right failed.")                                     \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_RIGHT,                                                            \
     "Finding furthest point for box volume in direction of center right failed.")                                     \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_LEFT,                                                             \
     "Finding furthest point for box volume in direction of center left failed.")                                      \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_UP,                                                               \
     "Finding furthest point for box volume in direction of center up failed.")                                        \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_DOWN,                                                             \
     "Finding furthest point for box volume in direction of center down failed.")                                      \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_TOP_RIGHT,                                                            \
     "Finding furthest point for circle volume in direction of top right failed.")                                     \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_TOP_LEFT,                                                             \
     "Finding furthest point for circle volume in direction of top left failed.")                                      \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_BOTTOM_LEFT,                                                          \
     "Finding furthest point for circle volume in direction of bottom left failed.")                                   \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_BOTTOM_RIGHT,                                                         \
     "Finding furthest point for circle volume in direction of bottom right failed.")                                  \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_RIGHT,                                                         \
     "Finding furthest point for circle volume in direction of center right failed.")                                  \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_LEFT,                                                          \
     "Finding furthest point for circle volume in direction of center left failed.")                                   \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_UP,                                                            \
     "Finding furthest point for circle volume in direction of center up failed.")                                     \
  XX(PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_DOWN,                                                          \
     "Finding furthest point for circle volume in direction of center down failed.")

enum physics_test_error {
  PHYSICS_TEST_ERROR_NONE = 0,
#define XX(name, message) name,
  TEST_ERROR_LIST(XX)
#undef XX

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

comptime struct physics_test_error_info {
  enum physics_test_error code;
  string message;
} PHYSICS_TEST_ERRORS[] = {
#define XX(name, msg) {.code = name, .message = STRING_FROM_ZERO_TERMINATED(msg)},
    TEST_ERROR_LIST(XX)
#undef XX
};

static string *
GetPhysicsTestErrorMessage(enum physics_test_error errorCode)
{
  for (u32 index = 0; index < ARRAY_COUNT(PHYSICS_TEST_ERRORS); index++) {
    const struct physics_test_error_info *info = PHYSICS_TEST_ERRORS + index;
    if (info->code == errorCode)
      return (struct string *)&info->message;
  }
  return 0;
}

static inline void
StringBuilderAppendV2(string_builder *sb, v2 value)
{
  StringBuilderAppendF32(sb, value.x, 2);
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
  StringBuilderAppendF32(sb, value.y, 2);
}

int
main(void)
{
  enum physics_test_error errorCode = PHYSICS_TEST_ERROR_NONE;

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

  // v2 FindFurthestPoint(struct entity *entity, v2 direction)
  { // Volume: Box
    __cleanup_memory_temp__ memory_temp tempMemory = MemoryTempBegin(&stackMemory);
    v2 size = V2(1.0f, 1.0f);
    v2 halfSize = v2_scale(size, 0.5f);
    v2 position = V2(1.0f, 5.0f);
    entity entity = {
        .position = position,
        .volume = VolumeBox(tempMemory.arena, size.x, size.y),
    };

    // vertex positions
    v2 topRight = v2_add(position, halfSize);
    v2 topLeft = v2_add(topRight, V2(-size.x, 0.0f));
    v2 bottomLeft = v2_add(position, v2_neg(halfSize));
    v2 bottomRight = v2_add(bottomLeft, V2(size.x, 0.0f));
    v2 centerRight = v2_add(position, V2(halfSize.x, 0.0f));
    v2 centerLeft = v2_add(position, V2(-halfSize.x, 0.0f));
    v2 centerUp = v2_add(position, V2(0.0f, halfSize.y));
    v2 centerDown = v2_add(position, V2(0.0f, -halfSize.y));

    struct test_case {
      v2 direction;
      v2 expected;
      enum physics_test_error error;
    } testCases[] = {
        {.direction = V2(1.0f, 1.0f),
         .expected = topRight,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_TOP_RIGHT},
        {.direction = V2(-1.0f, 1.0f), .expected = topLeft, .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_TOP_LEFT},
        {.direction = V2(-1.0f, -1.0f),
         .expected = bottomLeft,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_BOTTOM_LEFT},
        {.direction = V2(1.0f, -1.0f),
         .expected = bottomRight,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_BOTTOM_RIGHT},
        {.direction = V2(1.0f, 0.0f),
         .expected = centerRight,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_RIGHT},
        {.direction = V2(-1.0f, 0.0f),
         .expected = centerLeft,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_LEFT},
        {.direction = V2(0.0f, 1.0f),
         .expected = centerUp,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_UP},
        {.direction = V2(0.0f, -1.0f),
         .expected = centerDown,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_BOX_CENTER_DOWN},
    };

    v2 result;
    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      v2 expected = testCase->expected;
      v2 direction = testCase->direction;
      result = FindFurthestPoint(&entity, direction);
      if (result.x != expected.x || result.y != expected.y) {
        StringBuilderAppendString(sb, GetPhysicsTestErrorMessage(testCase->error));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  entity position: "));
        StringBuilderAppendV2(sb, position);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" size: "));
        StringBuilderAppendV2(sb, size);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  direction: "));
        StringBuilderAppendV2(sb, direction);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n   expected: "));
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n        got: "));
        StringBuilderAppendV2(sb, result);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);

        errorCode = testCase->error;
      }
    }
  }

  { // Volume: Circle
    __cleanup_memory_temp__ memory_temp tempMemory = MemoryTempBegin(&stackMemory);
    f32 radius = 5.0f;
    v2 position = V2(9.0f, 2.0f);
    entity entity = {
        .position = position,
        .volume = VolumeCircle(tempMemory.arena, radius),
    };

    // vertex positions
    v2 diagonalUnit = V2(Cos(PI / 4.0f), Sin(PI / 4.0f));
    if ((u32)(v2_length_square(diagonalUnit) + 0.5f) != 1) {
      return 5;
    }
    v2 topRight = v2_add(position, v2_hadamard(V2(radius, radius), diagonalUnit));
    v2 topLeft = v2_add(position, v2_hadamard(V2(-radius, radius), diagonalUnit));
    v2 bottomLeft = v2_add(position, v2_hadamard(V2(-radius, -radius), diagonalUnit));
    v2 bottomRight = v2_add(position, v2_hadamard(V2(radius, -radius), diagonalUnit));
    v2 centerRight = v2_add(position, V2(radius, 0.0f));
    v2 centerLeft = v2_add(position, V2(-radius, 0.0f));
    v2 centerUp = v2_add(position, V2(0.0f, radius));
    v2 centerDown = v2_add(position, V2(0.0f, -radius));

    struct test_case {
      v2 direction;
      v2 expected;
      enum physics_test_error error;
    } testCases[] = {
        {.direction = v2_normalize(V2(1.0f, 1.0f)),
         .expected = topRight,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_TOP_RIGHT},
        {.direction = v2_normalize(V2(-1.0f, 1.0f)),
         .expected = topLeft,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_TOP_LEFT},
        {.direction = v2_normalize(V2(-1.0f, -1.0f)),
         .expected = bottomLeft,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_BOTTOM_LEFT},
        {.direction = v2_normalize(V2(1.0f, -1.0f)),
         .expected = bottomRight,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_BOTTOM_RIGHT},
        {.direction = V2(1.0f, 0.0f),
         .expected = centerRight,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_RIGHT},
        {.direction = V2(-1.0f, 0.0f),
         .expected = centerLeft,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_LEFT},
        {.direction = V2(0.0f, 1.0f),
         .expected = centerUp,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_UP},
        {.direction = V2(0.0f, -1.0f),
         .expected = centerDown,
         .error = PHYSICS_TEST_ERROR_FINDFURTHESTPOINT_CIRCLE_CENTER_DOWN},
    };

    v2 result;
    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      v2 expected = testCase->expected;
      v2 direction = testCase->direction;
      result = FindFurthestPoint(&entity, direction);
      if (result.x != expected.x || result.y != expected.y) {
        StringBuilderAppendString(sb, GetPhysicsTestErrorMessage(testCase->error));
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  entity position: "));
        StringBuilderAppendV2(sb, position);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" radius: "));
        StringBuilderAppendF32(sb, radius, 2);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  direction: "));
        StringBuilderAppendV2(sb, direction);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n   expected: "));
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n        got: "));
        StringBuilderAppendV2(sb, result);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        string errorMessage = StringBuilderFlush(sb);
        LogMessage(&errorMessage);

        errorCode = testCase->error;
      }
    }
  }

  return (int)errorCode;
}
