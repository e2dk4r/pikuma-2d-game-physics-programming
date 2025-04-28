#pragma once

/*
 * Functions that have names with 'Extract' or 'Peek' or 'Is' in them, do NOT advance cursor position.
 */

#include "text.h"

struct string_cursor {
  struct string *source;
  u64 position;
};

typedef struct string_cursor string_cursor;

internalfn struct string_cursor
StringCursorFromString(struct string *string)
{
  return (struct string_cursor){
      .source = string,
      .position = 0,
  };
}

internalfn u64
StringCursorRemainingLength(struct string_cursor *cursor)
{
  return cursor->source->length - cursor->position;
}

internalfn struct string
StringCursorExtractSubstring(struct string_cursor *cursor, u64 length)
{
  if (length > StringCursorRemainingLength(cursor))
    length = StringCursorRemainingLength(cursor);
  struct string substring = StringFromBuffer(cursor->source->value + cursor->position, length);
  return substring;
}

internalfn struct string
StringCursorExtractConsumed(struct string_cursor *cursor)
{
  struct string substring = {.value = 0, .length = 0};
  if (cursor->position == 0)
    return substring;
  substring = StringFromBuffer(cursor->source->value, cursor->position);
  return substring;
}

internalfn struct string
StringCursorExtractRemaining(struct string_cursor *cursor)
{
  return StringCursorExtractSubstring(cursor, StringCursorRemainingLength(cursor));
}

internalfn struct string
StringCursorConsumeSubstring(struct string_cursor *cursor, u64 length)
{
  struct string substring = StringCursorExtractSubstring(cursor, length);
  cursor->position += substring.length;
  return substring;
}

internalfn b8
IsStringCursorAtEnd(struct string_cursor *cursor)
{
  return cursor->position == cursor->source->length;
}

internalfn b8
StringCursorPeekStartsWith(struct string_cursor *cursor, struct string *prefix)
{
  if (prefix->length > StringCursorRemainingLength(cursor))
    return 0;
  struct string substring = StringCursorExtractSubstring(cursor, prefix->length);
  return IsStringStartsWith(&substring, prefix);
}

internalfn b8
IsStringCursorStartsWith(struct string_cursor *cursor, struct string *prefix)
{
  b8 result = StringCursorPeekStartsWith(cursor, prefix);
  if (prefix->length > StringCursorRemainingLength(cursor))
    cursor->position += StringCursorRemainingLength(cursor);
  else
    cursor->position += prefix->length;
  return result;
}

internalfn b8
StringCursorAdvanceAfter(struct string_cursor *cursor, struct string *search)
{
  struct string remaining = StringCursorExtractRemaining(cursor);

  u64 index = 0;
  while (index < remaining.length) {
    struct string substring = StringFromBuffer(remaining.value + index, search->length);
    if (search->length > remaining.length - index) {
      cursor->position += remaining.length;
      return 0;
    }

    if (IsStringEqual(&substring, search)) {
      index += search->length;
      break;
    }

    index++;
  }

  cursor->position += index;
  return 1;
}

internalfn b8
IsStringCursorRemainingEqual(struct string_cursor *cursor, struct string *search)
{
  if (cursor->position + search->length > cursor->source->length)
    return 0;

  struct string remaining = StringCursorExtractRemaining(cursor);
  return IsStringEqual(&remaining, search);
}

internalfn struct string
StringCursorExtractUntil(struct string_cursor *cursor, struct string *search)
{
  struct string result = {.value = 0, .length = 0};
  struct string remaining = StringCursorExtractRemaining(cursor);

  u64 index = 0;
  while (index < remaining.length) {
    struct string substring = StringFromBuffer(remaining.value + index, search->length);
    if (search->length > remaining.length - index) {
      index = remaining.length;
      break;
    }

    if (IsStringEqual(&substring, search))
      break;

    index++;
  }

  if (index == 0)
    return result;

  result.value = remaining.value;
  result.length = index;
  return result;
}

internalfn struct string
StringCursorConsumeUntil(struct string_cursor *cursor, struct string *search)
{
  struct string result = StringCursorExtractUntil(cursor, search);
  cursor->position += result.length;
  return result;
}

internalfn struct string
StringCursorExtractThrough(struct string_cursor *cursor, struct string *search)
{
  struct string result = {.value = 0, .length = 0};
  struct string remaining = StringCursorExtractRemaining(cursor);

  u64 index = 0;
  while (index < remaining.length) {
    struct string substring = StringFromBuffer(remaining.value + index, search->length);
    if (search->length > remaining.length - index)
      return result;

    if (IsStringEqual(&substring, search))
      break;

    index++;
  }

  if (index == remaining.length)
    index = remaining.length - 1;

  result.value = remaining.value;
  result.length = index + search->length;
  return result;

  return result;
}

internalfn struct string
StringCursorExtractNumber(struct string_cursor *cursor)
{
  struct string result = {.value = 0, .length = 0};
  struct string remaining = StringCursorExtractRemaining(cursor);

  b8 isFloat = 0;

  u64 count = 0;
  for (u64 index = 0; index < remaining.length; index++) {
    comptime b8 allowed[U8_MAX] = {
        // [0x30, 0x39] or ['0', '9']
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x20
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 0x30
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

    u8 character = remaining.value[index];
    if (!(allowed[character] || (index == 0 && character == '-') || (!isFloat && character == '.')))
      break;

    if (character == '.')
      isFloat = 1;

    count++;
  }

  if (count == 0)
    return result;

  result = remaining;
  result.length = count;
  return result;
}
