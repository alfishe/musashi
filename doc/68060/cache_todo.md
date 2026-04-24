# 68060 Cache Simulation TODO

## Overview

The 68060 has separate 8KB instruction cache and 8KB data cache, each 4-way set
associative with 16-byte lines. Both caches are significantly larger than the 040
and include additional features (branch cache, store buffer).

## Cache Parameters (MC68060 UM Section 5)

### Instruction Cache
- 8KB, 4-way set associative
- 128 sets × 4 ways × 16 bytes/line
- Read-only, physically tagged
- Branch cache (256-entry BTB) — separate from I-cache

### Data Cache
- 8KB, 4-way set associative
- 128 sets × 4 ways × 16 bytes/line
- Copyback or writethrough (per page via CM bits)
- 4-entry store buffer for write gathering

### CACR Bits (060)
| Bit | Name | Function |
|-----|------|----------|
| 31 | EDC | Enable data cache |
| 30 | NAD | No allocate mode (data) |
| 29 | ESB | Enable store buffer |
| 28 | DPI | Disable CPUSH invalidation |
| 23 | EIC | Enable instruction cache |
| 22 | NAI | No allocate mode (instruction) |
| 21 | FIC | Flash invalidate I-cache |
| 20 | — | Reserved |
| 15 | EBC | Enable branch cache |
| 14 | CABC | Clear all branch cache entries |
| 11 | CLRA | Clear all (both caches) |

### Instructions
| Instruction | Behavior |
|-------------|----------|
| CINVL/CINVP/CINVA | Invalidate cache lines/pages/all |
| CPUSHL/CPUSHP/CPUSHA | Push dirty lines and invalidate |

### Cache Modes (from page descriptor CM bits)
Same as 040: WT, copyback, CI-precise, CI-imprecise

### Required Implementation
- I-cache: 128-set × 4-way tag + data arrays
- D-cache: 128-set × 4-way tag + data + dirty arrays
- Store buffer: 4-entry FIFO for write coalescing
- Branch cache: 256-entry BTB (if modeling branch prediction)
- Pseudo-LRU per set (4-way)
- CACR bit handling for enable/disable/flush
- Co-sim hooks for all cache operations

### 060-Specific: Store Buffer
The 060 store buffer merges consecutive writes to the same cache line.
Must model for co-sim if the RTL implements it.

## Priority
HIGH — Required for full 060 RTL co-simulation parity.

## Dependencies
- 040 cache model (share infrastructure, extend to 8KB)
- 060 CACR mask already implemented (0xF0F0C800)
