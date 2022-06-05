#ifndef FL_COMMON_H__
#define FL_COMMON_H__

#include <arm_math.h>

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef          char      i8;
typedef          int       i32;
typedef          float32_t f32;

#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

#define Maximum(A, B) ((A) < (B) ? (B) : (A))
#define Minimum(A, B) ((A) < (B) ? (A) : (B))

#define internal static

internal inline f32 Square(f32 A) {
   return A*A;
}

internal inline f32 Abs(f32 A) {
   return A >= 0 ? A : -A;
}

internal inline f32 Clamp(f32 Min, f32 A, f32 Max) {
   return Maximum(Minimum(A, Max), Min);
}

internal inline u32 ClampU(u32 Min, u32 A, u32 Max) {
   return Maximum(Minimum(A, Max), Min);
}

internal inline i32 Round(f32 A) {
   return (i32)(A + 0.5f);
}

internal inline f32 Lerp(f32 A, f32 t, f32 B) {
   return (1 - t) * A + t * B;
}

internal inline f32 Sine(f32 V) {
   return arm_sin_f32(V);
}

#endif
