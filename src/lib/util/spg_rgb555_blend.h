// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <cstdint>
#if !defined(PS4_OPT_BLEND) && defined(MAME_OPENORBIS) && __has_include("ps4_blend_config.h")
#include "ps4_blend_config.h"
#endif
#ifndef PS4_OPT_BLEND
#define PS4_OPT_BLEND 1
#endif
namespace spg_detail {
// 5-bit red and blue lanes are separated by10 bits. With alpha0..32,
// either lane's weighted sum is <=31*32=992<1024: no carry between lanes.
// Green is evaluated separately. This retains each original integer floor.
inline uint16_t blend_rgb555(uint16_t bottom,uint16_t top,uint8_t alpha) {
#if PS4_OPT_BLEND
    const uint32_t inverse=32-alpha;
    const uint32_t rb=((bottom&0x7c1f)*inverse+(top&0x7c1f)*alpha)>>5;
    const uint32_t g=((bottom&0x03e0)*inverse+(top&0x03e0)*alpha)>>5;
    return uint16_t((rb&0x7c1f)|(g&0x03e0));
#else
    auto channel=[alpha](unsigned b,unsigned t) {return ((32-alpha)*b+alpha*t)>>5;};
    return uint16_t((channel((bottom>>10)&31,(top>>10)&31)<<10)|
                    (channel((bottom>>5)&31,(top>>5)&31)<<5)|channel(bottom&31,top&31));
#endif
}
}
