// license:BSD-3-Clause
// Heterogeneous lookup for standard libraries predating C++20 unordered lookup.
#pragma once
#include <algorithm>
#include <unordered_set>

namespace util {
template <typename Container, typename Key>
auto unordered_find(Container &items, Key const &key)
{
#if defined(__cpp_lib_generic_unordered_lookup) && __cpp_lib_generic_unordered_lookup >= 201811L
    return items.find(key);
#else
    // No temporary key object: output items own notifier state and references.
    // The fallback is linear; modern standard libraries retain hashed lookup.
    auto const equal = items.key_eq();
    return std::find_if(items.begin(), items.end(), [&](auto const &item) { return equal(item, key); });
#endif
}
} // namespace util
