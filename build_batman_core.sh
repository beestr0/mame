#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

export OO_PS4_TOOLCHAIN="$HOME/Projects/Batman-PS4/sdk/OpenOrbis/PS4Toolchain"

make -f makefile \
  TARGET=mame \
  SUBTARGET=batman \
  SOURCES=src/mame/tvgames/spg2xx_jakks.cpp \
  CONFIG=libretro \
  OSD=retro \
  TARGETOS=freebsd \
  CROSS_BUILD=1 \
  CC=/usr/bin/clang \
  CXX=/usr/bin/clang++ \
  OVERRIDE_CC=/opt/homebrew/opt/llvm/bin/clang \
  OVERRIDE_CXX=/opt/homebrew/opt/llvm/bin/clang++ \
  OVERRIDE_AR=/opt/homebrew/opt/llvm/bin/llvm-ar \
  PTR64=1 \
  NOASM=1 \
  NO_OPENGL=1 \
  NO_USE_MIDI=1 \
  NO_USE_PORTAUDIO=1 \
  DONT_USE_NETWORK=1 \
  NOWERROR=1 \
  REGENIE=1 \
  ARCHOPTS="--target=x86_64-pc-freebsd12-elf -D_GNU_SOURCE -isysroot $OO_PS4_TOOLCHAIN -isystem $OO_PS4_TOOLCHAIN/include/c++/v1 -isystem $OO_PS4_TOOLCHAIN/include" \
  -j2 2>&1 | tee ../mame-ps4-build.log
