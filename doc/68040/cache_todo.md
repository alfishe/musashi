# 68040 Cache Simulation TODO

## Overview

The 68040 has separate 4KB instruction cache and 4KB data cache, each 4-way set associative
with 16-byte lines. The current Musashi implementation dispatches CINV/CPUSH as no-ops since
there is no cache line model. The FPGA RTL has full cache simulation; Musashi needs to match.

## Required Implementation

### Data Structures
- `uint32 icache_tag[64][4]` — 64 sets × 4 ways, tag + valid + modified bits
- `uint8  icache_data[64][4][16]` — 64 sets × 4 ways × 16 bytes/line
- `uint32 dcache_tag[64][4]` — same for data cache
- `uint8  dcache_data[64][4][16]`
- Per-way valid, dirty, and LRU bits

### Cache Parameters (MC68040 UM Section 5)
- I-cache: 4KB, 4-way SA, 16-byte lines, 64 sets, read-only
- D-cache: 4KB, 4-way SA, 16-byte lines, 64 sets, copyback or writethrough
- Line size: 16 bytes (4 longwords) — matches MOVE16 transfer size
- Replacement: pseudo-LRU per set

### Instructions to Implement
| Instruction | Encoding | Behavior |
|-------------|----------|----------|
| CINVL caches,(An) | F408+reg | Invalidate cache line containing (An) |
| CINVP caches,(An) | F410+reg | Invalidate cache page containing (An) |
| CINVA caches | F418 | Invalidate all cache entries |
| CPUSHL caches,(An) | F428+reg | Push (write-back) and invalidate line |
| CPUSHP caches,(An) | F430+reg | Push and invalidate page |
| CPUSHA caches | F438 | Push and invalidate all |

### Cache selection field (bits 7:6)
- 01 = Data cache only
- 10 = Instruction cache only
- 11 = Both caches

### CACR Integration
- CACR bit 31: DE (Data cache enable)
- CACR bit 15: IE (Instruction cache enable)
- When cache disabled, all accesses bypass

### Cache Modes (from TT/page descriptor CM bits)
- 00: Cacheable, writethrough
- 01: Cacheable, copyback
- 10: Cache-inhibited, precise
- 11: Cache-inhibited, imprecise

### Co-Sim Hooks
- `m68ki_cache_cosim_invalidate(addr, scope)` — sync with RTL on CINV
- `m68ki_cache_cosim_push(addr, scope, data)` — sync with RTL on CPUSH
- `m68ki_cache_cosim_lookup(addr)` — sync on cache hit/miss

### Memory Access Path Changes
- All reads must check cache first (when enabled)
- All writes must update cache (writethrough or copyback)
- MOVE16 operates on cache lines (16-byte aligned)
- CAS/TAS locked cycles must lock cache line

### Eviction
- Pseudo-LRU (3-bit tree per set for 4-way)
- On eviction of dirty copyback line: write-back to memory
- Eviction must fire co-sim callback

## Priority
HIGH — Required for full RTL co-simulation parity. The FPGA has cache simulation;
without it in Musashi, CINV/CPUSH and cache-dependent behavior won't match.

## Dependencies
- MOVE16 alignment (DONE)
- CAS/TAS LK tracking (DONE)
- MMU cache mode bits from page descriptors (partially available via CM field)
