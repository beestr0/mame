#!/usr/bin/env python3
"""Compare actual SPG renderer sources with a synthetic bus; never use game ROMs."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import tempfile
import re
import statistics
ROOT=Path(__file__).resolve().parents[2]
def run():
    cxx='clang++'
    flags=[cxx,'-std=c++20','-O2','-Wall','-Wextra','-Isrc/lib']
    with tempfile.TemporaryDirectory(prefix='spg-core-differential-') as folder:
        temp=Path(folder)
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
            shutil.copyfile(ROOT/'src/devices/machine/spg_renderer.h',temp/'spg_renderer.h')
            exe=temp/kind
            command=flags+['-Wno-unused-parameter','-Wno-sign-compare','-Itests/ps4/fake_renderer','-I'+str(temp),str(selected),'tests/ps4/renderer_contract.cpp','-o',str(exe)]
            subprocess.run(command,cwd=ROOT,check=True)
            executables[kind]=exe
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
