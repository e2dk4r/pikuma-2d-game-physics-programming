#pragma once

#if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
#define __ASSERT__ __builtin_debugtrap()
#elif defined(_MSC_VER)
#define __ASSERT__ __debugbreak()
#else
#define __ASSERT__ __asm__("int3; nop")
#endif

#if IS_BUILD_DEBUG

#define debug_assert(expression)                                                                                       \
  if (!(expression)) {                                                                                                 \
    __ASSERT__;                                                                                                        \
  }

#define breakpoint(...) __ASSERT__

#else

#define debug_assert(expression)
#define breakpoint()

#endif

#define runtime_assert(x)                                                                                              \
  if (!(x)) {                                                                                                          \
    __ASSERT__;                                                                                                        \
  }

#define static_assert(expression)                                                                                      \
  do {                                                                                                                 \
    enum { STATIC_ASSERT__ = 1 / (expression) };                                                                       \
  } while (0)
