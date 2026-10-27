// SPDX-License-Identifier: BSD-3-Clause
#ifndef BATMAN_PS4_WORKLOAD_ABI
#define BATMAN_PS4_WORKLOAD_ABI
#include <stdint.h>
#ifndef BATMAN_WORKLOAD_LINK
#define BATMAN_WORKLOAD_LINK
#endif
enum ps4_work_counter {
    PS4_W_INSTRUCTIONS, PS4_W_CPU_READS, PS4_W_CPU_WRITES, PS4_W_IRQ_CHECKS,
    PS4_W_PENDING_IRQ_CHECKS, PS4_W_CPU_CYCLES, PS4_W_SCREEN_UPDATES,
    PS4_W_SCANLINES, PS4_W_ACTIVE_PAGES, PS4_W_ELIGIBLE_SPRITES, PS4_W_TILE_STRIPS,
    PS4_W_PIXELS_DECODED, PS4_W_BLENDS, PS4_W_VIDEO_READS, PS4_W_TIMER_CALLBACKS,
    PS4_W_COUNT
};
enum ps4_work_scope {PS4_WS_CPU,PS4_WS_VIDEO,PS4_WS_PAGE,PS4_WS_SPRITE,PS4_WS_SCHEDULER,PS4_WS_BLIT,PS4_WS_SOUND,PS4_WS_COUNT};
struct ps4_workload_result {
    uint32_t version,size;
    uint64_t counts[PS4_W_COUNT],opcodes[16],calls[PS4_WS_COUNT];
    uint64_t probe_ticks[PS4_WS_COUNT],probe_calls[PS4_WS_COUNT];
    uint64_t clock_reads,invalid;
};
extern "C" {
const char *retro_ps4_build_identity() BATMAN_WORKLOAD_LINK;
unsigned retro_ps4_workload_available() BATMAN_WORKLOAD_LINK;
int retro_ps4_workload_begin(uint64_t (*clock)(),uint32_t seed) BATMAN_WORKLOAD_LINK;
int retro_ps4_workload_end(ps4_workload_result *result,uint32_t bytes) BATMAN_WORKLOAD_LINK;
}
#endif
