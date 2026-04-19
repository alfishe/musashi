
.TESTS_68060 = movep_trap.s cas2_trap.s chk2_cmp2_trap.s mull_64_trap.s divl_64_trap.s \
	movec_pcr.s movec_cacr_mask.s movec_buscr.s pcr_write_mask.s mmusr_trap.s \
	fsin_trap.s fadd_exec.s fmovecr_trap.s fpiar_trap.s packed_trap.s \
	fpu_hw_ops.s fpu_sw_traps.s fpu_rounding_ops.s \
	ptest_trap.s plpa_exec.s lpstop_exec.s \
	mmu_regs.s \
	lc060_fpu_trap.s ec060_pcr.s lc060_pcr.s


.TESTS_68060_O = $(.TESTS_68060:%.s=%.o)
$(.TESTS_68060_O): %.o: %.s
	$(M68K_AS) -march=68060 -g --gdwarf-sections -I$(CURDIR)/.. -I$(CURDIR) -o $@ $<

.TESTS_68060_BIN = $(.TESTS_68060_O:%.o=%.bin)
$(.TESTS_68060_BIN): %.bin: %.o
	$(M68K_LD) $(M68K_LDFLAGS) -o $@ $<

all: $(.TESTS_68060_BIN)

.PHONY: clean
clean:
	rm -f $(.TESTS_68060_O) $(.TESTS_68060_BIN)
