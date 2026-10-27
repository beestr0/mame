// SPDX-License-Identifier: BSD-3-Clause
// Minimal synthetic bus/bitmap fixture for the ACTUAL production SPG renderer.
// This does not simulate a complete MAME machine or physical PS4 execution.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
using device_type=int;
struct machine_config {};
class device_t {
public:
    device_t(const machine_config &,device_type,const char *,device_t *,uint32_t) {}
    virtual ~device_t()=default;
    virtual void device_start() {} virtual void device_reset() {}
    template<class T> void save_item(T &) {}
    template<class T> void save_pointer(T &,size_t) {}
};
#define ATTR_COLD
#define NAME(value) value
#define DEFINE_DEVICE_TYPE(symbol,...) int symbol=1;
#define DECLARE_DEVICE_TYPE(symbol,...) extern int symbol;
struct rectangle {int min_x=0,max_x=319,min_y=0,max_y=239;};
struct bitmap_rgb32 {
    std::vector<uint32_t> pixels=std::vector<uint32_t>(320*240,0xdeadbeef);
    uint32_t &pix(int y,int x) {return pixels.at(size_t(y)*320+x);}
};
class address_space {
public:
    uint32_t seed=1;
    uint64_t reads=0,trace=1469598103934665603ULL;
    uint16_t read_word(uint32_t address) {
        uint32_t v=address*1664525U+seed*1013904223U;
        v^=v>>11;v*=0x45d9f3bU;v^=v>>16;
        ++reads;trace=(trace^address)*1099511628211ULL;
        trace=(trace^uint16_t(v))*1099511628211ULL;
        return uint16_t(v);
    }
};
