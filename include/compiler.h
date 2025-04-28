#pragma once

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define swap(x, y)                                                                                                     \
  {                                                                                                                    \
    __typeof__(x) temp = x;                                                                                            \
    x = y;                                                                                                             \
    y = temp;                                                                                                          \
  }

static inline __UINT64_TYPE__
rdtsc()
{
  __UINT64_TYPE__ lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((__UINT64_TYPE__)hi << 32) | lo;
}
