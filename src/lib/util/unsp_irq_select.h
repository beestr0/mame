// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <bit>
#include <cstdint>
namespace unsp_detail {
// Match the original ascending bit0..8 scan, including its -1 result when only
// out-of-range bits are set. The caller's FIQ/IRQ gating and timing are unchanged.
inline int lowest_pending_irq(uint32_t pending) {
    const uint32_t valid = pending & 0x1ff;
    return valid ? int(std::countr_zero(valid)) : -1;
}
}
