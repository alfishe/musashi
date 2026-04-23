
.TESTS_68040 = bfchg.s bfclr.s bfext.s bfffo.s bfins.s bfset.s bftst.s cas.s chk2.s cmp2.s \
	divs_long.s divu_long.s interrupt.s jmp.s mul_long.s rtd.s shifts3.s trapcc.s

.TESTS_68040_MMU = movec_tc.s movec_urp_srp.s mmu_simple.s tt_data.s itt_match.s walk_4k.s walk_8k.s \
	ptest_wp.s ptest_super.s ptest_modified.s ptest_global.s pflusha.s pflush_page.s \
	pflushn.s invalid_page.s cinv_cpush.s indirect_desc.s um_bits.s


.TESTS_68040_O = $(.TESTS_68040:%.s=%.o)
$(.TESTS_68040_O): %.o: %.s
	$(M68K_AS) $(M68K_ASFLAGS) -o $@ $<

.TESTS_68040_BIN = $(.TESTS_68040_O:%.o=%.bin)
$(.TESTS_68040_BIN): %.bin: %.o
	$(M68K_LD) $(M68K_LDFLAGS) -o $@ $<

.TESTS_68040_MMU_O = $(.TESTS_68040_MMU:%.s=%.o)
$(.TESTS_68040_MMU_O): %.o: %.s
	$(M68K_AS) $(M68K_ASFLAGS) -o $@ $<

.TESTS_68040_MMU_BIN = $(.TESTS_68040_MMU_O:%.o=%.bin)
$(.TESTS_68040_MMU_BIN): %.bin: %.o
	$(M68K_LD) $(M68K_LDFLAGS) -o $@ $<

all: $(.TESTS_68040_BIN) $(.TESTS_68040_MMU_BIN)

.PHONY: clean
clean:
	rm -f $(.TESTS_68040_O) $(.TESTS_68040_MMU_O)
