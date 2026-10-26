// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "ps4_profile_abi.h"
namespace ps4_profile {
#if defined(MAME_OPENORBIS) || defined(PS4_PROFILE_TEST)
// Main emulation thread only; no allocation, I/O, locks or instruction hooks.
struct state {
    uint64_t (*clock)() = nullptr;
    ps4_core_profile result{};
    uint64_t last = 0;
    unsigned category = PS4_PROFILE_OTHER;
    void account() {
        const uint64_t now = clock(); ++result.clock_reads;
        if (now < last) ++result.invalid;
        else result.ticks[category] += now - last;
        last = now;
    }
};
inline state data;
class scope {
    bool active = false;
    unsigned previous = 0;
public:
    explicit scope(unsigned category) {
        if (data.clock) {
            active = true; previous = data.category; data.account();
            data.category = category; ++data.result.calls[category];
        }
    }
    scope(const scope &) = delete;
    ~scope() { if (active && data.clock) { data.account(); data.category = previous; } }
};
inline int begin(uint64_t (*clock)()) {
    if (!clock || data.clock) return -1;
    data.result = {}; data.result.version = 1; data.result.size = sizeof(ps4_core_profile);
    data.category = PS4_PROFILE_OTHER; data.clock = clock;
    data.last = clock(); ++data.result.clock_reads;
    return 0;
}
inline int end(ps4_core_profile *out, uint32_t size) {
    if (!data.clock) return -1;
    data.account(); data.clock = nullptr;
    if (!out || size != sizeof(ps4_core_profile)) return -2;
    *out = data.result;
    return out->invalid ? -3 : 0;
}
#else
struct scope { explicit scope(unsigned) {} };
#endif
}
