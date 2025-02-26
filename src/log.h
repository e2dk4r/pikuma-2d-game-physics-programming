#pragma once

#include "text.h"
static void
LogMessage(struct string *message);

#if IS_PLATFORM_LINUX
#include <unistd.h> // write()

static inline void
LogMessage(struct string *message)
{
  write(STDOUT_FILENO, message->value, message->length);
}

#elif IS_PLATFORM_WINDOWS

#include <windows.h>

static inline void
LogMessage(struct string *message)
{
  HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  WriteFile(outputHandle, message->value, (u32)message->length, 0, 0);
}

#else
#error "Log not implemented for this platform"
#endif
