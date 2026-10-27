// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <cstdint>
namespace spg_detail {
// 5-bit red and blue lanes are separated by10 bits. With alpha0..32,
// either lane's weighted sum is <=31*32=992<1024: no carry between lanes.
// Green is evaluated separately. This retains each original integer floor.
inline uint16_t blend_rgb555(uint16_t bottom,uint16_t top,uint8_t alpha) {
    const uint32_t inverse=32-alpha;
    const uint32_t rb=((bottom&0x7c1f)*inverse+(top&0x7c1f)*alpha)>>5;
    const uint32_t g=((bottom&0x03e0)*inverse+(top&0x03e0)*alpha)>>5;
    return uint16_t((rb&0x7c1f)|(g&0x03e0));
}
}
