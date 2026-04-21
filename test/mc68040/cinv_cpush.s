.include "entry_040_mmu.s"
* OPCODE: CINV/CPUSH
* Test that CINV and CPUSH cache instructions execute without crashing
* These are no-ops in an emulator without cache, but should not trap

op_CINV_CPUSH:

    * Set up A0 with a valid address for line/page operations
    lea     0x10000, %a0

    * ============================================
    * Test CINVL (Cache Invalidate Line)
    * Opcode: F408 + (cache<<6) + reg
    * cache: 1=data, 2=inst, 3=both
    * ============================================

    * CINVL data cache, (A0)
    .word   0xF448              | CINVL DC,(A0)

    * CINVL instruction cache, (A0)
    .word   0xF488              | CINVL IC,(A0)

    * CINVL both caches, (A0)
    .word   0xF4C8              | CINVL BC,(A0)

    * ============================================
    * Test CINVP (Cache Invalidate Page)
    * Opcode: F410 + (cache<<6) + reg
    * ============================================

    * CINVP data cache, (A0)
    .word   0xF450              | CINVP DC,(A0)

    * CINVP instruction cache, (A0)
    .word   0xF490              | CINVP IC,(A0)

    * CINVP both caches, (A0)
    .word   0xF4D0              | CINVP BC,(A0)

    * ============================================
    * Test CINVA (Cache Invalidate All)
    * Opcode: F418 + (cache<<6)
    * ============================================

    * CINVA data cache
    .word   0xF458              | CINVA DC

    * CINVA instruction cache
    .word   0xF498              | CINVA IC

    * CINVA both caches
    .word   0xF4D8              | CINVA BC

    * ============================================
    * Test CPUSHL (Cache Push Line)
    * Opcode: F428 + (cache<<6) + reg
    * ============================================

    * CPUSHL data cache, (A0)
    .word   0xF468              | CPUSHL DC,(A0)

    * CPUSHL instruction cache, (A0)
    .word   0xF4A8              | CPUSHL IC,(A0)

    * CPUSHL both caches, (A0)
    .word   0xF4E8              | CPUSHL BC,(A0)

    * ============================================
    * Test CPUSHP (Cache Push Page)
    * Opcode: F430 + (cache<<6) + reg
    * ============================================

    * CPUSHP data cache, (A0)
    .word   0xF470              | CPUSHP DC,(A0)

    * CPUSHP instruction cache, (A0)
    .word   0xF4B0              | CPUSHP IC,(A0)

    * CPUSHP both caches, (A0)
    .word   0xF4F0              | CPUSHP BC,(A0)

    * ============================================
    * Test CPUSHA (Cache Push All)
    * Opcode: F438 + (cache<<6)
    * ============================================

    * CPUSHA data cache
    .word   0xF478              | CPUSHA DC

    * CPUSHA instruction cache
    .word   0xF4B8              | CPUSHA IC

    * CPUSHA both caches
    .word   0xF4F8              | CPUSHA BC

    * If we get here, all instructions executed without trapping
    rts
