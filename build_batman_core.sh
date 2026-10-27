#!/usr/bin/env bash
# OpenOrbis target binaries; host tools are compiled for the Linux/macOS host.
set -euo pipefail
cd "$(dirname "$0")"
: "${OO_PS4_TOOLCHAIN:?Set OO_PS4_TOOLCHAIN to the public OpenOrbis v0.5.4 SDK}"
: "${JOBS:=2}"
: "${TARGET_CC:=clang}"
: "${TARGET_CXX:=clang++}"
: "${TARGET_AR:=llvm-ar}"
: "${TARGET_NM:=llvm-nm}"
: "${BUILD_DIR:=build/ps4}"
[[ -f "$OO_PS4_TOOLCHAIN/link.x" ]] || { echo "Invalid OpenOrbis SDK" >&2; exit 1; }
: "${PS4_CORE_VARIANT:=both}"
case "$BUILD_DIR" in /*) variant_headers="$BUILD_DIR/generated/ps4";; *) variant_headers="$(pwd)/$BUILD_DIR/generated/ps4";; esac
python3 scripts/build/ps4_variant.py "$variant_headers" "$PS4_CORE_VARIANT"
flags="--target=x86_64-pc-freebsd12-elf -D_GNU_SOURCE -isysroot $OO_PS4_TOOLCHAIN -isystem $OO_PS4_TOOLCHAIN/include/c++/v1 -isystem $OO_PS4_TOOLCHAIN/include -fPIC -I$variant_headers"
make -f makefile TARGET=mame SUBTARGET=jakks \
  SOURCES=src/mame/tvgames/spg2xx_jakks.cpp CONFIG=libretro OSD=retro \
  TARGETOS=freebsd CROSS_BUILD=1 SEPARATE_BIN=1 CC="${HOST_CC:-clang}" CXX="${HOST_CXX:-clang++}" \
  OVERRIDE_CC="$TARGET_CC" OVERRIDE_CXX="$TARGET_CXX" OVERRIDE_AR="$TARGET_AR" \
  PTR64=1 NOASM=1 FORCE_DRC_C_BACKEND=1 NO_OPENGL=1 NO_USE_MIDI=1 \
  NO_USE_PORTAUDIO=1 DONT_USE_NETWORK=1 NOWERROR=1 LIBRETRO_STATIC=1 LIBRETRO_OPENORBIS=1 \
  PYTHON_EXECUTABLE=python3 USE_QTDEBUG=0 SYMBOLS=0 OPTIMIZE=2 \
  BUILDDIR="$BUILD_DIR" ARCHOPTS="$flags" -j"$JOBS" "$@"
# Flatten the generated entry point and its dependencies into one real archive.
python3 scripts/build/merge_libretro.py "$BUILD_DIR" "$TARGET_AR" "$TARGET_NM"
