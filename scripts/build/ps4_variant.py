#!/usr/bin/env python3
"""Write stable, ignored per-feature headers; never mutate tracked source."""
from pathlib import Path
import json
import sys
VARIANTS={'control':(0,0,0),'irq-only':(1,0,0),'blend-only':(0,1,0),'both':(1,1,0),'workload':(1,1,1)}
def configure(folder,variant):
    if variant not in VARIANTS:raise ValueError('Unsupported PS4 core variant: '+variant)
    root=Path(folder);root.mkdir(parents=True,exist_ok=True)
    irq,blend,work=VARIANTS[variant]
    values={'PS4_OPT_IRQ':irq,'PS4_OPT_BLEND':blend,'PS4_WORKLOAD_HOOKS':work}
    def write(path,text):
        if path.exists() and path.read_text()==text:return
        temporary=path.with_suffix(path.suffix+'.tmp');temporary.write_text(text);temporary.replace(path)
    for name,value in values.items():
        leaf={'PS4_OPT_IRQ':'ps4_irq_config.h','PS4_OPT_BLEND':'ps4_blend_config.h','PS4_WORKLOAD_HOOKS':'ps4_workload_config.h'}[name]
        write(root/leaf,f'// Generated build selection; not source.\n#define {name} {value}\n')
    metadata={'variant':variant,'irq':irq,'blend':blend,'workload':work,
              'identity':f'PS4CORE:irq={irq};blend={blend};workload={work}'}
    write(root/'ps4-build-config.json',json.dumps(metadata,sort_keys=True,indent=2)+'\n')
    return metadata
if __name__=='__main__':print(json.dumps(configure(sys.argv[1],sys.argv[2]),sort_keys=True))
