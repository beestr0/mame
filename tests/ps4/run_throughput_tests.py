#!/usr/bin/env python3
"""Differential actual-source CPU/render tests and paired HOST-only benchmarks."""
from pathlib import Path
import filecmp,hashlib,json,re,shutil,statistics,subprocess,tempfile
from cache_fixture import generate
from cpu_fixture import generate_cpu
ROOT=Path(__file__).resolve().parents[2]

def run():
    with tempfile.TemporaryDirectory(prefix='batman-throughput-') as folder:
        out=Path(folder);evidence={'cache_fixture':generate(ROOT,out)}
        # Match the production -O2 PIC mode; compare unchanged instruction bodies.
        flags=['clang++','-std=c++20','-O2','-fPIC','-DMAME_OPENORBIS=1','-Wno-sign-compare','-Wno-unused-parameter','-Itests/ps4/fake_renderer','-I'+str(out),'-Isrc/lib','-Isrc/devices']
        executables={};cpu_results={};renderer_results={};bench={}
        original_bodies=None
        for name,reference in [('reference',True),('candidate',False)]:
            evidence['cpu_'+name]=generate_cpu(ROOT,out,reference)
            bodies={p.name:p.read_text().replace('PS4_UNSP_ALU_ATTR ','') for p in out.glob('body-*.cpp')}
            if original_bodies is None:original_bodies=bodies
            else:assert bodies==original_bodies,'Interpreter function bodies changed'
            objects=[]
            for part in ['unsp.cpp','unsp_other.cpp','unsp_jumps.cpp','unsp_fxxx.cpp','unsp_exxx.cpp','unsp_extended.cpp']:
                src=out/(name+'-'+part)
                src.write_text('#include <bit>\n#include <climits>\n#include "cpu-base.h"\n'+(out/('body-'+part)).read_text())
                obj=src.with_suffix('.o');subprocess.run(flags+['-c',str(src),'-o',str(obj)],cwd=ROOT,check=True);objects.append(str(obj))
            driver=out/'cpu-driver.cpp';driver.write_text((ROOT/'tests/ps4/cpu_execution_contract.cpp').read_text().replace('"cpu-selected.inc"','"cpu-base.h"'))
            exe=out/('cpu-'+name);subprocess.run(flags+[str(driver),*objects,'-o',str(exe)],cwd=ROOT,check=True)
            executables['cpu-'+name]=exe;cpu_results[name]=subprocess.check_output([str(exe),'--trace',str(out/(name+'-state.bin'))],text=True,timeout=40)
            # Verify normal call site elimination in the actual extracted interpreter object.
            assembly=subprocess.check_output(['objdump','-d','-C',str(exe)],text=True)
            evidence['alu_helper_calls_'+name]=len(re.findall(r'call[^\n]*<unsp_device::do_basic_alu_ops',assembly))
            shutil.copy(ROOT/('tests/ps4/reference/spg_renderer_v113.h'if reference else'src/devices/machine/spg_renderer.h'),out/'spg_renderer.h')
            source=ROOT/('tests/ps4/reference/spg_renderer_v113.cpp'if reference else'src/devices/machine/spg_renderer.cpp')
            selected=out/(name+'.cpp');shutil.copy(source,selected)
            exe=out/('renderer-'+name);subprocess.run(flags+[str(selected),'tests/ps4/renderer_contract.cpp','-o',str(exe)],cwd=ROOT,check=True)
            code=subprocess.check_output(['objdump','-d','-C',str(exe)],text=True)
            evidence['graphics_wrapper_calls_'+name]=len(re.findall(r'call[^\n]*read_video_word',code))
            if not reference:assert evidence['graphics_wrapper_calls_'+name]==0,'Extra hot read wrapper call remains'
            executables['renderer-'+name]=exe
            renderer_results[name]={mode:subprocess.check_output([str(exe),mode],text=True,stderr=subprocess.DEVNULL,timeout=40)for mode in ['normal','mutation','unsupported']}
        assert filecmp.cmp(out/'reference-state.bin',out/'candidate-state.bin',shallow=False),'Per-case CPU state or per-access mapped read/write bytes differ'
        evidence['exact_trace_bytes']=(out/'reference-state.bin').stat().st_size
        evidence['exact_trace_sha256']=hashlib.sha256((out/'reference-state.bin').read_bytes()).hexdigest()
        assert cpu_results['reference']==cpu_results['candidate'],'CPU register/cycle/IRQ/memory side effects differ'
        assert renderer_results['reference']==renderer_results['candidate'],'Renderer pixels/read ordering/side effects differ'
        assert evidence['alu_helper_calls_reference']>0 and evidence['alu_helper_calls_candidate']==0,'ALU reference-argument call not eliminated'
        for workload in ['alu','nop','branch','memory','renderer']:
            times={name:[]for name in ['reference','candidate']};cpu_times={name:[]for name in times}
            for repetition in range(9):
                for name in (list(times)if repetition%2==0 else list(times)[::-1]):
                    command=[str(executables[('renderer-'if workload=='renderer'else'cpu-')+name])]
                    if workload!='renderer':command.append(workload)
                    result=subprocess.run(command,check=True,capture_output=True,text=True,timeout=40)
                    time=re.search(r'elapsed_ms=([\d.]+)',result.stdout+result.stderr);assert time
                    times[name].append(float(time[1]));cpu_time=re.search(r'cpu_ms=([\d.]+)',result.stdout+result.stderr);assert cpu_time;cpu_times[name].append(float(cpu_time[1]))
            bench[workload]={name:{'median_ms':statistics.median(t),'samples_ms':t}for name,t in times.items()}
            bench[workload]['process_cpu_time']={name:{'median_ms':statistics.median(t),'samples_ms':t}for name,t in cpu_times.items()}
            bench[workload]['process_cpu_reduction_percent']=100*(1-statistics.median(cpu_times['candidate'])/statistics.median(cpu_times['reference']))
            bench[workload]['reduction_percent']=100*(1-bench[workload]['candidate']['median_ms']/bench[workload]['reference']['median_ms'])
        assert (ROOT/'src/devices/machine/spg_renderer.cpp').read_bytes()==(ROOT/'tests/ps4/reference/spg_renderer_v113.cpp').read_bytes(),'Rendering source changed'
        report={'rendering_source_unchanged':True,'physical_ps4_measured':False,'compiler':'LLVM18 -O2, no LTO/no fast-math','cpu':cpu_results,'renderer':renderer_results,'source_and_codegen':evidence,'host_benchmarks':bench,
                'limitations':'Actual instruction/cache bodies; synthetic memory framework and workloads, no whole-machine audio/ROM benchmark; no PS4 FPS inference'}
        target=ROOT/'build/throughput-host-results.json';target.parent.mkdir(exist_ok=True);target.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report,indent=2));print('PASS actual-source CPU state/cycles/IRQs and renderer pixels/read order incl live bank/map/palette/sprite changes; HOST simulations only')
if __name__=='__main__':run()
