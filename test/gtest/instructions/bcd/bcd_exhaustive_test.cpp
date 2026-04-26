#include "../../framework/m68k_fixture.h"

class BcdExhaustiveTest : public M68kTest {
public:
    // Golden reference for ABCD (Silicon-accurate logic from WinUAE/MAME)
    struct BcdResult {
        uint8_t res;
        uint16_t sr;
    };

    BcdResult abcd_golden(uint8_t src, uint8_t dst, uint8_t x, uint16_t sr_in) {
        unsigned int newv_lo = (src & 0xF) + (dst & 0xF) + x;
        unsigned int newv_hi = (src & 0xF0) + (dst & 0xF0);
        unsigned int res = newv_hi + newv_lo;
        unsigned int unadjusted = res;

        if (newv_lo > 9)
            res += 6;

        bool c = ((res & 0x3f0) > 0x90);
        if (c)
            res += 0x60;

        bool v = ((~unadjusted) & res & 0x80) != 0;
        res &= 0xFF;

        uint16_t sr = (sr_in & 0x2700);
        if (res & 0x80) sr |= 0x08;
        if (res != 0) sr &= ~0x04; else sr |= (sr_in & 0x04);
        if (v) sr |= 0x02;
        if (c) sr |= 0x11;

        return {(uint8_t)res, sr};
    }

    BcdResult sbcd_golden(uint8_t src, uint8_t dst, uint8_t x, uint16_t sr_in) {
        unsigned int newv_lo = (dst & 0xF) - (src & 0xF) - x;
        unsigned int newv_hi = (dst & 0xF0) - (src & 0xF0);
        unsigned int res = newv_hi + newv_lo;
        unsigned int tmp_newv = res;
        unsigned int bcd = 0;

        if (newv_lo & 0xf0) {
            res -= 6;
            bcd = 6;
        }
        if (((dst & 0xff) - (src & 0xff) - x) & 0x100)
            res -= 0x60;

        bool c = ((((dst & 0xff) - (src & 0xff) - bcd - x) & 0x300) > 0xff);
        res &= 0xFF;

        bool v = ((tmp_newv & 0x80) && !(res & 0x80)) != 0;

        uint16_t sr = (sr_in & 0x2700);
        if (res & 0x80) sr |= 0x08;
        if (res != 0) sr &= ~0x04; else sr |= (sr_in & 0x04);
        if (v) sr |= 0x02;
        if (c) sr |= 0x11;

        return {(uint8_t)res, sr};
    }

    BcdResult nbcd_golden(uint8_t src, uint8_t x, uint16_t sr_in) {
        unsigned int newv_lo = (0 - (src & 0xF) - x) & 0xFFFF;
        unsigned int newv_hi = (0 - (src & 0xF0)) & 0xFFFF;
        unsigned int tmp_newv = newv_hi + newv_lo;
        unsigned int res;

        if (newv_lo > 9)
            newv_lo -= 6;

        res = newv_hi + newv_lo;

        bool c = ((res & 0x1f0) > 0x90);
        if (c)
            res -= 0x60;

        res &= 0xFF;

        bool v = ((tmp_newv & 0x80) && !(res & 0x80)) != 0;

        uint16_t sr = (sr_in & 0x2700);
        if (res & 0x80) sr |= 0x08;
        if (res != 0) sr &= ~0x04; else sr |= (sr_in & 0x04);
        if (v) sr |= 0x02;
        if (c) sr |= 0x11;

        return {(uint8_t)res, sr};
    }
};

TEST_F(BcdExhaustiveTest, AbcdExhaustive) {
    write_mem_16(0x1000, 0xC300); // ABCD D0, D1
    
    for (int x = 0; x <= 1; ++x) {
        for (int z = 0; z <= 1; ++z) {
            for (int s = 0; s < 256; ++s) {
                for (int d = 0; d < 256; ++d) {
                    uint16_t sr_in = 0x2000 | (x ? 0x10 : 0) | (z ? 0x04 : 0);
                    m68k_set_reg(M68K_REG_D0, s);
                    m68k_set_reg(M68K_REG_D1, d);
                    m68k_set_reg(M68K_REG_SR, sr_in);
                    m68k_set_reg(M68K_REG_PC, 0x1000);
                    
                    m68k_execute(6);
                    
                    uint32_t res_got = m68k_get_reg(NULL, M68K_REG_D1) & 0xFF;
                    uint16_t sr_got = m68k_get_reg(NULL, M68K_REG_SR) & 0xFF;
                    
                    BcdResult expected = abcd_golden(s, d, x, sr_in);
                    
                    if (res_got != expected.res || (sr_got & 0x1F) != (expected.sr & 0x1F)) {
                        FAIL() << "ABCD Fail: " << (int)s << " + " << (int)d << " (X=" << x << ", Z=" << z << ")\n"
                               << "Expected: Res=" << std::hex << (int)expected.res << " SR=" << (int)expected.sr << "\n"
                               << "Got:      Res=" << (int)res_got << " SR=" << (int)sr_got;
                    }
                }
            }
        }
    }
}

TEST_F(BcdExhaustiveTest, SbcdExhaustive) {
    write_mem_16(0x1000, 0x8300); // SBCD D0, D1
    
    for (int x = 0; x <= 1; ++x) {
        for (int z = 0; z <= 1; ++z) {
            for (int s = 0; s < 256; ++s) {
                for (int d = 0; d < 256; ++d) {
                    uint16_t sr_in = 0x2000 | (x ? 0x10 : 0) | (z ? 0x04 : 0);
                    m68k_set_reg(M68K_REG_D0, s);
                    m68k_set_reg(M68K_REG_D1, d);
                    m68k_set_reg(M68K_REG_SR, sr_in);
                    m68k_set_reg(M68K_REG_PC, 0x1000);
                    
                    m68k_execute(6);
                    
                    uint32_t res_got = m68k_get_reg(NULL, M68K_REG_D1) & 0xFF;
                    uint16_t sr_got = m68k_get_reg(NULL, M68K_REG_SR) & 0xFF;
                    
                    BcdResult expected = sbcd_golden(s, d, x, sr_in);
                    
                    if (res_got != expected.res || (sr_got & 0x1F) != (expected.sr & 0x1F)) {
                        FAIL() << "SBCD Fail: " << (int)d << " - " << (int)s << " (X=" << x << ", Z=" << z << ")\n"
                               << "Expected: Res=" << std::hex << (int)expected.res << " SR=" << (int)expected.sr << "\n"
                               << "Got:      Res=" << (int)res_got << " SR=" << (int)sr_got;
                    }
                }
            }
        }
    }
}

TEST_F(BcdExhaustiveTest, NbcdExhaustive) {
    write_mem_16(0x1000, 0x4800); // NBCD D0
    
    for (int x = 0; x <= 1; ++x) {
        for (int z = 0; z <= 1; ++z) {
            for (int s = 0; s < 256; ++s) {
                uint16_t sr_in = 0x2000 | (x ? 0x10 : 0) | (z ? 0x04 : 0);
                m68k_set_reg(M68K_REG_D0, s);
                m68k_set_reg(M68K_REG_SR, sr_in);
                m68k_set_reg(M68K_REG_PC, 0x1000);
                
                m68k_execute(6);
                
                uint32_t res_got = m68k_get_reg(NULL, M68K_REG_D0) & 0xFF;
                uint16_t sr_got = m68k_get_reg(NULL, M68K_REG_SR) & 0xFF;
                
                BcdResult expected = nbcd_golden(s, x, sr_in);
                
                if (res_got != expected.res || (sr_got & 0x1F) != (expected.sr & 0x1F)) {
                    FAIL() << "NBCD Fail: 0 - " << (int)s << " (X=" << x << ", Z=" << z << ")\n"
                           << "Expected: Res=" << std::hex << (int)expected.res << " SR=" << (int)expected.sr << "\n"
                           << "Got:      Res=" << (int)res_got << " SR=" << (int)sr_got;
                }
            }
        }
    }
}
