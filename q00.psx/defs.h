#pragma once
#include <stdint.h>
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;
typedef u32 word;
typedef u16 hword;
typedef u8 byte;
#define SIGN_EXT32(a) (int32_t)(int16_t)a
