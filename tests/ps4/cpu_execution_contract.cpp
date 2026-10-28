// SPDX-License-Identifier: BSD-3-Clause
#include <bit>
#include <climits>
#include <cstring>
#include <chrono>
#include <ctime>
#include "cpu-selected.inc"
static uint64_t hash=1469598103934665603ULL;
static void mix(uint64_t n){hash=(hash^n)*1099511628211ULL;}
static void snapshot(unsp_device &cpu,address_space &bus){
 if(bus.trace_file){
  bus.trace_event(5,sizeof(cpu.state),0);std::fwrite(&cpu.state,sizeof(cpu.state),1,bus.trace_file);
  const uint64_t irqs[]={cpu.irq_events,cpu.irq_trace};std::fwrite(irqs,sizeof(irqs),1,bus.trace_file);
 }
 for(uint32_t value:cpu.state.m_r)mix(value);
 const auto *words=reinterpret_cast<const uint32_t*>(&cpu.state);
 for(unsigned n=0;n<sizeof(cpu.state)/4;++n)mix(words[n]);
 mix(cpu.irq_events);mix(cpu.irq_trace);mix(bus.trace);mix(bus.write_trace);
}
int main(int argc,char **argv){
 address_space bus;unsp_device cpu(bus);
 const bool trace=argc==3 && std::string(argv[1])=="--trace";
 if(trace){bus.trace_file=std::fopen(argv[2],"wb");if(!bus.trace_file)throw std::runtime_error("trace create failed");}
 if(argc>1 && !trace){
  const std::string mode=argv[1];
  bus.value_override=[&](uint32_t,unsigned){
   if(mode=="alu")return uint16_t(0x0241);
   if(mode=="branch")return uint16_t(0xee01);
   if(mode=="memory")return uint16_t(bus.reads%2?0x9200:0x55aa);
   return uint16_t(0xf165);
  };
  cpu.state.m_r[7]=0x8000;cpu.state.m_r[0]=0x1000;
  const auto begin=std::chrono::steady_clock::now();const auto cpu_begin=std::clock();
  for(unsigned run=0;run<16;++run){cpu.state.m_icount=1000000;cpu.execute_run();}
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
  printf("instructions=%llu lookups=%llu elapsed_ms=%.3f cpu_ms=%.3f; SYNTHETIC actual interpreter + production cache, not PS4 throughput\n",(unsigned long long)cpu.hooks,(unsigned long long)bus.lookups,ms,1000.0*(std::clock()-cpu_begin)/CLOCKS_PER_SEC);return 0;
 }
 unsigned exceptions=0,trials=0,invalid_register_ranges=0;
 bus.event=[&](uint32_t,address_space &memory){
  if(memory.reads%137==0){cpu.state.m_sirq^=1U<<((memory.reads/137)%9);}
  if(memory.reads%997==0)memory.remap();
 };
 const unsigned regions[]={0x100,0x2800,0x8000,0x1ffff0,0x200000,0x3fffff};
 for(unsigned region:regions)for(unsigned op=0;op<65536;++op){
  const unsigned op0=op>>12,opa=(op>>9)&7,op1=(op>>6)&7,opn=(op>>3)&7;
  if(((op1<<4)|op0)==0x2d && opn>opa+1){++invalid_register_ranges;continue;}
  if(((op1<<4)|op0)==0x29 && op!=0x9a98 && opn+opa>7){++invalid_register_ranges;continue;}
  ++trials;
  cpu.state={};cpu.state.m_r[7]=region&0xffff;cpu.state.m_r[6]=(region>>16)&63;
  for(unsigned r=0;r<6;++r)cpu.state.m_r[r]=((op*17+r*79)&0xfff)+0x400;
  cpu.state.m_pri=8;cpu.state.m_enable_irq=cpu.state.m_enable_fiq=1;
  cpu.state.m_sirq=op%19?0:1U<<(op%9);cpu.state.m_icount=0;
  const auto first=bus.reads+1;
  bus.value_override=[&](uint32_t address,unsigned){
   if(bus.reads==first)return uint16_t(op);
   if(address<bus.ram_words.size())return bus.ram_words[address];
   return uint16_t(0xf165);
  };
  if(op%313==0)bus.remap();
  try{cpu.execute_run();}catch(const std::runtime_error&){++exceptions;}
  snapshot(cpu,bus);
 }
 bus.event={};
 // Explicit RAM self-modification: the stored NOP must be fetched afresh.
 bus.value_override=[&](uint32_t a,unsigned){return a<bus.ram_words.size()?bus.ram_words[a]:uint16_t(0xf165);};
 cpu.state={};cpu.state.m_r[7]=0x100;cpu.state.m_r[1]=0xf165;cpu.state.m_r[2]=0x101;
 bus.ram_words[0x100]=0xd2c2;bus.ram_words[0x101]=0x9245;
 cpu.execute_run();cpu.state.m_icount=0;cpu.execute_run();
 if(cpu.state.m_r[1]!=0xf165 || cpu.state.m_r[7]!=0x102 || bus.ram_words[0x101]!=0xf165)throw std::runtime_error("self-modifying RAM fetch failed");
 snapshot(cpu,bus);
 // Live bank contents do not need a mapping invalidation; handler stays live.
 for(unsigned bank=0;bank<32;++bank){
  bus.seed=bank;bus.value_override=[&](uint32_t,unsigned){return uint16_t(0x9240|(bus.seed&31));};
  cpu.state={};cpu.state.m_r[7]=0x8000;cpu.execute_run();
  if(cpu.state.m_r[1]!=(bank&31))throw std::runtime_error("bank content stale");snapshot(cpu,bus);
 }
 // Map changes must replace cached handlers, including the first access after it.
 bus.value_override=[](uint32_t,unsigned kind){return uint16_t(0x9240+kind);};
 for(unsigned map=0;map<40;++map){bus.remap();cpu.state={};cpu.state.m_r[6]=0x20;cpu.execute_run();
  if(cpu.state.m_r[1]!=(bus.mapping_changed?3U:2U))throw std::runtime_error("mapping invalidation failed");snapshot(cpu,bus);}
 for(auto value:bus.ram_words)mix(value);
 if(bus.trace_file){std::fwrite(bus.ram_words.data(),sizeof(uint16_t),bus.ram_words.size(),bus.trace_file);if(std::fclose(bus.trace_file)!=0)throw std::runtime_error("trace close failed");bus.trace_file=nullptr;}
 printf("opcodes=%u invalid_register_ranges=%u exceptions=%u state_cycles_irq_and_memory_hash=%016llx reads=%llu writes=%llu io_reads=%llu\n",trials,invalid_register_ranges,exceptions,(unsigned long long)hash,(unsigned long long)bus.reads,(unsigned long long)bus.writes,(unsigned long long)bus.io_reads);
}
