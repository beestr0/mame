// SPDX-License-Identifier: GPL-2.0-or-later
// Differential selection/arithmetic only. No guest ROM or CPU timing changes.
#include "util/unsp_irq_select.h"
#include "util/spg_rgb555_blend.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <initializer_list>
static int reference_irq(uint32_t pending) {
    int result=-1;for(int i=0;i<=8;++i)if((pending>>i)&1){result=i;break;}return result;
}
static uint16_t reference_blend(uint16_t bottom,uint16_t top,uint8_t alpha) {
    uint16_t result=0;
    for(unsigned shift:{0,5,10})result|=uint16_t(((32-alpha)*((bottom>>shift)&31)+alpha*((top>>shift)&31))>>5)<<shift;
    return result;
}
extern "C" int target_irq_select(uint32_t pending) {return unsp_detail::lowest_pending_irq(pending);}
int main() {
    for(uint32_t mask=0;mask<65536;++mask)assert(reference_irq(mask)==unsp_detail::lowest_pending_irq(mask));
    for(unsigned a=0;a<=32;++a) {
        for(unsigned bottom=0;bottom<1024;++bottom)for(unsigned top=0;top<1024;++top) {
            const uint16_t b=uint16_t((bottom&31)|((bottom>>5)<<10));
            const uint16_t t=uint16_t((top&31)|((top>>5)<<10));
            assert(reference_blend(b,t,uint8_t(a))==spg_detail::blend_rgb555(b,t,uint8_t(a)));
        }
        for(unsigned b=0;b<32;++b)for(unsigned t=0;t<32;++t)
            assert(reference_blend(b<<5,t<<5,uint8_t(a))==spg_detail::blend_rgb555(b<<5,t<<5,uint8_t(a)));
    }
    uint32_t state=123;
    for(unsigned i=0;i<100000;++i) {
        state=state*1664525+1013904223;assert(reference_irq(state)==unsp_detail::lowest_pending_irq(state));
        const auto a=uint8_t((state>>16)%33);assert(reference_blend(uint16_t(state),uint16_t(state>>16),a)==spg_detail::blend_rgb555(uint16_t(state),uint16_t(state>>16),a));
    }
    volatile int sum=0;
    for(bool optimized:{false,true}) {
        state=123;const auto begin=std::chrono::steady_clock::now();
        for(unsigned i=0;i<5000000;++i) {
            state=state*1664525+1013904223;
            // Mix no pending interrupt and high/low priority masks.
            const uint32_t mask=i%4?1U<<((state>>20)%9):0;
            sum=sum+(optimized?unsp_detail::lowest_pending_irq(mask):reference_irq(mask));
        }
        const auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-begin).count()/5000000;
        printf("HOST IRQ SELECT ONLY optimized=%d ns_per_call=%.3f; not PS4 throughput\n",int(optimized),ns);
    }
    for(bool optimized:{false,true}) {
        state=123;const auto begin=std::chrono::steady_clock::now();
        for(unsigned i=0;i<5000000;++i) {
            state=state*1664525+1013904223;
            const auto a=uint8_t(((state>>28)&3)*8+8);
            sum=sum+(optimized?spg_detail::blend_rgb555(uint16_t(state),uint16_t(state>>16),a):reference_blend(uint16_t(state),uint16_t(state>>16),a));
        }
        const auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-begin).count()/5000000;
        printf("HOST RGB555 BLEND ONLY optimized=%d ns_per_pixel=%.3f; not PS4 throughput\n",int(optimized),ns);
    }
    puts("PASS: all65536 IRQ masks plus100000 arbitrary masks; all RB lane pairs andgreen pairs at33weights plus100000 fullRGB samples; exact reference equality");
}
