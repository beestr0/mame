# SPDX-License-Identifier: BSD-3-Clause
"""Compile the ACTUAL MAME cache algorithms in a small synthetic framework.
The real set/notifier, range check and native-read bodies are copied verbatim.
This tests mapped-handler semantics; fake dispatch timing is not PS4 throughput.
"""
from pathlib import Path
import hashlib,json

def body(source,marker):
    start=source.index(marker);brace=source.index('{',start);depth=0
    for end in range(brace,len(source)):
        if source[end]=='{':depth+=1
        elif source[end]=='}':
            depth-=1
            if not depth:return source[start:end+1]
    raise ValueError(marker)

def generate(root,out):
    src=(root/'src/emu/emumem.h').read_text()
    check=body(src,'void check_address_r(offs_t address)')
    native=body(src,'read_native(offs_t address, emu::detail::handler_entry_size_t<Width> mask)')
    native=native[native.index('{'):]
    marker='set(address_space *space, std::pair<void *, void *> rw)'
    setup=body(src[src.rindex(marker):],marker)
    dispatch=body(src,'template<int Level, int Width, int AddrShift> emu::detail::handler_entry_size_t<Width> dispatch_read(')
    lowbits=body(src,'constexpr int handler_entry_dispatch_level_to_lowbits(')
    prefix='namespace emu::detail {template<int Width>using handler_entry_size_t=uint16_t;'+lowbits+'}\n'+dispatch+'\n'
    specific_read=body(src,'NativeType read_native(offs_t address, NativeType mask = ~NativeType(0)) {')
    marker='set(address_space *space, std::pair<const void *, const void *> rw)'
    specific_set=body(src[src.rindex(marker):],marker)
    specific='''namespace emu::detail {
    template<int Level,int Width,int AddrShift,endianness_t Endian>class memory_access_specific {
    friend class ::address_space;using NativeType=uint16_t;
    address_space *m_space=nullptr;offs_t m_addrmask=0;
    const handler_entry_read<Width,AddrShift>*const*m_dispatch_read=nullptr;
    const handler_entry_write<Width,AddrShift>*const*m_dispatch_write=nullptr;
    void set(address_space*,std::pair<const void*,const void*>);
    public:uint16_t read_word(offs_t a){return read_native(a);}
    '''+specific_read+'''
    };
    template<int Level,int Width,int AddrShift,endianness_t Endian>
    void memory_access_specific<Level,Width,AddrShift,Endian>::'''+specific_set+'}\n'
    code=prefix+specific+'''// Generated FROM PRODUCTION emumem.h; do not hand-edit method bodies.
namespace emu::detail {
template<int Width,int AddrShift,endianness_t Endian>class memory_access_cache {
 friend class ::address_space;
 using NativeType=uint16_t;
 address_space *m_space=nullptr;
 offs_t m_addrmask=0,m_addrstart_r=1,m_addrend_r=0,m_addrstart_w=1,m_addrend_w=0;
 handler_entry_read<Width,AddrShift> *m_cache_r=nullptr,*m_root_read=nullptr;
 handler_entry_write<Width,AddrShift> *m_cache_w=nullptr,*m_root_write=nullptr;
 util::notifier_subscription m_subscription;
 void set(address_space *space,std::pair<void*,void*> rw);
 public:
 '''+check+'''
 uint16_t read_word(offs_t address){return read_native(address);}
 NativeType read_native(offs_t address,NativeType mask=0xffff)'''+native+'''
};
template<int Width,int AddrShift,endianness_t Endian>
void memory_access_cache<Width,AddrShift,Endian>::'''+setup+'''
}
template<int AddrWidth,int Width,int AddrShift,endianness_t Endian>struct memory_access {
 using cache=emu::detail::memory_access_cache<Width,AddrShift,Endian>;
 using specific=emu::detail::memory_access_specific<1,Width,AddrShift,Endian>;
};
template<class C>void address_space::specific(C &target){target.set(this,{m_dispatch_read.data(),nullptr});}
template<class C>void address_space::cache(C &target){target.set(this,{&root,&write_root});}
inline uint16_t address_space::read_specific(uint32_t a){return dispatch_read<1,1,-1>(~offs_t(0),a&addrmask(),0xffff,m_dispatch_read.data());}
'''
    (out/'fake_memory_cache.h').write_text(code)
    return {'source':'src/emu/emumem.h','source_sha256':hashlib.sha256(src.encode()).hexdigest(),'exact_bodies':{name:hashlib.sha256(value.encode()).hexdigest() for name,value in [('check_address_r',check),('read_native',native),('set_and_invalidation',setup)]}}
