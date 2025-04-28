#pragma once

#include "text.h"

static void
PrintString(struct string *string);

#if IS_PLATFORM_WINDOWS

#include <windows.h>

static inline void
PrintString(struct string *string)
{
  HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  (void)WriteFile(outputHandle, string->value, (u32)string->length, 0, 0);
}

#else

// https://www.man7.org/linux/man-pages/man2/write.2.html#SYNOPSIS
// https://man.freebsd.org/cgi/man.cgi?query=write&sektion=2&apropos=0&manpath=FreeBSD+14.2-RELEASE+and+Ports#SYNOPSIS
// https://man.openbsd.org/man2/write.2#SYNOPSIS
#include <unistd.h> // write()

static inline void
PrintString(struct string *string)
{
  (void)write(STDOUT_FILENO, string->value, string->length);
}

#endif
