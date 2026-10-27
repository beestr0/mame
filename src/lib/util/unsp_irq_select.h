// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <bit>
#include <cstdint>
#if !defined(PS4_OPT_IRQ) && defined(MAME_OPENORBIS) && __has_include("ps4_irq_config.h")
#include "ps4_irq_config.h"
#endif
#ifndef PS4_OPT_IRQ
#define PS4_OPT_IRQ 1
#endif
namespace unsp_detail {
// Match the original ascending bit0..8 scan, including its -1 result when only
// out-of-range bits are set. The caller's FIQ/IRQ gating and timing are unchanged.
inline int lowest_pending_irq(uint32_t pending) {
#if PS4_OPT_IRQ
    const uint32_t valid = pending & 0x1ff;
    return valid ? int(std::countr_zero(valid)) : -1;
#else
    int selected=-1;
    for(int i=0;i<=8;++i) if((pending>>i)&1) {selected=i;break;}
    return selected;
#endif
}
}
