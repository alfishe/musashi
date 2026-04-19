# Motorola 68060 Architecture Reference

## Overview

The Motorola 68060 is the final and most powerful member of the 68000 family, released in 1994. It features a superscalar architecture with dual instruction pipelines, branch prediction, and separate instruction/data caches.

## CPU Variants

| Variant | FPU | MMU | Description |
|---------|-----|-----|-------------|
| MC68060 | Yes | Yes | Full implementation |
| MC68LC060 | No | Yes | Low-cost, no FPU (physically absent or disabled) |
| MC68EC060 | No | No | Embedded controller, no FPU or MMU |

### Variant Details

**MC68060** (e.g., MC68060RC50): The full-featured version with both FPU and MMU. Most sought-after for Amiga/Atari upgrades. Even the "full" 68060 FPU is simplified compared to 68040 - transcendental functions are handled by software (FPSP).

**MC68LC060** (Low Cost): Includes the MMU but lacks the FPU. Designed for lower cost and power consumption. All FPU instructions trap to vector 11.

**MC68EC060** (Embedded Controller): Lacks both FPU and MMU. Intended for embedded systems. All FPU instructions trap to vector 11; all MMU instructions cause privilege violations or are NOPs.

### Manufacturing Notes

- **XC vs MC prefixes**: Early "XC" marked chips were sometimes full dies with factory-disabled FPU/MMU due to defects. Later "MC" production runs had these units physically absent.
- **Revision 6** (mask 71E41J): Manufactured on 0.32µm process, runs significantly cooler, commonly overclocked beyond 100 MHz.
- **Software workarounds**: Systems with LC/EC variants can use libraries like MuLibs for MMU/FPU emulation at significant performance cost.

### Emulation Implications

For emulation purposes:
- **MC68060**: PCR.DFP = 0, HAS_FPU = 1, HAS_PMMU = 1
- **MC68LC060**: PCR.DFP = 1, HAS_FPU = 0, HAS_PMMU = 1 (all FPU ops trap)
- **MC68EC060**: PCR.DFP = 1, HAS_FPU = 0, HAS_PMMU = 0 (all FPU ops trap, no translation)

## Key Architectural Differences from 68040

### New Control Registers

| Register | CR Code | Description |
|----------|---------|-------------|
| PCR | $808 | Processor Configuration Register |
| BUSCR | $008 | Bus Control Register |

#### PCR (Processor Configuration Register)

```
Bit 31-16: Revision/ID (read-only)
Bit 15:    ESS - Enable Superscalar dispatch
Bit 14:    DFP - Disable Floating Point unit
Bit 13:    Reserved
Bit 12-8:  Reserved
Bit 7:     EBE - Enable Branch cache Entry allocation
Bit 6-5:   Reserved  
Bit 4:     BTB - Branch Target Buffer enable
Bit 3:     Reserved
Bit 2:     NAD - No Align Down
Bit 1:     ESL - Enable Store Location
Bit 0:     EPE - Enable Pipelined Exceptions
```

PCR identification values:
- `$0430xxxx` - MC68060
- `$0431xxxx` - MC68LC060  
- `$0432xxxx` - MC68EC060

#### BUSCR (Bus Control Register)

Controls bus behavior; typically stubbed in emulation as it affects external bus timing.

### MMU Registers (68040/68060 Style)

The 68060 uses the same MMU register model as the 68040, NOT the 68030:

| Register | CR Code | Description |
|----------|---------|-------------|
| URP | $806 | User Root Pointer (replaces 68030 CRP) |
| SRP | $807 | Supervisor Root Pointer |
| TC | $003 | Translation Control Register |
| ITT0 | $004 | Instruction Transparent Translation 0 |
| ITT1 | $005 | Instruction Transparent Translation 1 |
| DTT0 | $006 | Data Transparent Translation 0 |
| DTT1 | $007 | Data Transparent Translation 1 |

Note: The 68030's CRP (CPU Root Pointer) with its 64-bit descriptor format is NOT used. The 68040/68060 use 32-bit URP/SRP pointers directly.

### Modified CACR Layout

The 68060 CACR differs significantly from 68040:

```
Bit 31:    EDC  - Enable Data Cache
Bit 30:    NAD  - No Allocate Mode (Data)
Bit 29:    ESB  - Enable Store Buffer
Bit 28:    DPI  - Disable CPUSH Invalidation
Bit 27-24: Reserved
Bit 23:    FOC  - Freeze On Cache miss
Bit 22:    EBC  - Enable Branch Cache
Bit 21:    CABC - Clear All Branch Cache entries
Bit 20:    CUBC - Clear User Branch Cache entries
Bit 19-16: Reserved
Bit 15:    EIC  - Enable Instruction Cache
Bit 14:    NAI  - No Allocate Mode (Instruction)
Bit 13-12: Reserved
Bit 11:    FIC  - Freeze Instruction Cache
Bit 10-8:  Reserved
Bit 7-0:   Reserved
```

## Instructions Removed from Hardware (Unimplemented)

These instructions generate exceptions on the 68060 and must be handled by software emulation packages (ISP/FPSP).

### Integer Instructions -> Vector 61 (Unimplemented Integer Instruction)

| Instruction | Description |
|-------------|-------------|
| MOVEP | Move Peripheral Data |
| CAS2 | Compare and Swap 2 (dual operand) |
| CHK2 | Check Register Against Bounds (2) |
| CMP2 | Compare Register Against Bounds (2) |
| MULS.L (64-bit result) | Signed Multiply Long to 64-bit |
| MULU.L (64-bit result) | Unsigned Multiply Long to 64-bit |
| DIVS.L (64-bit dividend) | Signed Divide Long from 64-bit |
| DIVU.L (64-bit dividend) | Unsigned Divide Long from 64-bit |

Note: 32-bit result variants of MULS.L/MULU.L and 32-bit dividend variants of DIVS.L/DIVU.L remain in hardware.

### FPU Instructions -> Vector 11 (F-Line Emulator)

Only these FPU operations remain in 68060 hardware:

| Retained in Hardware |
|---------------------|
| FABS |
| FADD |
| FBcc |
| FCMP |
| FDBcc |
| FDIV |
| FINT |
| FINTRZ |
| FMOVE (register/memory, NOT packed decimal) |
| FMOVEM (FPCR/FPSR only, NOT FPIAR) |
| FMUL |
| FNEG |
| FNOP |
| FRESTORE |
| FSAVE |
| FScc |
| FSQRT |
| FSUB |
| FTST |
| FTRAPcc |

All other FPU instructions trap to vector 11, including:
- Transcendentals: FSIN, FCOS, FTAN, FASIN, FACOS, FATAN, FATANH, FSINH, FCOSH, FTANH
- Exponentials: FETOX, FETOXM1, FTWOTOX, FTENTOX
- Logarithms: FLOGN, FLOGNP1, FLOG10, FLOG2
- Other: FMOD, FREM, FSCALE, FSGLDIV, FSGLMUL, FGETEXP, FGETMAN
- **FMOVECR** (ALL constant ROM offsets removed from silicon)
- Packed decimal FMOVE operations
- **FPIAR access** (via FMOVE or FMOVEM with FPIAR bit set)

### FPU Opcode / Parameter Differences Matrix (68040 vs 68060)

The following matrix highlights the execution status of all FPU opcodes and parameters between the 040 and the 060. Features marked "Trap v11" are unimplemented in silicon and must be handled by the Floating-Point Software Package (FPSP).

| Instruction Category | Instruction | Description | MC68040 Silicon | MC68060 Silicon | Implementation Note |
|----------------------|-------------|-------------|-----------------|-----------------|---------------------|
| **Core Arithmetic**  | `FABS`      | Absolute Value | Hardware        | Hardware        | Natively executed on both. |
|                      | `FADD`      | Add | Hardware        | Hardware        | Natively executed on both. |
|                      | `FCMP`      | Compare | Hardware        | Hardware        | Natively executed on both. |
|                      | `FDIV`      | Divide | Hardware        | Hardware        | Natively executed on both. |
|                      | `FINT`      | Integer Part | Hardware        | Hardware        | Natively executed on both. |
|                      | `FINTRZ`    | Integer Part (Round-to-Zero) | Hardware | Hardware | Natively executed on both. |
|                      | `FMUL`      | Multiply | Hardware        | Hardware        | Natively executed on both. |
|                      | `FNEG`      | Negate | Hardware        | Hardware        | Natively executed on both. |
|                      | `FSQRT`     | Square Root | Hardware        | Hardware        | Natively executed on both. |
|                      | `FSUB`      | Subtract | Hardware        | Hardware        | Natively executed on both. |
|                      | `FTST`      | Test | Hardware        | Hardware        | Natively executed on both. |
| **Control Flow**     | `FBcc`      | Branch on Condition | Hardware   | Hardware        | Natively executed on both. |
|                      | `FDBcc`     | Test, Decrement, and Branch | Hardware | Hardware | Natively executed on both. |
|                      | `FScc`      | Set on Condition | Hardware      | Hardware        | Natively executed on both. |
|                      | `FTRAPcc`   | Trap on Condition | Hardware     | Hardware        | Natively executed on both. |
|                      | `FNOP`      | No Operation | Hardware        | Hardware        | Natively executed on both. |
| **Data Movement**    | `FMOVE` (Reg/Mem) | Move Floating-Point Data | Hardware | Hardware | Excludes Packed Decimal format. |
| **Register Movement**| `FMOVEM`    | Move Multiple / Control Registers | Hardware | Hardware | Excludes `FPIAR` access. |
| **Context Switch**   | `FSAVE`     | Save Internal State | Hardware   | Hardware        | Internal state frame sizes/IDs differ. |
|                      | `FRESTORE`  | Restore Internal State | Hardware| Hardware        | Internal state frame sizes/IDs differ. |
| **Constants**        | `FMOVECR`   | Load internal ROM constants (e.g., Pi, e) into FPU | Partial Hardware (6 of 22) | **Trap v11** (All 22) | 040 natively supports 6 offsets, trapping the rest. 060 drops Native ROM entirely and traps all. |
| **Single Prec. Math**| `FSGLDIV`   | Single Precision Divide | Hardware | **Trap v11**    | Dropped to simplify 060's FPU pipeline. |
|                      | `FSGLMUL`   | Single Precision Multiply | Hardware | **Trap v11** | Dropped to simplify 060's FPU pipeline. |
| **Transcendentals**  | `FSIN`      | Sine | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FCOS`      | Cosine | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FTAN`      | Tangent | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FASIN`     | Arc Sine | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FACOS`     | Arc Cosine | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FATAN`     | Arc Tangent | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FATANH`    | Hyperbolic Arc Tangent | Trap v11| Trap v11        | Emulated via FPSP. |
|                      | `FSINH`     | Hyperbolic Sine | Trap v11     | Trap v11        | Emulated via FPSP. |
|                      | `FCOSH`     | Hyperbolic Cosine | Trap v11   | Trap v11        | Emulated via FPSP. |
|                      | `FTANH`     | Hyperbolic Tangent | Trap v11  | Trap v11        | Emulated via FPSP. |
| **Exponentials**     | `FETOX`     | e^x | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FETOXM1`   | e^x - 1 | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FTWOTOX`   | 2^x | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FTENTOX`   | 10^x | Trap v11        | Trap v11        | Emulated via FPSP. |
| **Logarithms**       | `FLOGN`     | ln(x) | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FLOGNP1`   | ln(x+1) | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FLOG10`    | log10(x) | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FLOG2`     | log2(x) | Trap v11        | Trap v11        | Emulated via FPSP. |
| **Misc Math**        | `FMOD`      | Modulo Remainder | Trap v11     | Trap v11        | Emulated via FPSP. |
|                      | `FREM`      | IEEE Remainder | Trap v11      | Trap v11        | Emulated via FPSP. |
|                      | `FSCALE`    | Scale Exponent | Trap v11      | Trap v11        | Emulated via FPSP. |
|                      | `FGETEXP`   | Get Exponent | Trap v11        | Trap v11        | Emulated via FPSP. |
|                      | `FGETMAN`   | Get Mantissa | Trap v11        | Trap v11        | Emulated via FPSP. |
| **FPIAR Access**     | `FMOVE`/`FMOVEM` | Access FPIAR Register | Hardware | **Trap v11** | `FPIAR` does not exist on 060. |
| **Packed Decimal**   | `FMOVE` (Packed) | Move Packed Decimal Data | Trap v11 | Trap v11 | Emulated via FPSP. |

### FMOVECR (Move Constant ROM) Execution Details

The `FMOVECR` instruction is used to instantly load well-known mathematical constants (e.g., Pi, *e*, or 10.0) from a predefined physical hardware ROM into a floating-point data register without requiring memory fetches. The instruction takes a 7-bit offset code (`$00` through `$40`). 

According to the **M68000 Family Programmer's Reference Manual (PRM)**, there are 22 defined constant offsets. However, physical implementation of this ROM varies drastically between CPU generations:

**MC68040 Implementation:**
* To conserve physical silicon area, Motorola reduced the physical internal ROM to a tiny subset. As detailed in the 040 programming manuals, the 68040 hardware **natively handles only 6 constants**:
  * `$00`: Pi (π)
  * `$0B`: log10(2)
  * `$0C`: e
  * `$0D`: log2(e)
  * `$0E`: log10(e)
  * `$0F`: 0.0
* If software attempts to load any of the remaining 16 valid offsets (such as `$32` for `10.0`), the 68040 hardware throws an **F-Line Unimplemented Instruction Exception (Vector 11)**. The host operating system's FPSP must intercept this exception and supply the constant via software.

**MC68060 Implementation:**
* As documented in the **MC68060 User's Manual** (specifically regarding Instruction Execution Timing and Exception Processing), Motorola completely removed the constant ROM hardware to further streamline the superscalar FPU pipeline.
* As a result, the 68060 FPU possesses **zero native constants**.
* Execution of **any** `FMOVECR` instruction code unconditionally triggers an **F-Line Exception (Vector 11)** trap. The 68060 depends entirely on the OS-installed FPSP to emulate all 22 constant loads via software mapping.

#### Complete `FMOVECR` Constant ROM Table

The Motorola FPU specification defines exactly 22 constant offsets. The internally documented 96-bit Extended-Precision hexadecimal values (including the 16-bit zero padding used in memory) are shown below alongside their implementation status for the 68040 and 68060.

| Offset | Constant Description | Float/Double Approximation | Extended-Precision Hex | 68040 Status | 68060 Status |
|--------|----------------------|----------------------------|------------------------|--------------|--------------|
| `$00` | Pi (π) | `3.14159265358979` | `$4000 0000 C90F DAA2 2168 C235` | NATIVE HW | Trap (FPSP) |
| `$0B` | log10(2) | `0.30102999566398` | `$3FFD 0000 9A20 9A84 FBCF F798` | NATIVE HW | Trap (FPSP) |
| `$0C` | e (Euler's Number) | `2.71828182845904` | `$4000 0000 ADF8 5458 A2BB 4A9B` | NATIVE HW | Trap (FPSP) |
| `$0D` | log2(e) | `1.44269504088896` | `$3FFF 0000 B8AA 3B29 5C17 F0BC` | NATIVE HW | Trap (FPSP) |
| `$0E` | log10(e) | `0.43429448190325` | `$3FFD 0000 DE5B D8A9 3728 7195` | NATIVE HW | Trap (FPSP) |
| `$0F` | 0.0 | `0.0` | `$0000 0000 0000 0000 0000 0000` | NATIVE HW | Trap (FPSP) |
| `$30` | ln(2) | `0.69314718055994` | `$3FFE 0000 B172 17F7 D1CF 79AC` | Trap (FPSP) | Trap (FPSP) |
| `$31` | ln(10) | `2.30258509299404` | `$4000 0000 935D 8DDD AAA8 AC17` | Trap (FPSP) | Trap (FPSP) |
| `$32` | 10^0 | `1.0` | `$3FFF 0000 8000 0000 0000 0000` | Trap (FPSP) | Trap (FPSP) |
| `$33` | 10^1 | `10.0` | `$4002 0000 A000 0000 0000 0000` | Trap (FPSP) | Trap (FPSP) |
| `$34` | 10^2 | `100.0` | `$4005 0000 C800 0000 0000 0000` | Trap (FPSP) | Trap (FPSP) |
| `$35` | 10^4 | `10000.0` | `$400C 0000 9C40 0000 0000 0000` | Trap (FPSP) | Trap (FPSP) |
| `$36` | 10^8 | `1.0e+08` | `$4019 0000 BEBC 2000 0000 0000` | Trap (FPSP) | Trap (FPSP) |
| `$37` | 10^16 | `1.0e+16` | `$4034 0000 8E1B C9BF 0400 0000` | Trap (FPSP) | Trap (FPSP) |
| `$38` | 10^32 | `1.0e+32` | `$4069 0000 9DC5 ADA8 2B70 B59E` | Trap (FPSP) | Trap (FPSP) |
| `$39` | 10^64 | `1.0e+64` | `$40D3 0000 C278 1F49 FFCD FAED` | Trap (FPSP) | Trap (FPSP) |
| `$3A` | 10^128 | `1.0e+128` | `$41A8 0000 93BA 47C9 80E9 8CE0` | Trap (FPSP) | Trap (FPSP) |
| `$3B` | 10^256 | `1.0e+256` | `$4351 0000 AA7E EBFB 9DF9 DE8E` | Trap (FPSP) | Trap (FPSP) |
| `$3C` | 10^512 | `1.0e+512` | `$46A3 0000 E319 A0AE A60E 91C7` | Trap (FPSP) | Trap (FPSP) |
| `$3D` | 10^1024 | `1.0e+1024` | `$4D48 0000 C976 7586 8175 0C17` | Trap (FPSP) | Trap (FPSP) |
| `$3E` | 10^2048 | `1.0e+2048` | `$5A92 0000 9E8B 3B5D C53D 5DE5` | Trap (FPSP) | Trap (FPSP) |
| `$3F` | 10^4096 | `1.0e+4096` | `$7525 0000 C460 5202 8A20 979B` | Trap (FPSP) | Trap (FPSP) |

The 68060 completely eliminated the FPIAR (Floating-Point Instruction Address Register) from hardware. Unlike the 68040 which maintained FPIAR for exception handling, the 68060 requires software emulation:

- Any FMOVE to/from FPIAR traps to vector 11
- Any FMOVEM with the FPIAR control register bit set traps to vector 11
- The FPSP must maintain a software shadow of FPIAR for compatibility

This is a significant difference from 68040 and must be checked in FMOVEM control register handling.

## New Instructions

### PLPA - Physical Load Physical Address

```
PLPA (An)
```

Privileged instruction that probes the MMU for address translation. Unlike PTEST, the result is placed in a CPU-accessible location. Used by operating systems for DMA setup.

Opcode: `1111 0101 00xx x000`

### LPSTOP - Low-Power Stop

```
LPSTOP #<data>
```

Enters low-power mode with specified status register value. Similar to STOP but with additional power management. The CPU remains stopped until an interrupt of sufficient priority occurs.

Opcode: `1111 1000 0000 0000` followed by `0000 0001 1100 0000` and 16-bit immediate

## Exception Stack Frames

The 68060 uses different stack frame formats than earlier processors.

### Frame Format $0 (4-word Normal)

Used for most exceptions on 68060:

```
+0: Status Register (16-bit)
+2: Program Counter (32-bit, high word)
+4: Program Counter (low word)
+6: Format/Vector Word: $0xxx
```

### Frame Format $4 (8-word Access Fault)

Used for access faults (bus error, address error):

```
+0:  Status Register
+2:  Program Counter (high)
+4:  Program Counter (low)
+6:  Format/Vector: $4xxx
+8:  Effective Address (high)
+10: Effective Address (low)
+12: Fault Status Longword (high)
+14: Fault Status Longword (low)
```

Fault Status Longword bits:
- Bit 31-28: Reserved
- Bit 27: MA - Misaligned Access
- Bit 26: ATC - ATC Fault
- Bit 25: LK - Locked Transfer
- Bit 24: RW - Read/Write (0=write, 1=read)
- Bit 23-21: SIZE - Transfer size
- Bit 20-16: TT - Transfer Type
- Bit 15-0: Reserved

## Software Emulation Packages

Motorola provides software packages to handle unimplemented instructions:

### ISP (Integer Software Package)

Handles vector 61 traps for:
- MOVEP
- CAS2
- CHK2/CMP2
- 64-bit multiply/divide

### FPSP (Floating-Point Software Package)

Handles vector 11 traps for:
- Unimplemented FPU instructions
- Packed decimal conversions
- Denormalized number handling

## References

1. MC68060 User's Manual (MC68060UM/AD) - Motorola, 1994
2. M68060 Software Package User's Manual - Motorola
3. M68000 Family Programmer's Reference Manual - Motorola
4. NetBSD/m68k 68060 support source code
5. Linux/m68k arch/m68k/ifpsp060/ and arch/m68k/kernel/
6. AmigaOS 68060.library documentation
