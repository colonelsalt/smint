#pragma once

#include <cstdint>

typedef uint8_t b8;
typedef int8_t s8;
typedef uint8_t u8;
typedef float f32;
typedef double f64;
typedef uint32_t u32;
typedef int32_t s32;
typedef int32_t b32;
typedef int64_t s64;
typedef uint64_t u64;

#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))

#if _WIN32
#define Assert(X) {if (!(X)) __debugbreak();}
#else
#define Assert(X) {if (!(X)) *(int*)0 = 42;}
#endif