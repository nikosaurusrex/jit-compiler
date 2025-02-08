#pragma once

#include <stdint.h>
#include <float.h>

#include "PlatformContext.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int8_t b8;
typedef int32_t b32;

typedef unsigned int uint;

#define global static
#define internal static

#define readonly const

#if COMPILER_MSVC
#define perthread __declspec(thread)
#elif COMPILER_CLANG
#define perthread __thread
#elif COMPILER_GCC
#define perthread __thread
#endif

#if COMPILER_MSVC
#define alignby(n) __declspec(align(n))
#elif COMPILER_CLANG
#define alignby(n) __attribute((aligned(n)))
#elif COMPILER_GCC
#define alignby(n) __attribute((aligned(n)))
#endif

#define KiloBytes(n) (((u64) (n)) << 10)
#define MegaBytes(n) (((u64) (n)) << 20)
#define GigaBytes(n) (((u64) (n)) << 30)
#define TeraBytes(n) (((u64) (n)) << 40)

#define Min(A, B) (((A) < (B)) ? (A) : (B))
#define Max(A, B) (((A) > (B)) ? (A) : (B))
#define ClampTop(A, X) Min(A, X)
#define ClampBot(X, B) Max(X, B)
#define Clamp(X, A, B) (((X) < (A)) ? (A) : ((X) > (B)) ? (B) : (X))

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MemberSize(type, member) (sizeof(((type *)0)->member))
#define ElementSize(array) (sizeof(array[0]))

#if COMPILER_CLANG || COMPILER_GCC
#define STATIC_EXPECT(expr, val) __builtin_expect((expr), (val))
#else
#define STATIC_EXPECT(expr, val) (expr)
#endif

#define NK_LIKELY(expr) STATIC_EXPECT(expr, 1)
#define NK_UNLIKELY(expr) STATIC_EXPECT(expr, 0)

#if COMPILER_MSVC
#define NK_TRAP() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
#define NK_TRAP() __builtin_trap()
#else
#error Unsupported Compiler.
#endif

#define NK_STRING_JOIN(arg1, arg2) NK_STRING_JOIN_DELAY(arg1, arg2)
#define NK_STRING_JOIN_DELAY(arg1, arg2) NK_STRING_JOIN_IMMEDIATE(arg1, arg2)
#define NK_STRING_JOIN_IMMEDIATE(arg1, arg2) arg1##arg2

#ifdef COMPILER_MSVC
#define NK_LINENUMBER(name) NK_STRING_JOIN(name, __COUNTER__)
#else
#define NK_LINENUMBER(name) NK_STRING_JOIN(name, __LINE__)
#endif

#define StaticAssert(cond) typedef char NK_LINENUMBER(_static_assert_var)[(cond) ? 1 : -1]

#define if_likely(cond) if (NK_LIKELY(cond))
#define if_unlikely(cond) if (NK_UNLIKELY(cond))

#if BUILD_DEBUG
#define Assert(x)                                                                                  \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            NK_TRAP();                                                                             \
        }                                                                                          \
    } while (0)
#else
#define Assert(x) ((void) 0)
#endif

#if COMPILER_MSVC
#define nkinline __forceinline
#elif COMPILER_CLANG || COMPILER_GCC
#define nkinline __attribute__((always_inline))
#else
#define nkinline
#endif

#define NotImplemented Assert("Not Implemented!");

#define AlignPow2(x, b) (((x) + (b) - 1) & (~((b) - 1)))
#define IsPow2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#define ASAN_POISON_MEMORY_REGION(addr, size) __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) ((void) (addr), (void) (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void) (addr), (void) (size))
#endif

#if COMPILER_CLANG
#define IGNORE_WARNINGS_BEGIN _Pragma("clang diagnostic push") \
                              _Pragma("clang diagnostic ignored \"-Weverything\"")
#define IGNORE_WARNINGS_END   _Pragma("clang diagnostic pop")
#else
#define IGNORE_WARNINGS_BEGIN
#define IGNORE_WARNINGS_END
#endif

readonly global float PI32 = 3.1415926535897f;

readonly global u64 max_u64 = 0xffffffffffffffffull;
readonly global u32 max_u32 = 0xffffffff;
readonly global u16 max_u16 = 0xffff;
readonly global u8 max_u8 = 0xff;

readonly global s64 max_s64 = (s64) 0x7fffffffffffffffull;
readonly global s32 max_s32 = (s32) 0x7fffffff;
readonly global s16 max_s16 = (s16) 0x7fff;
readonly global s8 max_s8 = (s8) 0x7f;

readonly global s64 min_s64 = (s64) 0xffffffffffffffffull;
readonly global s32 min_s32 = (s32) 0xffffffff;
readonly global s16 min_s16 = (s16) 0xffff;
readonly global s8 min_s8 = (s8) 0xff;

readonly global float float_min = FLT_MAX;
readonly global float float_max = FLT_MIN;
