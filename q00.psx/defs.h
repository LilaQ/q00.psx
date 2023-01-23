#pragma once
#include <stdint.h>
typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;
typedef u64 dword;
typedef i64 diword;
typedef u32 word;
typedef u16 hword;
typedef u8 byte;
#define SIGN_EXT8_TO_32(a) (int32_t)(int8_t)a
#define SIGN_EXT16_TO_32(a) (int32_t)(int16_t)a
#define SIGN_EXT16(a) (int16_t)(int8_t)a
#define SIGN_EXT32(a) (int32_t)(int16_t)a
#define SIGN_EXT64(a) (int64_t)(int32_t)a

#define SIGN_EXT_BYTE_TO_WORD(a) SIGN_EXT8_TO_32(a)
#define SIGN_EXT_HWORD_TO_WORD(a) SIGN_EXT16_TO_32(a)