// license:BSD-3-Clause
#include "util/unordered_lookup.h"
#include "util/bit_compat.h"
static_assert(util::bit_width(0U) == 0);
static_assert(util::bit_width(1U) == 1);
static_assert(util::bit_width(0xffffffffU) == 32);
static_assert(util::bit_width(1ULL << 63) == 64);
static_assert(!util::has_single_bit(0U));
static_assert(util::has_single_bit(1ULL << 63));
static_assert(!util::has_single_bit(3U));
#include <cassert>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_set>
struct Item { std::string name; int notifications = 0; };
struct Hash {
    using is_transparent = void;
    size_t operator()(const Item &i) const { return std::hash<std::string_view>()(i.name); }
    size_t operator()(std::string_view s) const { return std::hash<std::string_view>()(s); }
};
struct Equal {
    using is_transparent = void;
    bool operator()(const Item &a, const Item &b) const { return a.name == b.name; }
    bool operator()(const Item &a, std::string_view b) const { return a.name == b; }
    bool operator()(std::string_view a, const Item &b) const { return a == b.name; }
};
int main() {
    std::unordered_set<Item, Hash, Equal> items;
    items.insert({"lamp", 42});
    auto found = util::unordered_find(items, std::string_view("lamp"));
    assert(found != items.end() && found->notifications == 42);
    auto const *identity = &*found;
    assert(&*util::unordered_find(items, std::string_view("lamp")) == identity);
    assert(util::unordered_find(items, std::string_view("missing")) == items.end());
    const auto &constant = items;
    assert(util::unordered_find(constant, std::string_view("lamp")) != constant.end());
    std::unordered_set<std::reference_wrapper<const Item>, Hash, Equal> references;
    references.emplace(*identity);
    assert(&util::unordered_find(references, std::string_view("lamp"))->get() == identity);
}
