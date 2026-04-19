# Musashi 68060 Support Documentation

This directory contains documentation for adding Motorola 68060 CPU support to the Musashi emulator.

## Documents

| Document | Description |
|----------|-------------|
| [68060-architecture.md](68060-architecture.md) | Reference documentation for 68060 CPU architecture, registers, and instruction changes |
| [68060-gap-analysis.md](68060-gap-analysis.md) | Analysis of current Musashi state vs 68060 requirements |
| [implementation-plan.md](implementation-plan.md) | Phased implementation plan with tasks and deliverables |
| [technical-design-document.md](technical-design-document.md) | Complete TDD with file-by-file changes, data structures, and APIs |

## Quick Reference

### CPU Variants

| Variant | FPU | MMU | PCR ID |
|---------|-----|-----|--------|
| MC68060 | Yes | Yes | $0430xxxx |
| MC68LC060 | No | Yes | $0431xxxx |
| MC68EC060 | No | No | $0432xxxx |

### Key Changes Summary

```
m68k.h        - Add CPU type enums, register enums
m68kconf.h    - Add M68K_EMULATE_060 config
m68kcpu.h     - Add CPU_TYPE_060, PCR/BUSCR, macros
m68kcpu.c     - CPU init, exception frames, MOVEC
m68k_in.c     - 6th column, trapped ops, PLPA/LPSTOP
m68kmake.c    - NUM_CPU_TYPES=6, trap markers
m68kfpu.c     - Hardware op partitioning, trap logic
m68kmmu.h     - PLPA support, 060 PTEST
```

### Trapped Instructions (Vector 61)

- MOVEP
- CAS2
- CHK2/CMP2
- MULS.L/MULU.L (64-bit result)
- DIVS.L/DIVU.L (64-bit dividend)

### Trapped FPU Operations (Vector 11)

All FPU operations EXCEPT:
- FABS, FADD, FBcc, FCMP, FDBcc, FDIV
- FINT, FINTRZ, FMOVE (NOT packed decimal), FMUL
- FMOVEM (FPCR/FPSR only, NOT FPIAR)
- FNEG, FNOP, FRESTORE, FSAVE, FScc
- FSQRT, FSUB, FTST, FTRAPcc

**Always trapped on 68060:**
- FMOVECR (ALL constant ROM offsets removed)
- FPIAR access (register eliminated from hardware)
- Packed decimal FMOVE operations
- All transcendental functions (FSIN, FCOS, etc.)

## Estimated Effort

| Phase | Duration |
|-------|----------|
| Infrastructure | 2-3 days |
| Instruction table | 3-5 days |
| Exception frames | 2-3 days |
| FPU split | 3-4 days |
| MMU/New instructions | 2-3 days |
| Variants | 1 day |
| Testing | 5-10 days |
| **Total** | **18-29 days** |

## External References

- [MC68060 User's Manual](https://www.nxp.com/docs/en/reference-manual/MC68060UM.pdf)
- [M68000 Programmer's Reference](https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf)
- [Musashi GitHub](https://github.com/kstenerud/Musashi)
