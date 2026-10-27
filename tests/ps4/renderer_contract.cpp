// SPDX-License-Identifier: BSD-3-Clause
#include "emu.h"
#include "spg_renderer.h"
#if defined(PS4_WORKLOAD_TEST)
#include "util/ps4_workload.h"
[[maybe_unused]] static uint64_t clock_tick(){static uint64_t tick=0;return ++tick;}
#endif
#include <cassert>
#include <chrono>
#include <cstdio>
struct renderer : spg_renderer_device {
    renderer(const machine_config &config):spg_renderer_device(config,"synthetic",nullptr) {device_start();device_reset();}
    void reset() {device_reset();}
};
static uint32_t random_word(uint32_t &state) {state=state*1664525+1013904223;return state;}
int main() {
#if defined(PS4_WORKLOAD_TEST) && PS4_WORKLOAD_HOOKS
    assert(ps4_workload::begin(clock_tick,0)==0);
#endif
    machine_config config;renderer device(config);address_space bus;
    uint64_t pixels_hash=1469598103934665603ULL,register_hash=0;
    const auto begin=std::chrono::steady_clock::now();
    for(unsigned scene=0;scene<192;++scene) {
        device.reset();bitmap_rgb32 bitmap;bus.seed=scene+1;
        uint32_t state=scene+35;uint16_t sprites[1024],palette[256],scroll[256];
        for(auto &p:palette)p=uint16_t(random_word(state));
        for(auto &p:scroll)p=uint16_t(random_word(state));
        for(unsigned i=0;i<256;++i) {
            sprites[4*i]=i%7?uint16_t(random_word(state)):0;
            sprites[4*i+1]=uint16_t(random_word(state));sprites[4*i+2]=uint16_t(random_word(state));
            sprites[4*i+3]=uint16_t(random_word(state));
        }
        device.set_video_reg_42(scene%3==0?0:scene%3==1?1:3);
        device.set_video_reg_2a(scene&3);device.set_video_reg_30(scene%4?0:7);
        device.set_video_reg_3c(scene%8?0x20:0x60);
        uint16_t scrollregs[]={uint16_t(scene*7),uint16_t(scene*13)};
        uint16_t page[]={uint16_t(random_word(state)),uint16_t(8|(scene&2)|((scene&1)<<4)),uint16_t(random_word(state)),uint16_t(random_word(state))};
        rectangle full;rectangle clip;clip.min_x=int(scene%17);clip.max_x=319-int(scene%19);
        const int limit=scene%4==0?0:scene%4==1?1:scene%4==2?64:256;
        for(unsigned y=0;y<240;++y) {
            device.new_line(full);
            for(int priority=0;priority<4;++priority) {
                device.draw_page(clip,y,priority,0x104,scrollregs,page,bus,palette,scroll,0);
                device.draw_sprites(clip,y,priority,0x314,bus,palette,sprites,limit);
            }
            device.apply_saturation_and_fade(bitmap,clip,y);
            if(scene%5==0 && y==97) {sprites[7]^=0xffff;palette[13]^=0x1234;device.set_video_reg_2a((scene+1)&3);}
        }
        for(auto p:bitmap.pixels)pixels_hash=(pixels_hash^p)*1099511628211ULL;
        register_hash=register_hash*65599+device.get_video_reg_42()+device.get_video_reg_2a()+device.get_video_reg_30()+device.get_video_reg_3c();
    }
#if defined(PS4_WORKLOAD_TEST) && PS4_WORKLOAD_HOOKS
    ps4_workload_result result{};assert(ps4_workload::end(&result,sizeof(result))==0);
    // The reference fixture has no production hooks. Active hooks must still
    // leave pixels/bus ordering identical; optimized source must record work.
    if(result.calls[PS4_WS_PAGE])assert(result.counts[PS4_W_VIDEO_READS]>0 && result.clock_reads<=24);
#endif
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    printf("scenes=192 pixels=%016llx bus_trace=%016llx reads=%llu registers=%016llx\n",
        (unsigned long long)pixels_hash,(unsigned long long)bus.trace,(unsigned long long)bus.reads,(unsigned long long)register_hash);
    fprintf(stderr,"HOST SYNTHETIC DEVICE ONLY elapsed_ms=%.3f; not MAME gameplay or PS4 performance\n",ms);
}
