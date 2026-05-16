#include "../../framework/m68k_fixture.h"

/* ========================================================================
 * Phase 3 cycle-count verification tests.
 *
 * Tests cover three categories of non-AERR cycle mismatches:
 *   A) Byte immediate +2 overcount (ADD/AND/OR/SUB #imm.b,Dn)
 *   B) ADDQ.w An -4 undercount
 *   C) Register-to-register long undercount (OR.l Dn,Dn, SUBA.l Dn/An,An)
 * ======================================================================== */

/* ---- Category A: Byte immediate overcount ---- */

class CycleByteImmTest : public M68kTest {};

TEST_F(CycleByteImmTest, ADD_b_ImmDn) {
    // ADD.b #$12,D0: opcode 0xD03C (ADD byte, er, #imm)
    write_mem_16(0x1000, 0xD03C);
    write_mem_16(0x1002, 0x0012);  // immediate byte (in low byte of word)
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x00000000);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

TEST_F(CycleByteImmTest, AND_b_ImmDn) {
    // AND.b #$FF,D0: opcode 0xC03C
    write_mem_16(0x1000, 0xC03C);
    write_mem_16(0x1002, 0x00FF);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x000000FF);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

TEST_F(CycleByteImmTest, OR_b_ImmDn) {
    // OR.b #$55,D0: opcode 0x803C
    write_mem_16(0x1000, 0x803C);
    write_mem_16(0x1002, 0x0055);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x000000AA);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

TEST_F(CycleByteImmTest, SUB_b_ImmDn) {
    // SUB.b #$01,D0: opcode 0x903C
    write_mem_16(0x1000, 0x903C);
    write_mem_16(0x1002, 0x0001);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x00000010);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

/* ---- Category B: ADDQ.w An undercount ---- */

class CycleAddqTest : public M68kTest {};

TEST_F(CycleAddqTest, ADDQ_w_An) {
    // ADDQ.w #1,A0: opcode 0x5048
    write_mem_16(0x1000, 0x5048);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A0, 0x00001000);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

TEST_F(CycleAddqTest, ADDQ_w_Dn_notAffected) {
    // ADDQ.w #1,D0: opcode 0x5040 — should stay at 4 cycles
    write_mem_16(0x1000, 0x5040);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x00000000);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 4);
}

/* ---- Category C: Register long undercount ---- */

class CycleRegLongTest : public M68kTest {};

TEST_F(CycleRegLongTest, OR_l_DnDn) {
    // OR.l D2,D5 (er mode): opcode 0x8A82
    // 1000_101_010_000_010 = OR, reg=5, opmode=010(long,er), EA=Dn(D2)
    write_mem_16(0x1000, 0x8A82);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D2, 0xFF00FF00);
    m68k_set_reg(M68K_REG_D5, 0x00FF0000);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

TEST_F(CycleRegLongTest, SUBA_l_DnAn) {
    // SUBA.l D0,A0: opcode 0x91C0
    // 1001_000_111_000_000 = SUBA, reg=0, opmode=111(long), EA=Dn(D0)
    write_mem_16(0x1000, 0x91C0);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_D0, 0x00000008);
    m68k_set_reg(M68K_REG_A0, 0x00001000);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}

TEST_F(CycleRegLongTest, SUBA_l_AnAn) {
    // SUBA.l A2,A0: opcode 0x91CA
    // 1001_000_111_001_010 = SUBA, reg=0, opmode=111(long), EA=An(A2)
    write_mem_16(0x1000, 0x91CA);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_set_reg(M68K_REG_A2, 0x00000100);
    m68k_set_reg(M68K_REG_A0, 0x00001000);
    int cycles = m68k_execute(1);
    EXPECT_EQ(cycles, 8);
}
