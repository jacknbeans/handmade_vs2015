#pragma once

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32_t bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#define ArrayCount(a_Array) (sizeof(a_Array) / sizeof((a_Array)[0]))

#define Kilobytes(a_Value) ((a_Value) * 1024)
#define Megabytes(a_Value) (Kilobytes(a_Value) * 1024)
#define Gigabytes(a_Value) (Megabytes(a_Value) * 1024)
#define Terabytes(a_Value) (Gigabytes(a_Value) * 1024)

/*
 * HANDMADE_INTERNAL:
 *     0 - Build for public release
 *     1 - Build for developer only
 *     
 * HANDMADE_SLOW:
 *     0 - No slow code allowed!
 *     1 - Slow code welcome.
 */


#if HANDMADE_SLOW
#define Assert(a_Expression) if (!(a_Expression)) { *(int *)0 = 0;}
#else
#define Assert(a_Expression)
#endif
