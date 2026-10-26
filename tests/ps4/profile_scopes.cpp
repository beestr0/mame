// SPDX-License-Identifier: BSD-3-Clause
// Synthetic scope arithmetic, not PS4 timing or game execution.
#define PS4_PROFILE_TEST
#include "util/ps4_profile.h"
#include <cassert>
static uint64_t tick;
static uint64_t clock_tick() {return tick;}
static_assert(sizeof(ps4_core_profile)==120);
int main() {
    ps4_core_profile out{};
    assert(ps4_profile::begin(clock_tick)==0);
    tick=10;
    {ps4_profile::scope cpu(PS4_PROFILE_CPU);tick+=20;
        {ps4_profile::scope video(PS4_PROFILE_VIDEO);tick+=30;}
        tick+=40;
    }
    tick+=50;
    assert(ps4_profile::end(&out,sizeof(out))==0);
    assert(out.ticks[PS4_PROFILE_CPU]==60 && out.ticks[PS4_PROFILE_VIDEO]==30 && out.ticks[PS4_PROFILE_OTHER]==60);
    const auto reads=out.clock_reads;
    {ps4_profile::scope cpu(PS4_PROFILE_CPU);tick+=20;}
    assert(ps4_profile::end(&out,sizeof(out))==-1 && out.clock_reads==reads);
}
