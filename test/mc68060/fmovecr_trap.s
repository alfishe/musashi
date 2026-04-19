
*
* fmovecr_trap.s — FULL coverage: ALL 64 FMOVECR offsets must trap on 68060
* The 060 removed the constant ROM entirely. Every offset ($00-$3F) must trap.
*
* FMOVECR encoding: F200 5Cxx (rm=1, src=7, opmode=0x1C|offset_in_low_6)
* word2 = 0x5C00 | offset
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

.macro TEST_CR offset
    clr.l   %d7
    .short  0xF200
    .short  \offset
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL
.endm

    * Offsets $00-$0F
    TEST_CR 0x5C00          | $00 Pi
    TEST_CR 0x5C01          | $01 (undefined)
    TEST_CR 0x5C02          | $02 (undefined)
    TEST_CR 0x5C03          | $03 (undefined)
    TEST_CR 0x5C04          | $04 (undefined)
    TEST_CR 0x5C05          | $05 (undefined)
    TEST_CR 0x5C06          | $06 (undefined)
    TEST_CR 0x5C07          | $07 (undefined)
    TEST_CR 0x5C08          | $08 (undefined)
    TEST_CR 0x5C09          | $09 (undefined)
    TEST_CR 0x5C0A          | $0A (undefined)
    TEST_CR 0x5C0B          | $0B log10(e)
    TEST_CR 0x5C0C          | $0C e
    TEST_CR 0x5C0D          | $0D log2(e)
    TEST_CR 0x5C0E          | $0E log10(2)
    TEST_CR 0x5C0F          | $0F 0.0

    * Offsets $10-$1F
    TEST_CR 0x5C10          | $10 (undefined)
    TEST_CR 0x5C11          | $11 (undefined)
    TEST_CR 0x5C12          | $12 (undefined)
    TEST_CR 0x5C13          | $13 (undefined)
    TEST_CR 0x5C14          | $14 (undefined)
    TEST_CR 0x5C15          | $15 (undefined)
    TEST_CR 0x5C16          | $16 (undefined)
    TEST_CR 0x5C17          | $17 (undefined)
    TEST_CR 0x5C18          | $18 (undefined)
    TEST_CR 0x5C19          | $19 (undefined)
    TEST_CR 0x5C1A          | $1A (undefined)
    TEST_CR 0x5C1B          | $1B (undefined)
    TEST_CR 0x5C1C          | $1C (undefined)
    TEST_CR 0x5C1D          | $1D (undefined)
    TEST_CR 0x5C1E          | $1E (undefined)
    TEST_CR 0x5C1F          | $1F (undefined)

    * Offsets $20-$2F
    TEST_CR 0x5C20          | $20 (undefined)
    TEST_CR 0x5C21          | $21 (undefined)
    TEST_CR 0x5C22          | $22 (undefined)
    TEST_CR 0x5C23          | $23 (undefined)
    TEST_CR 0x5C24          | $24 (undefined)
    TEST_CR 0x5C25          | $25 (undefined)
    TEST_CR 0x5C26          | $26 (undefined)
    TEST_CR 0x5C27          | $27 (undefined)
    TEST_CR 0x5C28          | $28 (undefined)
    TEST_CR 0x5C29          | $29 (undefined)
    TEST_CR 0x5C2A          | $2A (undefined)
    TEST_CR 0x5C2B          | $2B (undefined)
    TEST_CR 0x5C2C          | $2C (undefined)
    TEST_CR 0x5C2D          | $2D (undefined)
    TEST_CR 0x5C2E          | $2E (undefined)
    TEST_CR 0x5C2F          | $2F (undefined)

    * Offsets $30-$3F
    TEST_CR 0x5C30          | $30 ln(2)
    TEST_CR 0x5C31          | $31 ln(10)
    TEST_CR 0x5C32          | $32 1.0
    TEST_CR 0x5C33          | $33 10
    TEST_CR 0x5C34          | $34 10^2
    TEST_CR 0x5C35          | $35 10^4
    TEST_CR 0x5C36          | $36 10^8
    TEST_CR 0x5C37          | $37 10^16
    TEST_CR 0x5C38          | $38 10^32
    TEST_CR 0x5C39          | $39 10^64
    TEST_CR 0x5C3A          | $3A 10^128
    TEST_CR 0x5C3B          | $3B 10^256
    TEST_CR 0x5C3C          | $3C 10^512
    TEST_CR 0x5C3D          | $3D 10^1024
    TEST_CR 0x5C3E          | $3E 10^2048
    TEST_CR 0x5C3F          | $3F 10^4096

    rts
