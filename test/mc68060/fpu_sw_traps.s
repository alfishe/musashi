
*
* fpu_sw_traps.s — Verify unimplemented FPU ops trap to F-line on 060
*
* Tests transcendental/special ops removed from 060 hardware:
*   FSIN(0x0E), FCOS(0x1D), FTAN(0x0F), FASIN(0x0C), FACOS(0x1C),
*   FATAN(0x0A), FSINH(0x02), FCOSH(0x19), FTANH(0x09),
*   FETOX(0x10), FTWOTOX(0x11), FTENTOX(0x12),
*   FLOGN(0x14), FLOG10(0x15), FLOG2(0x16),
*   FMOD(0x21), FSGLDIV(0x24), FSGLMUL(0x27),
*   FGETEXP(0x1E), FGETMAN(0x1F),
*   FREM(0x25)
*
* word2 format: 0 src(3) dst(3) opmode(7)
* For reg-to-reg with src=fp1(001), dst=fp0(000):
*   word2 = (1 << 10) | opmode = 0x0400 | opmode
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Load harmless values into FP regs
    fmove.l  #1, %fp0
    fmove.l  #1, %fp1

* Macro: test that an FPU opmode traps (F-line)
* word2_val must be pre-computed: 0x0400 + opmode
.macro TEST_SW_TRAP word2_val
    clr.l    %d7
    .short   0xF200
    .short   \word2_val
    cmpi.l   #0xEEEE000B, %d7
    bne      TEST_FAIL
.endm

    * Transcendentals
    TEST_SW_TRAP 0x0402     | FSINH  (0x02)
    TEST_SW_TRAP 0x0409     | FTANH  (0x09)
    TEST_SW_TRAP 0x040A     | FATAN  (0x0A)
    TEST_SW_TRAP 0x040C     | FASIN  (0x0C)
    TEST_SW_TRAP 0x040E     | FSIN   (0x0E)
    TEST_SW_TRAP 0x040F     | FTAN   (0x0F)
    TEST_SW_TRAP 0x0419     | FCOSH  (0x19)
    TEST_SW_TRAP 0x041C     | FACOS  (0x1C)
    TEST_SW_TRAP 0x041D     | FCOS   (0x1D)

    * Exponential/logarithmic
    TEST_SW_TRAP 0x0410     | FETOX  (0x10)
    TEST_SW_TRAP 0x0411     | FTWOTOX (0x11)
    TEST_SW_TRAP 0x0412     | FTENTOX (0x12)
    TEST_SW_TRAP 0x0414     | FLOGN  (0x14)
    TEST_SW_TRAP 0x0415     | FLOG10 (0x15)
    TEST_SW_TRAP 0x0416     | FLOG2  (0x16)

    * Special
    TEST_SW_TRAP 0x041E     | FGETEXP (0x1E)
    TEST_SW_TRAP 0x041F     | FGETMAN (0x1F)
    TEST_SW_TRAP 0x0421     | FMOD   (0x21)
    TEST_SW_TRAP 0x0424     | FSGLDIV (0x24)
    TEST_SW_TRAP 0x0425     | FREM   (0x25)
    TEST_SW_TRAP 0x0427     | FSGLMUL (0x27)

    rts
