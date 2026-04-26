#include "../../framework/m68k_fixture.h"

class BcdArithmeticTest : public M68kTest {
};

TEST_F(BcdArithmeticTest, AbcdFlagTransitionVFlag) {
    // Test ABCD D0, D1  (opcode: 0xC300)
    // D0 = 0x39, D1 = 0x42, X=0. 
    // Unadjusted = 0x39 + 0x42 = 0x7B. (MSB = 0).
    // Low nibble 9 + 2 = 11 (>9) -> +6.
    // Adjusted = 0x7B + 6 = 0x81 (MSB = 1).
    // Silicon V-flag formula: (!(unadj & 0x80) && (res & 0x80))
    // V flag MUST be set!

    m68k_set_reg(M68K_REG_D0, 0x39);
    m68k_set_reg(M68K_REG_D1, 0x42);
    m68k_set_reg(M68K_REG_SR, 0x2000); // Clear all flags, X=0

    // Opcode: ABCD D0, D1
    write_mem_16(0x1000, 0xC300);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6); // ABCD takes 6 cycles

    uint32_t sr = m68k_get_reg(NULL, M68K_REG_SR);
    uint32_t d1 = m68k_get_reg(NULL, M68K_REG_D1);

    EXPECT_EQ(d1 & 0xFF, 0x81);
    EXPECT_TRUE(sr & 0x02) << "V flag should be set for 0x7B -> 0x81 transition";
    EXPECT_TRUE(sr & 0x08) << "N flag should be set for 0x81";
}

TEST_F(BcdArithmeticTest, SbcdFlagTransitionVFlag) {
    // Test SBCD D0, D1 (opcode: 0x8300)
    // We want Unadj MSB=1, Adjusted MSB=0.
    // dst = 0x01, src = 0x82, X=0.
    // Unadj = 0x01 - 0x82 = 0x7F (MSB=0). Not what we want.
    
    // Let's try: dst = 0x20, src = 0x41.
    // Unadj = 0x20 - 0x41 = 0xDF (MSB=1).
    // Low nibble borrow (0-1) -> -6.
    // 0xDF - 0x06 = 0xD9.
    // Decimal borrow (0x20 < 0x41) -> -0x60.
    // 0xD9 - 0x60 = 0x79 (MSB=0).
    // Silicon V-flag formula: ((unadj & 0x80) && !(res & 0x80))
    // V flag MUST be set!

    m68k_set_reg(M68K_REG_D0, 0x41);
    m68k_set_reg(M68K_REG_D1, 0x20);
    m68k_set_reg(M68K_REG_SR, 0x2000); // Clear all flags, X=0

    // Opcode: SBCD D0, D1
    write_mem_16(0x1000, 0x8300);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);

    uint32_t sr = m68k_get_reg(NULL, M68K_REG_SR);
    uint32_t d1 = m68k_get_reg(NULL, M68K_REG_D1);

    EXPECT_EQ(d1 & 0xFF, 0x79);
    EXPECT_TRUE(sr & 0x02) << "V flag should be set for 0xDF -> 0x79 transition";
    EXPECT_FALSE(sr & 0x08) << "N flag should be clear for 0x79";
}

TEST_F(BcdArithmeticTest, NbcdFlagTransitionVFlag) {
    // Test NBCD D0 (opcode: 0x4800)
    // NBCD is 0 - src - X.
    // Let src = 0x41, X=0.
    // Unadj = 0 - 0x41 = 0xBF (MSB=1).
    // Low nibble borrow (0-1) -> -6.
    // 0xBF - 0x06 = 0xB9.
    // Decimal borrow (0 < 0x41) -> -0x60.
    // 0xB9 - 0x60 = 0x59 (MSB=0).
    // V flag MUST be set!

    m68k_set_reg(M68K_REG_D0, 0x41);
    m68k_set_reg(M68K_REG_SR, 0x2000);

    // Opcode: NBCD D0
    write_mem_16(0x1000, 0x4800);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);

    uint32_t sr = m68k_get_reg(NULL, M68K_REG_SR);
    uint32_t d0 = m68k_get_reg(NULL, M68K_REG_D0);

    EXPECT_EQ(d0 & 0xFF, 0x59);
    EXPECT_TRUE(sr & 0x02) << "V flag should be set for 0xBF -> 0x59 transition";
}

TEST_F(BcdArithmeticTest, AbcdZFlagHandling) {
    // ABCD does not SET the Z flag if the result is 0. 
    // It only CLEARS it if the result is NOT 0.
    // This is used for multiple-precision BCD arithmetic.

    // Case 1: Pre-set Z=0, Result=0. Z should remain 0.
    m68k_set_reg(M68K_REG_D0, 0x00);
    m68k_set_reg(M68K_REG_D1, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2000); // Z=0 (SR bit 2)
    write_mem_16(0x1000, 0xC300); // ABCD D0, D1
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);
    EXPECT_FALSE(m68k_get_reg(NULL, M68K_REG_SR) & 0x04) << "Z flag should remain 0 if result is 0 and it was 0";

    // Case 2: Pre-set Z=1, Result=0. Z should remain 1.
    m68k_set_reg(M68K_REG_D0, 0x00);
    m68k_set_reg(M68K_REG_D1, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2004); // Z=1
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);
    EXPECT_TRUE(m68k_get_reg(NULL, M68K_REG_SR) & 0x04) << "Z flag should remain 1 if result is 0 and it was 1";

    // Case 3: Pre-set Z=1, Result=1. Z should be CLEARED.
    m68k_set_reg(M68K_REG_D0, 0x01);
    m68k_set_reg(M68K_REG_D1, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2004); // Z=1
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);
    EXPECT_FALSE(m68k_get_reg(NULL, M68K_REG_SR) & 0x04) << "Z flag should be cleared if result is non-zero";
}

TEST_F(BcdArithmeticTest, AbcdCarryAndExtend) {
    // 99 + 1 = 00 with Carry and Extend set.
    m68k_set_reg(M68K_REG_D0, 0x01);
    m68k_set_reg(M68K_REG_D1, 0x99);
    m68k_set_reg(M68K_REG_SR, 0x2000); // X=0
    write_mem_16(0x1000, 0xC300); // ABCD D0, D1
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);

    uint32_t sr = m68k_get_reg(NULL, M68K_REG_SR);
    uint32_t d1 = m68k_get_reg(NULL, M68K_REG_D1);

    EXPECT_EQ(d1 & 0xFF, 0x00);
    EXPECT_TRUE(sr & 0x01) << "Carry flag should be set for 99+1";
    EXPECT_TRUE(sr & 0x10) << "Extend flag should be set for 99+1";
}

TEST_F(BcdArithmeticTest, SbcdBorrowAndExtend) {
    // 00 - 1 = 99 with Carry (Borrow) and Extend set.
    m68k_set_reg(M68K_REG_D0, 0x01);
    m68k_set_reg(M68K_REG_D1, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2000); // X=0
    write_mem_16(0x1000, 0x8300); // SBCD D0, D1
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);

    uint32_t sr = m68k_get_reg(NULL, M68K_REG_SR);
    uint32_t d1 = m68k_get_reg(NULL, M68K_REG_D1);

    EXPECT_EQ(d1 & 0xFF, 0x99);
    EXPECT_TRUE(sr & 0x01) << "Carry (borrow) flag should be set for 00-1";
    EXPECT_TRUE(sr & 0x10) << "Extend (borrow) flag should be set for 00-1";
}

TEST_F(BcdArithmeticTest, AbcdPreDecrementAddressing) {
    // Test ABCD -(A0), -(A1) (opcode: 0xC308)
    // A0 points to 0x2001, A1 points to 0x2003.
    // After dec: A0=0x2000, A1=0x2002.
    // Mem[0x2000]=0x15, Mem[0x2002]=0x26.
    // Result Mem[0x2002] = 0x15+0x26 = 0x41.

    write_mem_8(0x2000, 0x15);
    write_mem_8(0x2002, 0x26);
    
    m68k_set_reg(M68K_REG_A0, 0x2001);
    m68k_set_reg(M68K_REG_A1, 0x2003);
    m68k_set_reg(M68K_REG_SR, 0x2000);

    write_mem_16(0x1000, 0xC308); 
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(10); // -(An) mode takes more cycles (10)

    EXPECT_EQ(m68k_get_reg(NULL, M68K_REG_A0), 0x2000);
    EXPECT_EQ(m68k_get_reg(NULL, M68K_REG_A1), 0x2002);
    EXPECT_EQ(read_mem_8(0x2002), 0x41);
}

TEST_F(BcdArithmeticTest, NbcdZeroHandling) {
    // NBCD 0 with X=0 should be 0.
    // NBCD 0 with X=1 should be 99.
    
    // Case 1: X=0
    m68k_set_reg(M68K_REG_D0, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2000);
    write_mem_16(0x1000, 0x4800); 
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);
    EXPECT_EQ(m68k_get_reg(NULL, M68K_REG_D0) & 0xFF, 0x00);

    // Case 2: X=1
    m68k_set_reg(M68K_REG_D0, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2010); // X=1
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);
    EXPECT_EQ(m68k_get_reg(NULL, M68K_REG_D0) & 0xFF, 0x99);
    EXPECT_TRUE(m68k_get_reg(NULL, M68K_REG_SR) & 0x01) << "Carry should be set for 0-0-1";
}

TEST_F(BcdArithmeticTest, BcdIllegalInputs) {
    // 0x0A is illegal BCD. 
    // Let's see what Musashi does with 0x0A + 0x00.
    m68k_set_reg(M68K_REG_D0, 0x0A);
    m68k_set_reg(M68K_REG_D1, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2000);
    write_mem_16(0x1000, 0xC300); 
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);

    uint32_t d1 = m68k_get_reg(NULL, M68K_REG_D1) & 0xFF;
    // On silicon, 0x0A + 0x00 -> 0x10 (adjusted) or 0x0A?
    // According to SST failures, Musashi often misses a 0x60 correction.
    printf("Result of 0x0A + 0x00 = 0x%02X\n", d1);
}

TEST_F(BcdArithmeticTest, AbcdCarryEdgeCase) {
    // Test case where result might be off by 0x60
    // src = 0x80, dst = 0x80, X=0.
    // Unadj = 0x80 + 0x80 = 0x100 (in 8-bit: 0x00, but carry bit is set).
    // Musashi's current logic uses `res > 0x99`.
    
    m68k_set_reg(M68K_REG_D0, 0x80);
    m68k_set_reg(M68K_REG_D1, 0x80);
    m68k_set_reg(M68K_REG_SR, 0x2000);
    write_mem_16(0x1000, 0xC300); 
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);

    uint32_t d1 = m68k_get_reg(NULL, M68K_REG_D1) & 0xFF;
    uint32_t sr = m68k_get_reg(NULL, M68K_REG_SR);

    // 80 + 80 in BCD (if treated as 80+80) is illegal.
    // But if we follow silicon: 
    // Unadj = 0x00, C=1.
    // High correction?
    printf("Result of 0x80 + 0x80 = 0x%02X, SR=0x%04X\n", d1, sr);
}

TEST_F(BcdArithmeticTest, SbcdZeroBorrow) {
    // 00 - 0 with X=1 should be 99 with Borrow.
    m68k_set_reg(M68K_REG_D0, 0x00);
    m68k_set_reg(M68K_REG_D1, 0x00);
    m68k_set_reg(M68K_REG_SR, 0x2010); // X=1
    write_mem_16(0x1000, 0x8300); // SBCD D0, D1
    m68k_set_reg(M68K_REG_PC, 0x1000);
    m68k_execute(6);

    EXPECT_EQ(m68k_get_reg(NULL, M68K_REG_D1) & 0xFF, 0x99);
    EXPECT_TRUE(m68k_get_reg(NULL, M68K_REG_SR) & 0x01) << "Carry should be set for 00-0-1";
}



