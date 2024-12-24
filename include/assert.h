#pragma once

#if IS_BUILD_DEBUG

#if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
#define __ASSERT__ __builtin_debugtrap()
#elif defined(_MSC_VER)
#define __ASSERT__ __debugbreak()
#else
#define __ASSERT__ __asm__("int3; nop")
#endif

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
