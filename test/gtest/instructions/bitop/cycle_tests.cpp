#include "../../framework/m68k_fixture.h"

/* ========================================================================
 * Cycle-count verification tests for TAS and bit manipulation instructions.
 *
 * These tests verify that m68k_execute(1) returns the correct number of
 * clock cycles for the 68000 CPU, matching real hardware as documented in
 * the M68000 Family Programmer's Reference Manual.
 *
 * The cycle table is built by m68kmake from m68k_in.c entries plus
 * per-EA-mode costs from g_ea_cycle_table[].  Changing base cycle values
 * in m68k_in.c affects all EA variants, so each test covers multiple modes.
 * ======================================================================== */

class CycleTasTest : public M68kTest {};

// ---------- TAS Dn ----------

TEST_F(CycleTasTest, TAS_Dn) {
    // TAS D0: opcode 0x4AC0
    write_mem_16(0x1000, 0x4AC0);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x00);
    int cycles = m68k_execute(1);
    // Real 68000: TAS Dn = 4 cycles
    EXPECT_EQ(cycles, 4);
}

// ---------- TAS (An) ----------

TEST_F(CycleTasTest, TAS_AI) {
    // TAS (A0): opcode 0x4AD0
    write_mem_16(0x1000, 0x4AD0);
    write_mem_8(0x2000, 0x00);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    int cycles = m68k_execute(1);
    // Real 68000: TAS (An) = 14 cycles
    EXPECT_EQ(cycles, 14);
}

// ---------- TAS (An)+ ----------

TEST_F(CycleTasTest, TAS_PI) {
    // TAS (A0)+: opcode 0x4AD8
    write_mem_16(0x1000, 0x4AD8);
    write_mem_8(0x2000, 0x00);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    int cycles = m68k_execute(1);
    // Real 68000: TAS (An)+ = 14 cycles
    EXPECT_EQ(cycles, 14);
}

// ---------- TAS -(An) ----------

TEST_F(CycleTasTest, TAS_PD) {
    // TAS -(A0): opcode 0x4AE0
    write_mem_16(0x1000, 0x4AE0);
    write_mem_8(0x1FFF, 0x00);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    int cycles = m68k_execute(1);
    // Real 68000: TAS -(An) = 16 cycles
    EXPECT_EQ(cycles, 16);
}

// ---------- TAS (d16,An) ----------

TEST_F(CycleTasTest, TAS_DI) {
    // TAS (d16,A0): opcode 0x4AE8, displacement = 0x0010
    write_mem_16(0x1000, 0x4AE8);
    write_mem_16(0x1002, 0x0010);  // d16 = +16
    write_mem_8(0x2010, 0x00);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    int cycles = m68k_execute(1);
    // Real 68000: TAS (d16,An) = 18 cycles
    EXPECT_EQ(cycles, 18);
}

// ---------- TAS (xxx).w ----------

TEST_F(CycleTasTest, TAS_AW) {
    // TAS ($3000).w: opcode 0x4AF8
    write_mem_16(0x1000, 0x4AF8);
    write_mem_16(0x1002, 0x3000);  // absolute word address
    write_mem_8(0x3000, 0x00);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    int cycles = m68k_execute(1);
    // Real 68000: TAS (xxx).w = 18 cycles
    EXPECT_EQ(cycles, 18);
}

/* ========================================================================
 * BCHG / BCLR / BSET / BTST cycle tests
 *
 * Real 68000 cycle counts (from PRM):
 *
 *             Dn,Dn    Dn,(ea)   #imm,Dn   #imm,(ea)
 * BCHG          6       base+EA    10       base+EA
 * BCLR          6       base+EA    10       base+EA
 * BSET          6       base+EA    10       base+EA
 * BTST          6       base+EA    10       base+EA
 *
 * Where "base+EA" means the register-to-memory variant uses
 * g_ea_cycle_table to add EA-specific costs.
 * ======================================================================== */

class CycleBitopTest : public M68kTest {};

// BCHG Dn,Dn — 6 cycles base + 2 when bit >= 16
TEST_F(CycleBitopTest, BCHG_DnDn) {
    // BCHG D0,D1: opcode 0x0141 (Dn=0, op=BCHG=101, Dn dest=1)
    write_mem_16(0x1000, 0x0141);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x01);  // bit 1 < 16, no +2
    m68k_set_reg(M68K_REG_D1, 0x00);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 6);
}

// BCHG Dn,(An) — real 68000 = 8 + 4 = 12 cycles
TEST_F(CycleBitopTest, BCHG_Dn_AI) {
    // BCHG D0,(A0): opcode 0x0150
    write_mem_16(0x1000, 0x0150);
    write_mem_8(0x2000, 0x00);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    m68k_set_reg(M68K_REG_D0, 0x01);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 12);
}

// BCLR Dn,Dn — 8 cycles base + 2 when bit >= 16
TEST_F(CycleBitopTest, BCLR_DnDn) {
    // BCLR D0,D1: opcode 0x0181
    write_mem_16(0x1000, 0x0181);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x01);  // bit 1 < 16, no +2
    m68k_set_reg(M68K_REG_D1, 0xFF);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

// BCLR Dn,(An) — real 68000 = 8 + 4 = 12 cycles
TEST_F(CycleBitopTest, BCLR_Dn_AI) {
    // BCLR D0,(A0): opcode 0x0190
    write_mem_16(0x1000, 0x0190);
    write_mem_8(0x2000, 0xFF);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    m68k_set_reg(M68K_REG_D0, 0x01);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 12);
}

// BSET Dn,Dn — 6 cycles base + 2 when bit >= 16
TEST_F(CycleBitopTest, BSET_DnDn) {
    // BSET D0,D1: opcode 0x01C1
    write_mem_16(0x1000, 0x01C1);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x01);  // bit 1 < 16, no +2
    m68k_set_reg(M68K_REG_D1, 0x00);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 6);
}

// BSET Dn,(An) — real 68000 = 8 + 4 = 12 cycles
TEST_F(CycleBitopTest, BSET_Dn_AI) {
    // BSET D0,(A0): opcode 0x01D0
    write_mem_16(0x1000, 0x01D0);
    write_mem_8(0x2000, 0x00);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    m68k_set_reg(M68K_REG_D0, 0x01);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 12);
}

// BTST Dn,Dn — real 68000 = 6 cycles
TEST_F(CycleBitopTest, BTST_DnDn) {
    // BTST D0,D1: opcode 0x0101
    write_mem_16(0x1000, 0x0101);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x01);
    m68k_set_reg(M68K_REG_D1, 0xFF);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 6);
}

// BTST Dn,(An) — real 68000 = 4 + 4 = 8 cycles
TEST_F(CycleBitopTest, BTST_Dn_AI) {
    // BTST D0,(A0): opcode 0x0110
    write_mem_16(0x1000, 0x0110);
    write_mem_8(0x2000, 0xFF);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x2000);
    m68k_set_reg(M68K_REG_D0, 0x01);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}
