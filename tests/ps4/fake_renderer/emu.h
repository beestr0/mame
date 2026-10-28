// SPDX-License-Identifier: BSD-3-Clause
// Minimal synthetic bus/bitmap fixture for the ACTUAL production SPG renderer.
// This does not simulate a complete MAME machine or physical PS4 execution.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include <array>
using device_type=int;
struct machine_config {};
class device_t {
public:
    device_t(const machine_config &,device_type,const char *,device_t *,uint32_t) {}
    virtual ~device_t()=default;
    virtual void device_start() {} virtual void device_reset() {}
    template<class T> void save_item(T &) {}
    template<class T> void save_pointer(T &,size_t) {}
};
#define ATTR_FORCE_INLINE __attribute__((always_inline))
#define ATTR_COLD
#define NAME(value) value
#define DEFINE_DEVICE_TYPE(symbol,...) int symbol=1;
#define DECLARE_DEVICE_TYPE(symbol,...) extern int symbol;
struct rectangle {int min_x=0,max_x=319,min_y=0,max_y=239;};
struct bitmap_rgb32 {
    std::vector<uint32_t> pixels=std::vector<uint32_t>(320*240,0xdeadbeef);
    uint32_t &pix(int y,int x) {return pixels.at(size_t(y)*320+x);}
};
// Synthetic mapped framework around actual MAME memory_access_cache methods.
#include <functional>
#include <cstdio>
using offs_t=uint32_t;using u32=uint32_t;using u16=uint16_t;
enum endianness_t {ENDIANNESS_LITTLE,ENDIANNESS_BIG};
enum class read_or_write:unsigned {READ=1,WRITE=2,READWRITE=3};
#define EXPECTED(value) (value)
namespace util {using notifier_subscription=std::shared_ptr<std::function<void(read_or_write)>>;}
template<int Width,int Shift>struct handler_entry_read {
 virtual ~handler_entry_read()=default;
 virtual uint16_t read(offs_t,uint16_t)const=0;
 virtual void lookup(offs_t,offs_t&,offs_t&,handler_entry_read*&)const{}
};
template<int Width,int Shift>struct handler_entry_write {};
class address_space {
 struct Entry:handler_entry_read<1,-1>{address_space *bus;unsigned kind;
  Entry(address_space *b,unsigned k):bus(b),kind(k){}
  uint16_t read(offs_t a,uint16_t m)const override{return bus->read_data(a,m,kind);}
 } ram{this,0},io{this,1},rom{this,2},remapped{this,3};
 struct Root:handler_entry_read<1,-1>{address_space *bus;
  explicit Root(address_space *b):bus(b){}
  void lookup(offs_t a,offs_t &start,offs_t &end,handler_entry_read<1,-1>*&entry)const override{
   ++bus->lookups;
   if(a<0x2000){start=0;end=0x1fff;entry=&bus->ram;}
   else if(a<0x4000){start=0x2000;end=0x3fff;entry=&bus->io;}
   else if(bus->mapping_changed && a>=0x200000){start=0x200000;end=0x7fffff;entry=&bus->remapped;}
   else {start=0x4000;end=bus->mapping_changed?0x1fffff:0x7fffff;entry=&bus->rom;}
  }
  uint16_t read(offs_t a,uint16_t m)const override{
   return bus->m_low_dispatch[a&0x3fff]->read(a,m);
  }
 }root{this};
 handler_entry_write<1,-1>write_root;
 std::vector<std::weak_ptr<std::function<void(read_or_write)>>>notifiers;
public:
 std::FILE *trace_file=nullptr;
 void trace_event(uint32_t kind,uint32_t address,uint32_t value){if(trace_file){const uint32_t record[]={kind,address,value};std::fwrite(record,sizeof(record),1,trace_file);}}
 std::function<uint16_t(uint32_t,unsigned)>value_override;
 std::vector<uint16_t>ram_words=std::vector<uint16_t>(0x2000);
 uint64_t writes=0,write_trace=0;
 std::array<handler_entry_read<1,-1>*,512>m_dispatch_read{};
 std::array<handler_entry_read<1,-1>*,0x4000>m_low_dispatch{};
 address_space(){refresh_dispatch();}
 void refresh_dispatch(){
  m_dispatch_read.fill(&rom);m_dispatch_read[0]=&root;
  if(mapping_changed)for(unsigned i=0x200000>>14;i<m_dispatch_read.size();++i)m_dispatch_read[i]=&remapped;
  for(unsigned i=0;i<m_low_dispatch.size();++i)m_low_dispatch[i]=i<0x2000?static_cast<handler_entry_read<1,-1>*>(&ram):&io;
 }
 uint16_t read_specific(uint32_t a);
 void write_specific(uint32_t a,uint16_t v){a&=addrmask();++writes;trace_event(4,a,v);write_trace=write_trace*65599+a*17+v;if(a<ram_words.size())ram_words[a]=v;if(event)event(a,*this);}
 uint32_t seed=1;unsigned width=23,bits=16;int shift=-1;endianness_t endian=ENDIANNESS_BIG;
 uint64_t reads=0,trace=1469598103934665603ULL,lookups=0,io_reads=0;
 bool mapping_changed=false;
 std::function<void(uint32_t,address_space&)>event;
 uint32_t addrmask()const{return (1U<<width)-1;}
 unsigned addr_width()const{return width;}unsigned data_width()const{return bits;}
 int addr_shift()const{return shift;}endianness_t endianness()const{return endian;}
 template<class C>void cache(C &target);
 template<class C>void specific(C &target);
 template<class F>util::notifier_subscription add_change_notifier(F f){
  auto value=std::make_shared<std::function<void(read_or_write)>>(f);notifiers.push_back(value);return value;
 }
 void remap(){mapping_changed=!mapping_changed;refresh_dispatch();for(auto &weak:notifiers)if(auto fn=weak.lock())(*fn)(read_or_write::READWRITE);}
 virtual ~address_space()=default;
 virtual uint16_t read_word(uint32_t address){return read_specific(address);}
 uint16_t read_data(uint32_t address,uint16_t mask,unsigned kind){
  ++reads;if(kind==1)++io_reads;
  if(event)event(address,*this);
  uint32_t v=address*1664525U+seed*1013904223U+(kind==3?0x314159U:0);
  v^=v>>11;v*=0x45d9f3bU;v^=v>>16;
  if(value_override)v=value_override(address,kind);
  trace_event(kind,address,uint16_t(v)&mask);
  trace=(trace^address)*1099511628211ULL;trace=(trace^(uint16_t(v)&mask))*1099511628211ULL;
  return uint16_t(v)&mask;
 }
};
#if __has_include("fake_memory_cache.h")
#include "fake_memory_cache.h"
#else
#error "Generate fake_memory_cache.h from production emumem.h before compiling this fixture"
#endif
