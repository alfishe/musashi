#include "../../framework/m68k_fixture.h"
#include <vector>
#include <string>

// Structure for BCD test cases
struct BcdParams {
    std::string name;
    uint16_t opcode;
    uint8_t src;
    uint8_t dst;
    uint8_t x;
    uint8_t z;

    // For better GTest output
    friend std::ostream& operator<<(std::ostream& os, const BcdParams& p) {
        return os << p.name << " (src=" << std::hex << (int)p.src << " dst=" << (int)p.dst << " X=" << (int)p.x << " Z=" << (int)p.z << ")";
    }
};

class BcdTest : public M68kTest, public ::testing::WithParamInterface<BcdParams> {
protected:
    // Flamewing's bit-exact logic for 68000 BCD
    struct Result {
        uint8_t res;
        uint8_t x, n, z, v, c;
    };

    Result abcd_ref(uint8_t xx, uint8_t yy, uint8_t x_in, uint8_t z_in) {
        uint8_t ss = xx + yy + x_in;
        uint8_t bc = ((xx & yy) | (~ss & xx) | (~ss & yy)) & 0x88;
        uint8_t dc = (((ss + 0x66) ^ ss) & 0x110) >> 1;
        uint8_t corf = (bc | dc) - ((bc | dc) >> 2);
        uint8_t rr = ss + corf;
        
        Result r;
        r.res = rr;
        r.x = r.c = (bc | (ss & ~rr)) >> 7;
        r.v = (~ss & rr) >> 7;
        r.z = z_in & (rr == 0);
        r.n = rr >> 7;
        return r;
    }

    Result sbcd_ref(uint8_t xx, uint8_t yy, uint8_t x_in, uint8_t z_in) {
        uint8_t dd = yy - xx - x_in;
        uint8_t bc = ((~yy & xx) | (dd & ~yy) | (dd & xx)) & 0x88;
        uint8_t corf = bc - (bc >> 2);
        uint8_t rr = dd - corf;

        Result r;
        r.res = rr;
        r.x = r.c = (bc | (~dd & rr)) >> 7;
        r.v = (dd & ~rr) >> 7;
        r.z = z_in & (rr == 0);
        r.n = rr >> 7;
        return r;
    }

    Result nbcd_ref(uint8_t xx, uint8_t x_in, uint8_t z_in) {
        // NBCD is basically 0 - src - X
        uint8_t dd = 0 - xx - x_in;
        uint8_t bc = (xx | dd) & 0x88;
        uint8_t corf = bc - (bc >> 2);
        uint8_t rr = dd - corf;

        Result r;
        r.res = rr;
        r.x = r.c = (bc | (~dd & rr)) >> 7;
        r.v = (dd & ~rr) >> 7;
        r.z = z_in & (rr == 0);
        r.n = rr >> 7;
        return r;
    }
};

TEST_P(BcdTest, Execute) {
    const auto& p = GetParam();
    
    m68k_set_reg(M68K_REG_D0, p.src);
    m68k_set_reg(M68K_REG_D1, p.dst);
    uint16_t sr_in = 0x2000 | (p.x ? 0x10 : 0) | (p.z ? 0x04 : 0);
    m68k_set_reg(M68K_REG_SR, sr_in);
    
    write_mem_16(0x1000, p.opcode);
    m68k_set_reg(M68K_REG_PC, 0x1000);
    
    // BCD register-to-register takes 6 cycles; 10 would execute the next insn too
    m68k_execute(6);
    
    uint32_t res_got;
    if ((p.opcode & 0xFF00) == 0x4800) { // NBCD
        res_got = m68k_get_reg(NULL, M68K_REG_D0) & 0xFF;
    } else {
        res_got = m68k_get_reg(NULL, M68K_REG_D1) & 0xFF;
    }
    uint16_t sr_got = m68k_get_reg(NULL, M68K_REG_SR);

    Result expected;
    if ((p.opcode & 0xF100) == 0xC100) expected = abcd_ref(p.src, p.dst, p.x, p.z);
    else if ((p.opcode & 0xF100) == 0x8100) expected = sbcd_ref(p.src, p.dst, p.x, p.z);
    else expected = nbcd_ref(p.src, p.x, p.z);

    EXPECT_EQ(res_got, expected.res) << "Result mismatch";
    EXPECT_EQ((sr_got >> 4) & 1, expected.x) << "X flag mismatch";
    EXPECT_EQ((sr_got >> 3) & 1, expected.n) << "N flag mismatch";
    EXPECT_EQ((sr_got >> 2) & 1, expected.z) << "Z flag mismatch";
    EXPECT_EQ((sr_got >> 1) & 1, expected.v) << "V flag mismatch";
    EXPECT_EQ((sr_got >> 0) & 1, expected.c) << "C flag mismatch";
}

// Curated interesting cases
INSTANTIATE_TEST_SUITE_P(BcdInteresting, BcdTest, ::testing::Values(
    // ABCD
    BcdParams{"ABCD_Basic", 0xC300, 0x12, 0x34, 0, 0},
    BcdParams{"ABCD_CarryLow", 0xC300, 0x09, 0x02, 0, 0},
    BcdParams{"ABCD_CarryHigh", 0xC300, 0x90, 0x10, 0, 0},
    BcdParams{"ABCD_Wrap", 0xC300, 0x99, 0x01, 0, 1},
    BcdParams{"ABCD_Illegal", 0xC300, 0x0A, 0x00, 0, 0},
    BcdParams{"ABCD_V_Transition", 0xC300, 0x39, 0x42, 0, 0},
    
    // SBCD
    BcdParams{"SBCD_Basic", 0x8300, 0x12, 0x34, 0, 0},
    BcdParams{"SBCD_BorrowLow", 0x8300, 0x05, 0x12, 0, 0},
    BcdParams{"SBCD_BorrowHigh", 0x8300, 0x20, 0x10, 0, 0},
    BcdParams{"SBCD_Wrap", 0x8300, 0x01, 0x00, 0, 1},
    BcdParams{"SBCD_V_Transition", 0x8300, 0x41, 0x20, 0, 0},

    // NBCD
    BcdParams{"NBCD_Basic", 0x4800, 0x12, 0, 0, 0},
    BcdParams{"NBCD_Zero_X0", 0x4800, 0x00, 0, 0, 1},
    BcdParams{"NBCD_Zero_X1", 0x4800, 0x00, 0, 1, 1},
    BcdParams{"NBCD_V_Transition", 0x4800, 0x41, 0, 0, 0}
));

// ---------------------------------------------------------------------------
// Exhaustive flag verification (all 256 src x 256 dst x 2 X x 2 Z)
// ---------------------------------------------------------------------------

TEST_F(BcdTest, AbcdExhaustiveFlags) {
    for (int src = 0; src < 256; ++src) {
        for (int dst = 0; dst < 256; ++dst) {
            for (int x = 0; x < 2; ++x) {
                for (int z = 0; z < 2; ++z) {
                    m68k_set_reg(M68K_REG_D0, src);
                    m68k_set_reg(M68K_REG_D1, dst);
                    uint16_t sr_in = 0x2000 | (x ? 0x10 : 0) | (z ? 0x04 : 0);
                    m68k_set_reg(M68K_REG_SR, sr_in);

                    write_mem_16(0x1000, 0xC300);
                    m68k_set_reg(M68K_REG_PC, 0x1000);
                    m68k_execute(6);

                    uint8_t res_got = m68k_get_reg(NULL, M68K_REG_D1) & 0xFF;
                    uint16_t sr_got = m68k_get_reg(NULL, M68K_REG_SR);

                    Result expected = abcd_ref(src, dst, x, z);

                    EXPECT_EQ(res_got, expected.res)
                        << "ABCD src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 4) & 1, expected.x)
                        << "ABCD X flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 3) & 1, expected.n)
                        << "ABCD N flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 2) & 1, expected.z)
                        << "ABCD Z flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 1) & 1, expected.v)
                        << "ABCD V flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 0) & 1, expected.c)
                        << "ABCD C flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                }
            }
        }
    }
}

TEST_F(BcdTest, SbcdExhaustiveFlags) {
    for (int src = 0; src < 256; ++src) {
        for (int dst = 0; dst < 256; ++dst) {
            for (int x = 0; x < 2; ++x) {
                for (int z = 0; z < 2; ++z) {
                    m68k_set_reg(M68K_REG_D0, src);
                    m68k_set_reg(M68K_REG_D1, dst);
                    uint16_t sr_in = 0x2000 | (x ? 0x10 : 0) | (z ? 0x04 : 0);
                    m68k_set_reg(M68K_REG_SR, sr_in);

                    write_mem_16(0x1000, 0x8300);
                    m68k_set_reg(M68K_REG_PC, 0x1000);
                    m68k_execute(6);

                    uint8_t res_got = m68k_get_reg(NULL, M68K_REG_D1) & 0xFF;
                    uint16_t sr_got = m68k_get_reg(NULL, M68K_REG_SR);

                    Result expected = sbcd_ref(src, dst, x, z);

                    EXPECT_EQ(res_got, expected.res)
                        << "SBCD src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 4) & 1, expected.x)
                        << "SBCD X flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 3) & 1, expected.n)
                        << "SBCD N flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 2) & 1, expected.z)
                        << "SBCD Z flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 1) & 1, expected.v)
                        << "SBCD V flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                    EXPECT_EQ((sr_got >> 0) & 1, expected.c)
                        << "SBCD C flag src=" << src << " dst=" << dst << " X=" << x << " Z=" << z;
                }
            }
        }
    }
}

TEST_F(BcdTest, NbcdExhaustiveFlags) {
    for (int src = 0; src < 256; ++src) {
        for (int x = 0; x < 2; ++x) {
            for (int z = 0; z < 2; ++z) {
                m68k_set_reg(M68K_REG_D0, src);
                uint16_t sr_in = 0x2000 | (x ? 0x10 : 0) | (z ? 0x04 : 0);
                m68k_set_reg(M68K_REG_SR, sr_in);

                write_mem_16(0x1000, 0x4800);
                m68k_set_reg(M68K_REG_PC, 0x1000);
                m68k_execute(6);

                uint8_t res_got = m68k_get_reg(NULL, M68K_REG_D0) & 0xFF;
                uint16_t sr_got = m68k_get_reg(NULL, M68K_REG_SR);

                Result expected = nbcd_ref(src, x, z);

                EXPECT_EQ(res_got, expected.res)
                    << "NBCD src=" << src << " X=" << x << " Z=" << z;
                EXPECT_EQ((sr_got >> 4) & 1, expected.x)
                    << "NBCD X flag src=" << src << " X=" << x << " Z=" << z;
                EXPECT_EQ((sr_got >> 3) & 1, expected.n)
                    << "NBCD N flag src=" << src << " X=" << x << " Z=" << z;
                EXPECT_EQ((sr_got >> 2) & 1, expected.z)
                    << "NBCD Z flag src=" << src << " X=" << x << " Z=" << z;
                EXPECT_EQ((sr_got >> 1) & 1, expected.v)
                    << "NBCD V flag src=" << src << " X=" << x << " Z=" << z;
                EXPECT_EQ((sr_got >> 0) & 1, expected.c)
                    << "NBCD C flag src=" << src << " X=" << x << " Z=" << z;
            }
        }
    }
}
