// SPDX-License-Identifier: BSD-3-Clause
#include "util/ps4_workload.h"
#include <cassert>
#include <cstdio>
static uint64_t tick=0,reads=0;
static uint64_t clock_now(){++reads;return ++tick;}
static uint64_t backward(){++reads;return --tick;}
static_assert(sizeof(ps4_workload_result)==440);
struct Bus {unsigned calls=0;uint16_t read_word(uint32_t a){++calls;return uint16_t(a);}};
int main(){
    using namespace ps4_workload;
    Bus bus;assert(video_word(bus,0x1234)==0x1234 && bus.calls==1);
#if PS4_WORKLOAD_HOOKS
    for(unsigned seed=0;seed<100;++seed){
        reads=0;assert(begin(clock_now,seed)==0);assert(begin(clock_now,seed)!=0);
        for(unsigned c=0;c<PS4_WS_COUNT;++c)for(unsigned i=0;i<4000;++i){scope s(c);note(PS4_W_CPU_READS);opcode(0x1234);}
        ps4_workload_result r{};assert(end(&r,sizeof(r))==0);
        assert(r.clock_reads==reads && reads<=24 && reads>0 && r.invalid==0);
        assert(r.counts[PS4_W_CPU_READS]==28000 && r.counts[PS4_W_INSTRUCTIONS]==28000 && r.opcodes[1]==28000);
        for(unsigned c=0;c<PS4_WS_COUNT;++c){assert(r.calls[c]==4000);assert(r.probe_calls[c]<=2);}
        assert(end(&r,sizeof(r))!=0);
    }
    assert(begin(backward,0)==0);{scope s(PS4_WS_CPU);}ps4_workload_result bad{};assert(end(&bad,sizeof(bad))!=0 && bad.invalid);
    assert(begin(clock_now,0)==0);{scope s(PS4_WS_CPU);}assert(end(nullptr,0)!=0);
    assert(begin(clock_now,0)==0);ps4_workload_result clean{};assert(end(&clean,sizeof(clean))==0 && clean.counts[0]==0 && clean.clock_reads==0);
    puts("PASS bounded workload probes: 100 rotated seeds <=24 reads, raw counts, reset, nested API rejection, backward clock and ABI440; synthetic only");
#else
    assert(!compiled && begin(clock_now,0)!=0 && reads==0);
    puts("PASS workload instrumentation compiled out; original bus operation preserved");
#endif
}
