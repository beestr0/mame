# SPDX-License-Identifier: BSD-3-Clause
"""Build actual ISA1 interpreter methods with a synthetic mapped bus, no ROMs.
Only device plumbing/DRC/debug callbacks are mocked; instruction/IRQ/cycle bodies
are extracted verbatim and hashed. This is not a complete machine/audio test.
"""
from pathlib import Path
import re,hashlib,json
from cache_fixture import body

def generate_cpu(root,out,reference=False):
 h=(root/'src/devices/cpu/unsp/unsp.h').read_text()
 cpp=(root/('tests/ps4/reference/unsp_v113.cpp' if reference else 'src/devices/cpu/unsp/unsp.cpp')).read_text()
 names=['update_nzsc','update_nz','push','pop','trigger_fiq','trigger_irq','check_irqs','execute_one','execute_run','get_ds','set_ds','set_fr','get_fr']
 methods=[];groups={}
 for name in names:
  match=re.search(r'^(?:inline )?(?:void|uint16_t) unsp_device::'+name+r'\(',cpp,re.M);assert match,name
  methods.append(body(cpp[match.start():],match[0]))
 groups['unsp.cpp']=list(methods)
 for file in ['unsp_other.cpp','unsp_jumps.cpp','unsp_fxxx.cpp','unsp_exxx.cpp','unsp_extended.cpp']:
  selected=root/'src/devices/cpu/unsp'/file
  if reference and file=='unsp_other.cpp':selected=root/'tests/ps4/reference/unsp_other_v113.cpp'
  text=selected.read_text();group=[]
  for match in re.finditer(r'^(?:PS4_UNSP_ALU_ATTR )?(?:inline )?(?:void|bool) unsp_device::\w+\(',text,re.M):
   selected=body(text[match.start():],match[0]);methods.append(selected);group.append(selected)
  groups[file]=group
 inline_methods=[body(h,'uint16_t read16('),body(h,'void write16('),body(h,'void add_lpc(')]
 declarations=[]
 for m in methods:
  signature=m[:m.index('{')].strip().replace('inline ','').replace('unsp_device::','').replace('PS4_UNSP_ALU_ATTR ','')
  if any(name+'(' in signature for name in ['execute_fxxx_101_group','execute_exxx_group','execute_extended_group','execute_run']):signature='virtual '+signature
  declarations.append(signature+';')
 state=body(h,'struct internal_unsp_state')+';'
 reg_enum=h[h.index('enum : uint32_t',h.index('protected:')):h.index('/* internal compiler state */')]
 setup='''#include "emu.h"
#include "util/ps4_profile.h"
#include "util/ps4_workload.h"
#include "util/unsp_irq_select.h"
#include "util/ps4_throughput_config.h"
#include "cpu/unsp/unspdefs.h"
#include <array>
#include <stdexcept>
#include <cstdio>
#define UNSP_LOG_OPCODES 0
#define UNSP_LOG_REGS 0
#define LOGMASKED(...) do{}while(0)
#define logerror(...) do{}while(0)
#define BIT(value,bit) (((value)>>(bit))&1)
enum {UNSP_FIQ_LINE=0,UNSP_IRQ0_LINE=1};
struct unsp_device {
'''+reg_enum+state+'''
 internal_unsp_state state{};internal_unsp_state *m_core=&state;
 bool m_enable_drc=false;int m_iso=10;unsigned m_vectorbase=0xfff0;
 address_space &bus;
 struct specific {address_space &bus;uint16_t read_word(uint32_t a){return bus.read_specific(a);}void write_word(uint32_t a,uint16_t v){bus.write_specific(a,v);}}m_program;
 memory_access<23,1,-1,ENDIANNESS_BIG>::cache m_cache;
 uint64_t hooks=0,irq_events=0,irq_trace=0;
 unsp_device(address_space &b):bus(b),m_program{b}{b.cache(m_cache);}
 void debugger_instruction_hook(uint32_t){++hooks;}
 void standard_irq_callback(unsigned line,uint32_t pc){++irq_events;irq_trace=irq_trace*65599+line*0x10000+pc;}
 void execute_run_drc(){throw std::runtime_error("DRC forbidden in fixture");}
 bool op_is_divq(uint16_t){return false;}
 void unimplemented_opcode(uint16_t){throw std::runtime_error("invalid synthetic opcode");}
 void unimplemented_opcode(uint16_t,uint16_t){throw std::runtime_error("invalid synthetic opcode");}
 void unimplemented_opcode(uint16_t,uint16_t,uint16_t){throw std::runtime_error("invalid synthetic opcode");}
'''+ '\n'.join(inline_methods+declarations)+'''\n};\n'''
 (out/'cpu-base.h').write_text('#pragma once\n'+setup)
 for name,items in groups.items():(out/('body-'+name)).write_text('\n'.join(items))
 all_code=setup+'\n\n'.join(methods)
 target=out/('cpu-reference.inc' if reference else 'cpu-candidate.inc');target.write_text(all_code)
 return {'reference':reference,'method_count':len(methods),'bodies_sha256':hashlib.sha256('\n'.join(methods).encode()).hexdigest()}
