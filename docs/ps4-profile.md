# Opt-in OpenOrbis coarse wall profiling

The v1.11 Batman frontend can enable measurement for one in17 retro_run calls,
only during an explicitly enabled performance capture. No emulation decisions
depend on timings. CPU instruction execution, clocks, device algorithms, audio,
video output and game inputs are unchanged.

util/ps4_profile.h compiles to a no-op outside MAME_OPENORBIS. On OpenOrbis its
disabled scopes only check the inactive clock pointer. No per-instruction hook,
allocation, I/O, lock or logging is added. A shared host clock and fixed120-byte
versioned ABI expose retro_ps4_profile_begin/end. Nested scopes charge exclusive
wall ticks to CPU execution, SPG video generation, SPG sound generation,
Libretro software rasterization and scheduling; remaining work is other.
CPU includes memory/IRQ handlers except separately scoped nested devices.
Scopes measure wall time and cannot prove instruction-level or thermal causes.

The frontend subtracts its actual host video/audio callbacks from other ticks;
the reviewed Libretro implementation invokes these after retro_main_loop.
Original aggregate frame/core/video metrics remain independent. Sampling has
overhead; headline comparison captures keep this API disabled. Invalid/backward
clocks reject that sample instead of manufacturing category times. No ROM data
or private SDK is required to compile or test the API.

Frontend tests compare the ABI bytes/layout, synthetic nested-exclusive clocks,
disable/reset/error behavior, and the real host integration. CI cross-builds
the actual public core and verifies exported/linked measurement symbols.
Physical PS4 category measurements are not available until the owner captures
the new build; previous physical15.7–15.9ms core timings combine all categories.
