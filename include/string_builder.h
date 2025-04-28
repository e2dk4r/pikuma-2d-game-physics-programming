#pragma once

#include "memory.h"
#include "string_cursor.h"
#include "teju.h"
#include "text.h"

struct string_builder {
  // Output buffer. All appended things stored in here.
  // REQUIRED
  struct string *outBuffer;
  // Used for converting u64, f32.
  // If you are appending only strings you can omit this.
  // OPTIONAL
  struct string *stringBuffer;
  // Length of output buffer.
  u64 length;
};

typedef struct string_builder string_builder;

static inline void
StringBuilderClear(struct string_builder *stringBuilder)
{
  stringBuilder->length = 0;
}

static string_builder *
MakeStringBuilder(memory_arena *arena, u64 outBufferLength, u64 stringBufferLength)
{
  debug_assert(outBufferLength > 0);
  string_builder *sb = MemoryArenaPush(arena, sizeof(*sb));

  string *outBuffer = MemoryArenaPush(arena, sizeof(*outBuffer));
  outBuffer->length = outBufferLength;
  outBuffer->value = MemoryArenaPush(arena, sizeof(u8) * outBuffer->length);
  sb->outBuffer = outBuffer;

  if (stringBufferLength == 0) {
    // do not need to convert any variable, all strings
    sb->stringBuffer = 0;
  } else {
    string *stringBuffer = MemoryArenaPush(arena, sizeof(*stringBuffer));
    stringBuffer->length = stringBufferLength;
    stringBuffer->value = MemoryArenaPush(arena, sizeof(u8) * stringBuffer->length);
    sb->stringBuffer = stringBuffer;
  }

  StringBuilderClear(sb);

  return sb;
}

static inline void
StringBuilderAppendZeroTerminated(string_builder *stringBuilder, const char *src, u64 max)
{
  struct string *outBuffer = stringBuilder->outBuffer;
  struct string string = StringFromZeroTerminated((u8 *)(u64)src, max);
  memcpy(outBuffer->value + stringBuilder->length, string.value, string.length);
  stringBuilder->length += string.length;
  debug_assert(stringBuilder->length <= outBuffer->length);
}

static inline void
StringBuilderAppendString(string_builder *stringBuilder, struct string *string)
{
  struct string *outBuffer = stringBuilder->outBuffer;
  memcpy(outBuffer->value + stringBuilder->length, string->value, string->length);
  stringBuilder->length += string->length;
  debug_assert(stringBuilder->length <= outBuffer->length);
}

#define StringBuilderAppendStringLiteral(sb, s) StringBuilderAppendString(sb, &StringFromLiteral(s))

static inline void
StringBuilderAppendU64(string_builder *stringBuilder, u64 value)
{
  struct string *outBuffer = stringBuilder->outBuffer;
  struct string *stringBuffer = stringBuilder->stringBuffer;

  struct string string = FormatU64(stringBuffer, value);
  memcpy(outBuffer->value + stringBuilder->length, string.value, string.length);
  stringBuilder->length += string.length;
  debug_assert(stringBuilder->length <= outBuffer->length);
}

static inline void
StringBuilderAppendU8(string_builder *stringBuilder, u8 value)
{
  StringBuilderAppendU64(stringBuilder, (u64)value);
}

static inline void
StringBuilderAppendU16(string_builder *stringBuilder, u16 value)
{
  StringBuilderAppendU64(stringBuilder, (u64)value);
}

static inline void
StringBuilderAppendU32(string_builder *stringBuilder, u32 value)
{
  StringBuilderAppendU64(stringBuilder, (u64)value);
}

static inline void
StringBuilderAppendS64(string_builder *stringBuilder, s64 value)
{
  if (value < 0)
    StringBuilderAppendStringLiteral(stringBuilder, "-");
  StringBuilderAppendU64(stringBuilder, (u64)value);
}

static inline void
StringBuilderAppendS8(string_builder *stringBuilder, s8 value)
{
  StringBuilderAppendS64(stringBuilder, (s64)value);
}

static inline void
StringBuilderAppendS16(string_builder *stringBuilder, s16 value)
{
  StringBuilderAppendS64(stringBuilder, (s64)value);
}

static inline void
StringBuilderAppendS32(string_builder *stringBuilder, s32 value)
{
  StringBuilderAppendS64(stringBuilder, (s64)value);
}

static inline void
StringBuilderAppendHex(string_builder *stringBuilder, u64 value)
{
  struct string *outBuffer = stringBuilder->outBuffer;
  struct string *stringBuffer = stringBuilder->stringBuffer;

  struct string string = FormatHex(stringBuffer, value);
  memcpy(outBuffer->value + stringBuilder->length, string.value, string.length);
  stringBuilder->length += string.length;
  debug_assert(stringBuilder->length <= outBuffer->length);
}

static inline void
StringBuilderAppendF32(string_builder *stringBuilder, f32 value, u32 fractionCount)
{
  struct string *outBuffer = stringBuilder->outBuffer;
  struct string *stringBuffer = stringBuilder->stringBuffer;

  struct string string = FormatF32(stringBuffer, value, fractionCount);
  memcpy(outBuffer->value + stringBuilder->length, string.value, string.length);
  stringBuilder->length += string.length;
  debug_assert(stringBuilder->length <= outBuffer->length);
}

static void
StringBuilderAppendHexDump(string_builder *sb, struct string *string)
{
  struct string_cursor cursor = StringCursorFromString(string);
  u8 offsetBuffer[8];
  struct string offsetBufferString = StringFromBuffer(offsetBuffer, ARRAY_COUNT(offsetBuffer));
  u8 hexBuffer[2];
  struct string hexBufferString = StringFromBuffer(hexBuffer, ARRAY_COUNT(hexBuffer));

  while (!IsStringCursorAtEnd(&cursor)) {
    if (cursor.position == 0) {
      struct string header = StringFromLiteral("          0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f\n");
      StringBuilderAppendString(sb, &header);
    }

    // offset
    struct string offsetText = FormatHex(&offsetBufferString, cursor.position);
    for (u32 offsetTextPrefixIndex = 0; offsetTextPrefixIndex < offsetBufferString.length - offsetText.length;
         offsetTextPrefixIndex++) {
      // offset length must be 8, so fill prefix with zeros
      StringBuilderAppendStringLiteral(sb, "0");
    }
    StringBuilderAppendString(sb, &offsetText);

    StringBuilderAppendStringLiteral(sb, " ");

    // hex
    u64 width = 16;
    struct string substring = StringCursorConsumeSubstring(&cursor, width);
    for (u64 substringIndex = 0; substringIndex < substring.length; substringIndex++) {
      u8 character = *(substring.value + substringIndex);
      struct string hexText = FormatHex(&hexBufferString, (u64)character);
      debug_assert(hexText.length == 2);
      StringBuilderAppendString(sb, &hexText);

      StringBuilderAppendStringLiteral(sb, " ");

      if (substringIndex + 1 == 8)
        StringBuilderAppendStringLiteral(sb, " ");
    }

    for (u64 index = 0; index < width - substring.length; index++) {
      // align ascii to right
      StringBuilderAppendStringLiteral(sb, "   ");
      if (index + substring.length + 1 == 8)
        StringBuilderAppendStringLiteral(sb, " ");
    }

    // ascii input
    StringBuilderAppendStringLiteral(sb, "|");
    for (u64 substringIndex = 0; substringIndex < substring.length; substringIndex++) {
      u8 character = *(substring.value + substringIndex);
      b8 disallowed[U8_MAX] = {
          // [0x00, 0x1a]
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x00
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // 0x10
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x20
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x30
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x40
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x50
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x60
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x70
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x80
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x90
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xa0
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xb0
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xc0
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xd0
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xe0
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 0xf0
      };
      if (disallowed[character]) {
        StringBuilderAppendStringLiteral(sb, ".");
      } else {
        struct string characterString = StringFromBuffer(&character, 1);
        StringBuilderAppendString(sb, &characterString);
      }
    }
    StringBuilderAppendStringLiteral(sb, "|");

    if (!IsStringCursorAtEnd(&cursor))
      StringBuilderAppendStringLiteral(sb, "\n");
  }
}

/*
 * Returns string that is ready for transmit.
 * Also resets length of builder.
 *
 * @code
 *   StringBuilderAppend..(stringBuilder, x);
 *   struct string string = StringBuilderFlush(stringBuilder);
 *   write(x, string.value, string.length);
 * @endcode
 */
static inline struct string
StringBuilderFlush(string_builder *stringBuilder)
{
  debug_assert(stringBuilder->length != 0);
  struct string result = StringSlice(stringBuilder->outBuffer, 0, stringBuilder->length);
  StringBuilderClear(stringBuilder);
  return result;
}

static inline struct string
StringBuilderFlushZeroTerminated(string_builder *stringBuilder)
{
  struct string result = StringBuilderFlush(stringBuilder);
  result.value[result.length] = 0;
  debug_assert(result.length + 1 <= stringBuilder->outBuffer->length);
  return result;
}
