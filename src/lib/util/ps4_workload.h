// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "ps4_workload_abi.h"
#if !defined(PS4_WORKLOAD_HOOKS) && defined(MAME_OPENORBIS) && __has_include("ps4_workload_config.h")
#include "ps4_workload_config.h"
#endif
#ifndef PS4_WORKLOAD_HOOKS
#define PS4_WORKLOAD_HOOKS 0
#endif
namespace ps4_workload {
#if PS4_WORKLOAD_HOOKS && (defined(MAME_OPENORBIS) || defined(PS4_WORKLOAD_TEST))
inline constexpr bool compiled=true;
struct state {
    bool active=false;
    uint64_t (*clock)()=nullptr;
    uint32_t seed=0;
    ps4_workload_result result{};
};
inline state data;
inline void note(unsigned event,uint64_t n=1) {if(data.active)data.result.counts[event]+=n;}
inline void opcode(uint16_t value) {if(data.active){++data.result.opcodes[value>>12];++data.result.counts[PS4_W_INSTRUCTIONS];}}
class scope {
    unsigned category;
    bool timed=false;
    uint64_t begin=0;
public:
    explicit scope(unsigned c):category(c) {
        if(!data.active)return;
        const auto call=data.result.calls[c]++;
        constexpr unsigned stride[]={64,16,64,1024,64,1,8};
        constexpr unsigned limit[]={2,2,2,2,2,1,1};
        if(data.clock && data.result.probe_calls[c]<limit[c] && (call+data.seed*17+c*13)%stride[c]==0) {
            timed=true;++data.result.probe_calls[c];
            begin=data.clock();++data.result.clock_reads;
        }
    }
    scope(const scope &)=delete;
    ~scope() {
        if(timed && data.active) {
            const auto end=data.clock();++data.result.clock_reads;
            if(end<begin)++data.result.invalid;
            else data.result.probe_ticks[category]+=end-begin;
        }
    }
};
inline int begin(uint64_t (*clock)(),uint32_t seed) {
    if(data.active)return -1;
    data.result={};data.result.version=1;data.result.size=sizeof(ps4_workload_result);
    data.seed=seed;data.clock=clock;data.active=true;return 0;
}
inline int end(ps4_workload_result *out,uint32_t bytes) {
    if(!data.active)return -1;
    data.active=false;data.clock=nullptr;
    if(!out || bytes!=sizeof(ps4_workload_result))return -2;
    *out=data.result;return out->invalid?-3:0;
}
#else
inline constexpr bool compiled=false;
inline void note(unsigned,uint64_t=1) {}
inline void opcode(uint16_t) {}
struct scope {explicit scope(unsigned) {}};
inline int begin(uint64_t (*)(),uint32_t) {return -1;}
inline int end(ps4_workload_result *,uint32_t) {return -1;}
#endif
template<class Space> inline uint16_t video_word(Space &space,uint32_t address) {
    note(PS4_W_VIDEO_READS);return space.read_word(address);
}
}
