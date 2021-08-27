
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cerrno>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

template<typename T, size_t N>
inline constexpr size_t countof(T (&_)[N]) {
    return N;
}

#include <nn_Includes.hpp>