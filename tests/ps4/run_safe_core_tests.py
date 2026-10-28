#!/usr/bin/env python3
"""Compare actual SPG renderer sources with a synthetic bus; never use game ROMs."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import tempfile
import re
import statistics
from cache_fixture import generate
ROOT=Path(__file__).resolve().parents[2]
def run():
    cxx='clang++'
    flags=[cxx,'-std=c++20','-O2','-Wall','-Wextra','-Isrc/lib']
    with tempfile.TemporaryDirectory(prefix='spg-core-differential-') as folder:
        temp=Path(folder)
        generate(ROOT,temp)
        # Generated feature headers are stable; switching one flag only rewrites its header.
        import sys,time
        sys.path.insert(0,str(ROOT/'scripts/build'))
        from ps4_variant import configure,VARIANTS
        config=temp/'generated';configure(config,'both')
        stamps={p.name:p.stat().st_mtime_ns for p in config.glob('*.h')}
        time.sleep(.01);configure(config,'irq-only')
        assert (config/'ps4_irq_config.h').stat().st_mtime_ns==stamps['ps4_irq_config.h']
        assert (config/'ps4_workload_config.h').stat().st_mtime_ns==stamps['ps4_workload_config.h']
        assert (config/'ps4_blend_config.h').stat().st_mtime_ns!=stamps['ps4_blend_config.h']
        for variant,(irq,blend,hooks) in VARIANTS.items():
            variant_flags=flags+[f'-DPS4_OPT_IRQ={irq}',f'-DPS4_OPT_BLEND={blend}',f'-DPS4_WORKLOAD_HOOKS={hooks}','-DPS4_WORKLOAD_TEST']
            exe=temp/('math-'+variant)
            subprocess.run(variant_flags+['tests/ps4/safe_core_contract.cpp','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
            exe=temp/('work-'+variant)
            subprocess.run(variant_flags+['tests/ps4/workload_contract.cpp','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
            if not hooks:
                symbols=subprocess.check_output(['nm','-C',str(exe)],text=True)
                assert 'ps4_workload::data' not in symbols
        helpers=temp/'helpers'
        subprocess.run(flags+['tests/ps4/safe_core_contract.cpp','-o',str(helpers)],cwd=ROOT,check=True)
        subprocess.run([str(helpers)],check=True)
        reference=ROOT/'tests/ps4/reference/spg_renderer_v111.cpp'
        digest=hashlib.sha256(reference.read_bytes()).hexdigest()
        assert digest=='f072937e9dd44a7a3821f806ce0f54e2b7e6e2562836b3009df4ec94a9200110','Pinned v111 production reference changed'
        print('Reference production source SHA256='+digest)
        executables={};results={};timings={'reference':[],'optimized':[]}
        for kind,source in [('reference',reference),('optimized',ROOT/'src/devices/machine/spg_renderer.cpp')]:
            # Place both actual .cpp files next to the same unmodified public header.
            selected=temp/(kind+'.cpp');shutil.copyfile(source,selected)
            shutil.copyfile(ROOT/('tests/ps4/reference/spg_renderer_v113.h'if kind=='reference'else'src/devices/machine/spg_renderer.h'),temp/'spg_renderer.h')
            exe=temp/kind
            command=flags+['-Wno-unused-parameter','-Wno-sign-compare','-Itests/ps4/fake_renderer','-I'+str(temp),str(selected),'tests/ps4/renderer_contract.cpp','-o',str(exe)]
            subprocess.run(command,cwd=ROOT,check=True)
            executables[kind]=exe
        # Actual optimized production renderer also matches the pinned original with hooks
        # compiled in, hooks actively recording, and packed arithmetic disabled.
        for blend in (0,1):
            for hooks in (0,1):
                exe=temp/f'renderer-{blend}-{hooks}'
                command=flags+[f'-DPS4_OPT_BLEND={blend}',f'-DPS4_WORKLOAD_HOOKS={hooks}','-DPS4_WORKLOAD_TEST','-Wno-unused-parameter','-Wno-sign-compare','-Itests/ps4/fake_renderer','-I'+str(temp),str(temp/'optimized.cpp'),'tests/ps4/renderer_contract.cpp','-o',str(exe)]
                subprocess.run(command,cwd=ROOT,check=True)
                result=subprocess.check_output([str(exe)],text=True)
                reference_result=subprocess.check_output([str(executables['reference'])],text=True)
                assert result==reference_result
        for repetition in range(5):
            order=['reference','optimized'] if repetition%2==0 else ['optimized','reference']
            for kind in order:
                result=subprocess.run([str(executables[kind])],check=True,capture_output=True,text=True)
                assert kind not in results or results[kind]==result.stdout,'Nondeterministic device output'
                results[kind]=result.stdout
                timings[kind].append(float(re.search(r'elapsed_ms=([\d.]+)',result.stderr)[1]))
        assert results['reference']==results['optimized'],'Production SPG renderer pixels, bus order/count or register results differ'
        for kind in ['reference','optimized']:
            print(kind+': '+results[kind].strip())
            print(f'HOST SYNTHETIC DEVICE ONLY {kind} median_ms={statistics.median(timings[kind]):.3f} samples={timings[kind]}; not PS4 performance')
    print('PASS actual reference/optimized SPG renderer:192 seeded scenes; identical pixels/read order/count/registers; synthetic fixture, not hardware')
if __name__=='__main__':run()
