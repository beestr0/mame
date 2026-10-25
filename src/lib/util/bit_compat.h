// license:BSD-3-Clause
// Stable names for C++20 bit operations missing/renamed in libc++ 11.
#pragma once
#include <type_traits>
namespace util {
template <typename Unsigned>
constexpr unsigned bit_width(Unsigned value) noexcept
{
    static_assert(std::is_unsigned_v<Unsigned>, "bit_width requires an unsigned integer");
    unsigned bits = 0;
    while (value) { ++bits; value >>= 1; }
    return bits;
}
template <typename Unsigned>
constexpr bool has_single_bit(Unsigned value) noexcept
{
    static_assert(std::is_unsigned_v<Unsigned>, "has_single_bit requires an unsigned integer");
    return value && !(value & (value - 1));
}
} // namespace util
