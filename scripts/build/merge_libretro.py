#!/usr/bin/env python3
"""Combine target archives using LLVM's MRI support (never rename a .so)."""
import pathlib
import subprocess
import sys
build = pathlib.Path(sys.argv[1]).resolve()
output = build / 'libmame_libretro.a'
archives = sorted((build / 'libretro' / 'bin').rglob('*.a'))
if not archives or not any('jakks_libretro' in p.name for p in archives):
    raise SystemExit('Missing JAKKS Libretro entry archive')
# MRI has no portable shell quoting. Reject ambiguous paths explicitly.
if any(any(c.isspace() for c in str(p)) for p in [output] + archives):
    raise SystemExit('Build paths must not contain whitespace')
temporary = output.with_suffix('.a.tmp')
script = '\n'.join(['create ' + str(temporary)] + ['addlib ' + str(p) for p in archives] + ['save', 'end', ''])
subprocess.run([sys.argv[2], '-M'], input=script, text=True, check=True)
symbols = subprocess.check_output([sys.argv[3], '--defined-only', '--extern-only', str(temporary)], text=True)
required = {'retro_api_version', 'retro_init', 'retro_run', 'retro_load_game', 'retro_set_environment', 'retro_unload_game', 'retro_deinit', 'retro_set_video_refresh',
            'retro_set_audio_sample', 'retro_set_audio_sample_batch', 'retro_set_input_poll',
            'retro_set_input_state', 'retro_get_system_info', 'retro_get_system_av_info',
            'retro_set_controller_port_device'}
defined = {line.split()[-1] for line in symbols.splitlines() if line.split()}
missing = required - defined
if missing:
    temporary.unlink(missing_ok=True)
    raise SystemExit('Missing Libretro exports: ' + ', '.join(sorted(missing)))
temporary.replace(output)
print('Static PS4 core:', output)
