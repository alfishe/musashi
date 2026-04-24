# 68030 Cache Simulation TODO

## Overview

The 68030 has a 256-byte instruction cache (no data cache). The current Musashi
implementation stores CACR but does not model cache lines. For RTL co-simulation,
full cache behavior needs to be replicated.

## Cache Parameters (MC68030 UM Section 6)
- Instruction cache only (no data cache)
- 256 bytes total, 16 entries × 4 longwords (16 bytes per entry)
- Direct-mapped (no associativity)
- No write capability (I-cache is read-only)

### CACR Bits (030)
| Bit | Name | Function |
|-----|------|----------|
| 0 | EI | Enable instruction cache |
| 1 | FI | Freeze instruction cache |
| 2 | CEI | Clear entry in instruction cache |
| 3 | CI | Clear instruction cache |
| 4-7 | — | Reserved |
| 8 | ED | Enable data cache (030 has none — ignored) |

### Instructions
- MOVEC CACR — read/write cache control
- Cache is automatically managed by instruction fetch pipeline
- CINV/CPUSH not available on 030 (040+ only)

### Required Implementation
- 16-entry tag array + valid bits
- Check on instruction fetch for cache hit
- Invalidation on CACR CI/CEI writes
- Freeze support (FI bit)

## Priority
MEDIUM — The 030 I-cache is simple but important for cycle-accurate co-simulation.
