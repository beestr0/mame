# v1.12 equivalent unSP/SPG arithmetic

Only two emulation-source expressions change from owner-tested f972250863a3:

* unSP pending IRQ selection: mask bits0..8 and use portable C++20 countr_zero,
  returning-1 when no in-range bit is set. Preserve all checks, delivery order,
  fetch/decode/PC/cycle behavior and instruction-boundary call placement.
* SPG RGB555 blend: evaluate red/blue in separate10-bit lanes and green separately.
  With original alpha weights8/16/24/32 the weighted5-bit sum is<=992, preventing
  lane carry. Retain original integer floors, flags, priorities, palette and bus
  access order. No emulated RAM snapshots or caches are introduced.

tests/ps4/run_safe_core_tests.py exhausts65536 IRQ masks plus100000 arbitrary masks,
all RB lane pairs/green pairs at33weights plus100000 full color pairs. It compiles
the actual old and new SPG renderers with the same synthetic bus and compares192
scenes, pixels, registers and7,531,584 ordered bus reads. The pinned original
reference retains its source license and SHA256. Five interleaved device timings
and arithmetic microbenchmarks are host-only, not PS4 speed or full machine tests.

Scheduler, instruction semantics, CPU clocks, sound, frame skipping and the
experimental DRC remain unchanged. The existing optional coarse profiler and ABI
are retained; new frontend reports expose its pre-existing call counts and a
one-shot requested clock probe rather than subtracting guessed overhead.
Owner v111 data has100 valid samples/540712 clock reads and19.72067ms sampled core
versus15.81421ms on unsampled frames; normal execution is not19.72ms.
