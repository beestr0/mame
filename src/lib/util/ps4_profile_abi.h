// SPDX-License-Identifier: BSD-3-Clause
#ifndef BATMAN_PS4_CORE_PROFILE_ABI
#define BATMAN_PS4_CORE_PROFILE_ABI
#include <stdint.h>
#ifndef BATMAN_PROFILE_LINK
#define BATMAN_PROFILE_LINK
#endif
// Same ABI in the frontend and public core. Measurements use the host's clock.
enum { PS4_PROFILE_CPU, PS4_PROFILE_VIDEO, PS4_PROFILE_SOUND, PS4_PROFILE_BLIT,
       PS4_PROFILE_SCHEDULER, PS4_PROFILE_OTHER, PS4_PROFILE_CATEGORIES };
struct ps4_core_profile {
    uint32_t version, size;
    uint64_t ticks[PS4_PROFILE_CATEGORIES], calls[PS4_PROFILE_CATEGORIES];
    uint64_t clock_reads, invalid;
};
extern "C" {
int retro_ps4_profile_begin(uint64_t (*clock)()) BATMAN_PROFILE_LINK;
int retro_ps4_profile_end(ps4_core_profile *out, uint32_t size) BATMAN_PROFILE_LINK;
}
#endif
