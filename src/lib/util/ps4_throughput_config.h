// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#if defined(MAME_OPENORBIS) && __has_include("ps4_throughput_build_config.h")
#include "ps4_throughput_build_config.h"
#endif
#ifndef PS4_UNSP_ALU_INLINE
#define PS4_UNSP_ALU_INLINE 1
#endif
#if defined(MAME_OPENORBIS) && PS4_UNSP_ALU_INLINE && (defined(__GNUC__) || defined(__clang__))
#define PS4_UNSP_ALU_ATTR __attribute__((always_inline))
#else
#define PS4_UNSP_ALU_ATTR
#endif
