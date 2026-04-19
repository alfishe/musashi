
*
* mmu_regs.s — Verify 040/060 MMU MOVEC registers round-trip
* Tests TC, ITT0, ITT1, DTT0, DTT1, URP, SRP
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * --- TC ($003) round-trip ---
    mov.l   #0x80C0, %d0
    .short  0x4E7B              | MOVEC Rn,cr
    .short  0x0003              | D0 -> TC
    clr.l   %d0
    .short  0x4E7A              | MOVEC cr,Rn
    .short  0x0003              | TC -> D0
    cmpi.l  #0x80C0, %d0
    bne     TEST_FAIL

    * --- ITT0 ($004) round-trip ---
    mov.l   #0xFF00E040, %d0
    .short  0x4E7B
    .short  0x0004              | D0 -> ITT0
    clr.l   %d0
    .short  0x4E7A
    .short  0x0004              | ITT0 -> D0
    cmpi.l  #0xFF00E040, %d0
    bne     TEST_FAIL

    * --- ITT1 ($005) round-trip ---
    mov.l   #0x12345678, %d0
    .short  0x4E7B
    .short  0x0005
    clr.l   %d0
    .short  0x4E7A
    .short  0x0005
    cmpi.l  #0x12345678, %d0
    bne     TEST_FAIL

    * --- DTT0 ($006) round-trip ---
    mov.l   #0xABCD0000, %d0
    .short  0x4E7B
    .short  0x0006
    clr.l   %d0
    .short  0x4E7A
    .short  0x0006
    cmpi.l  #0xABCD0000, %d0
    bne     TEST_FAIL

    * --- DTT1 ($007) round-trip ---
    mov.l   #0xDEADBEEF, %d0
    .short  0x4E7B
    .short  0x0007
    clr.l   %d0
    .short  0x4E7A
    .short  0x0007
    cmpi.l  #0xDEADBEEF, %d0
    bne     TEST_FAIL

    * --- URP ($806) round-trip ---
    mov.l   #0x00100000, %d0
    .short  0x4E7B
    .short  0x0806
    clr.l   %d0
    .short  0x4E7A
    .short  0x0806
    cmpi.l  #0x00100000, %d0
    bne     TEST_FAIL

    * --- SRP ($807) round-trip ---
    mov.l   #0x00200000, %d0
    .short  0x4E7B
    .short  0x0807
    clr.l   %d0
    .short  0x4E7A
    .short  0x0807
    cmpi.l  #0x00200000, %d0
    bne     TEST_FAIL

    rts
