# OpenOrbis JAKKS Libretro target

The starting branch is `batman-ps4-port` at
`b7af2e2431e6ab4a6289fe80926385a4751a36f7`. Development is on
`dev/linux-openorbis`, preserving the original port and upstream history.

The sibling BatmanPS4 repository provides the Linux container setup:

```sh
cd ../BatmanPS4
./tools/setup_linux.sh
cd ../mame
../BatmanPS4/tools/run_linux.sh ./build_batman_core.sh
```

`OO_PS4_TOOLCHAIN` is required when building directly. LLVM 18 compiles target
objects for `x86_64-pc-freebsd12-elf` against the public OpenOrbis v0.5.4 SDK.
Host compiler variables are separate from target compiler variables. `JOBS`
defaults to 2; keep peak C++ template compilation memory in mind.

This selects `src/mame/tvgames/spg2xx_jakks.cpp`, including `jak_batm`, `jak_spdv`
(The Amazing Spider-Man in Villain Round-Up), and `jak_spd3` (Spider-Man 3).
It does not include every JAKKS hardware family. The output archive is
`build/ps4/libmame_libretro.a`; its entry point and all dependency archives are
combined with LLVM MRI support and checked for required public Libretro exports. Only that target's build directory is scanned.
The library must still link with the native PS4 host before it is useful.

`DONT_USE_NETWORK` defines `MAME_NO_HTTP` for consistent class layout and output
module registration. Network-enabled builds retain the original HTTP server.
`LIBRETRO_OPENORBIS=1` identifies unavailable SDK features independently of the
host OS. `LIBRETRO_STATIC=1` builds an archive rather than a shared library.
`NOASM=1` and `FORCE_DRC_C_BACKEND=1` avoid native DRC execution requirements.

The SDK has libc++ 11 rather than full C++20 library support. Compatibility
helpers provide identity-preserving heterogeneous lookup and unsigned bit
operations without redefining standard-library or third-party classes.
The five previously failing `output.cpp` lookups and the offline `http.cpp`
are compiled with the real target headers during validation.

Compatibility tests:

```sh
# In the Linux build container, at this repository's root:
clang++ -std=c++20 -Isrc/lib tests/ps4/unordered_lookup.cpp -o /tmp/lookup-test
/tmp/lookup-test
clang++ --target=x86_64-pc-freebsd12-elf -D_GNU_SOURCE \
  -isysroot "$OO_PS4_TOOLCHAIN" \
  -isystem "$OO_PS4_TOOLCHAIN/include/c++/v1" \
  -isystem "$OO_PS4_TOOLCHAIN/include" \
  -std=c++20 -Isrc/lib -c tests/ps4/unordered_lookup.cpp -o /tmp/lookup-ps4.o
```

These validate lookup identity, missing keys, reference-wrapper sets, const
containers, and zero/high-bit bit operations. Target object compilation is not
execution on a PS4. See BatmanPS4/docs/PORTING.md for the audit and hardware
validation sequence. No ROMs, proprietary SDK, or licensed game artwork are
part of the build setup.

Build iterations also required a string key in screen.cpp, skipping the native
FreeBSD directory shim for the musl SDK, disabling unsupported pseudo-terminals,
and guarding linenoise's terminal-size ioctl. Offline OpenOrbis socket/domain
file transports return not_supported rather than importing DNS support into
ordinary ROM loading. Other targets retain the real POSIX implementation.
VFS version negotiation, success returns, and 64-bit sizes were corrected.

FSELF conversion additionally requires zstd's supported ZSTD_NO_TRACE and
ZSTD_TRACE=0 configuration: OpenOrbis cannot bind its optional ELF weak tracing
hooks. Rebuild all zstd objects together when changing this setting because it
changes internal context layouts. The normal compression implementation remains.
