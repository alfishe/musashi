# Integer ISA Test Plan

## File: `test/gtest/cpu_68000/test_data_move.cpp`

Fixture: `DataMoveTest` (cpu=68000, also parameterized for 040/060)

| # | Test Name | Opcode | Verification | Flags |
|---|-----------|--------|-------------|-------|
| 1 | MOVE_Byte_DnToDn | MOVE.B D0,D1 | D1.b = D0.b, upper D1 unchanged | N,Z |
| 2 | MOVE_Word_DnToDn | MOVE.W D0,D1 | D1.w = D0.w | N,Z |
| 3 | MOVE_Long_DnToDn | MOVE.L D0,D1 | D1 = D0 | N,Z |
| 4 | MOVE_Byte_ImmToDn | MOVE.B #$42,D0 | D0.b = 0x42 | Z=0 |
| 5 | MOVE_Word_ImmToMem | MOVE.W #$1234,(A0) | [A0] = 0x1234 | N,Z |
| 6 | MOVE_Long_MemToDn | MOVE.L (A0),D0 | D0 = [A0] | N,Z |
| 7 | MOVE_PostInc | MOVE.W (A0)+,D0 | D0.w = [A0]; A0 += 2 | N,Z |
| 8 | MOVE_PreDec | MOVE.W D0,-(A1) | A1 -= 2; [A1] = D0.w | N,Z |
| 9 | MOVE_Displacement | MOVE.L 4(A0),D0 | D0 = [A0+4] | N,Z |
| 10 | MOVE_AbsShort | MOVE.W $1000.W,D0 | D0.w = [$1000] | N,Z |
| 11 | MOVE_AbsLong | MOVE.L $10000.L,D0 | D0 = [$10000] | N,Z |
| 12 | MOVE_ZeroSetsZ | MOVE.L #0,D0 | Z=1, N=0 | Z |
| 13 | MOVE_NegSetsN | MOVE.L #$80000000,D0 | N=1, Z=0 | N |
| 14 | MOVE_ClearsVC | MOVE.L #1,D0 | V=0, C=0 | V,C |
| 15 | MOVEA_Word_SignExtend | MOVEA.W #$FFFF,A0 | A0 = $FFFFFFFF | none |
| 16 | MOVEA_Long | MOVEA.L #$12345678,A0 | A0 = $12345678 | none |
| 17 | MOVEA_NoFlags | MOVEA.W #0,A0 | flags unchanged | none |
| 18 | MOVEQ_Positive | MOVEQ #42,D3 | D3 = 42 | Z=0,N=0 |
| 19 | MOVEQ_Negative | MOVEQ #-1,D0 | D0 = $FFFFFFFF | N=1 |
| 20 | MOVEQ_Zero | MOVEQ #0,D0 | D0 = 0 | Z=1 |
| 21 | LEA_Displacement | LEA 8(A0),A1 | A1 = A0+8 | none |
| 22 | LEA_AbsLong | LEA $10000.L,A0 | A0 = $10000 | none |
| 23 | PEA_AbsLong | PEA $10000.L | [SP] = $10000; SP -= 4 | none |
| 24 | EXG_DnDn | EXG D0,D1 | D0↔D1 | none |
| 25 | EXG_AnAn | EXG A0,A1 | A0↔A1 | none |
| 26 | EXG_DnAn | EXG D0,A0 | D0↔A0 | none |
| 27 | SWAP_Normal | SWAP D0 | D0 halves swapped | N,Z |
| 28 | SWAP_Zero | SWAP D0 (D0=0) | Z=1 | Z |
| 29 | MOVE_USP_ToAn | MOVE USP,A0 | A0 = USP (supervisor) | none |
| 30 | MOVE_USP_FromAn | MOVE A0,USP | USP = A0 (supervisor) | none |

Est. coverage: **30 tests**, all MOVE/MOVEA/MOVEQ/LEA/PEA/EXG/SWAP/MOVE USP variants.

---

## File: `test/gtest/cpu_68000/test_arithmetic.cpp`

Fixture: `ArithmeticTest`

| # | Test Name | Opcode | Verification | Flags |
|---|-----------|--------|-------------|-------|
| 1 | ADD_Byte_DnToDn | ADD.B D0,D1 | D1.b = D0.b + D1.b | X,N,Z,V,C |
| 2 | ADD_Word_DnToDn | ADD.W D0,D1 | D1.w = sum | X,N,Z,V,C |
| 3 | ADD_Long_DnToDn | ADD.L D0,D1 | D1 = sum | X,N,Z,V,C |
| 4 | ADD_Long_Overflow | ADD.L #$7FFFFFFF,D0(=1) | V=1, result=$80000000 | V |
| 5 | ADD_Byte_Carry | ADD.B #$FF,D0(=$01) | C=1, X=1, result=0 | C,X,Z |
| 6 | ADDA_Word_SignExt | ADDA.W #$FFFF,A0 | A0 -= 1 (sign-extended) | none |
| 7 | ADDA_Long | ADDA.L #100,A0 | A0 += 100 | none |
| 8 | ADDI_Byte | ADDI.B #$10,D0 | D0.b += $10 | X,N,Z,V,C |
| 9 | ADDI_Long | ADDI.L #$12345678,D0 | D0 += $12345678 | X,N,Z,V,C |
| 10 | ADDQ_Dn | ADDQ.L #8,D0 | D0 += 8 | X,N,Z,V,C |
| 11 | ADDQ_An | ADDQ.L #4,A0 | A0 += 4, no flags | none |
| 12 | ADDX_Byte_DnDn | ADDX.B D0,D1 | D1.b = D0.b + D1.b + X | X,N,Z,V,C |
| 13 | ADDX_PreDec | ADDX.L -(A0),-(A1) | multi-precision add | X,N,Z,V,C |
| 14 | SUB_Long_DnToDn | SUB.L D0,D1 | D1 -= D0 | X,N,Z,V,C |
| 15 | SUB_Long_Borrow | SUB.L #1,D0(=0) | C=1, N=1 | C,N |
| 16 | SUB_Long_Overflow | SUB.L #1,D0($80000000) | V=1 | V |
| 17 | SUBA_Word | SUBA.W #$100,A0 | A0 -= $100 | none |
| 18 | SUBI_Long | SUBI.L #$100,D0 | D0 -= $100 | X,N,Z,V,C |
| 19 | SUBQ_Dn | SUBQ.L #1,D0 | D0 -= 1 | X,N,Z,V,C |
| 20 | SUBX_Long | SUBX.L D0,D1 | multi-precision sub | X,N,Z,V,C |
| 21 | CMP_Long_Equal | CMP.L D0,D1 (equal) | Z=1 | N,Z,V,C |
| 22 | CMP_Long_Greater | CMP.L D0,D1 (D1>D0) | N=0,Z=0 | N,Z,V,C |
| 23 | CMP_Long_Less | CMP.L D0,D1 (D1<D0) | N=1 | N,Z,V,C |
| 24 | CMPA_Long | CMPA.L D0,A0 | compare An-Dn | N,Z,V,C |
| 25 | CMPI_Long | CMPI.L #$1234,D0 | D0 - imm | N,Z,V,C |
| 26 | CMPM_Long | CMPM.L (A0)+,(A1)+ | [A1]-[A0], both inc | N,Z,V,C |
| 27 | MULS_Word | MULS.W D0,D1 | D1 = D0.w * D1.w (signed) | N,Z |
| 28 | MULU_Word | MULU.W D0,D1 | D1 = D0.w * D1.w (unsigned) | N,Z |
| 29 | DIVS_Word | DIVS.W D0,D1 | D1 = quot:rem | N,Z,V,C |
| 30 | DIVS_Word_ByZero | DIVS.W #0,D0 | divide-by-zero exception | vector 5 |
| 31 | DIVU_Word | DIVU.W D0,D1 | D1 = quot:rem | N,Z,V,C |
| 32 | DIVU_Word_Overflow | DIVU.W #1,D0(big) | V=1 | V |
| 33 | NEG_Long | NEG.L D0 | D0 = -D0 | X,N,Z,V,C |
| 34 | NEG_Zero | NEG.L D0(=0) | D0=0, Z=1, C=0 | Z,C |
| 35 | NEGX_Long | NEGX.L D0 | 0-D0-X | X,N,Z,V,C |
| 36 | CLR_Long | CLR.L D0 | D0=0, Z=1 | Z |
| 37 | CLR_Byte_Mem | CLR.B (A0) | [A0]=0 | Z |
| 38 | TST_Long_Positive | TST.L D0(>0) | Z=0, N=0 | N,Z |
| 39 | TST_Long_Zero | TST.L D0(=0) | Z=1 | Z |
| 40 | TST_Long_Negative | TST.L D0(<0) | N=1 | N |
| 41 | EXT_WordFromByte | EXT.W D0 | sign-extend B→W | N,Z |
| 42 | EXT_LongFromWord | EXT.L D0 | sign-extend W→L | N,Z |
| 43 | TAS_Dn | TAS D0 | set bit 7, flags | N,Z |
| 44 | TAS_Mem | TAS (A0) | read-modify-write, LK | N,Z |
| 45 | CHK_Word_InRange | CHK.W D1,D0 | no trap if 0<=D0<=D1 | — |
| 46 | CHK_Word_Negative | CHK.W D1,D0(neg) | CHK exception | N=1 |
| 47 | CHK_Word_Overflow | CHK.W D1,D0(>D1) | CHK exception | N=0 |

Est. coverage: **47 tests**

---

## File: `test/gtest/cpu_68000/test_logic.cpp`

Fixture: `LogicTest`

| # | Test Name | Opcode | Verification | Flags |
|---|-----------|--------|-------------|-------|
| 1 | AND_Long_DnToDn | AND.L D0,D1 | D1 = D0 & D1 | N,Z,V=0,C=0 |
| 2 | AND_Byte_ImmToDn | ANDI.B #$0F,D0 | D0.b &= $0F | N,Z |
| 3 | ANDI_ToCCR | ANDI #$00,CCR | clear all flags | all |
| 4 | ANDI_ToSR | ANDI #$F8FF,SR | clear interrupt mask | supervisor |
| 5 | OR_Long_DnToDn | OR.L D0,D1 | D1 = D0 \| D1 | N,Z |
| 6 | OR_Word_ImmToMem | ORI.W #$FF00,(A0) | [A0] \|= $FF00 | N,Z |
| 7 | ORI_ToCCR | ORI #$1F,CCR | set all flags | all |
| 8 | ORI_ToSR | ORI #$0700,SR | set interrupt mask | supervisor |
| 9 | EOR_Long_DnToMem | EOR.L D0,(A0) | [A0] ^= D0 | N,Z |
| 10 | EORI_ToCCR | EORI #$1F,CCR | toggle all flags | all |
| 11 | EORI_ToSR | EORI #$2000,SR | toggle S bit | supervisor |
| 12 | NOT_Long | NOT.L D0 | D0 = ~D0 | N,Z |
| 13 | NOT_Byte | NOT.B D0 | D0.b = ~D0.b | N,Z |
| 14 | AND_ClearsVC | AND.L D0,D1 | V=0, C=0 always | V,C |

Est. coverage: **14 tests**

---

## File: `test/gtest/cpu_68000/test_shift.cpp`

Fixture: `ShiftTest`

| # | Test Name | Opcode | Verification | Flags |
|---|-----------|--------|-------------|-------|
| 1 | ASL_Reg_Count1 | ASL.L #1,D0 | shift left, MSB→C/X | X,N,Z,V,C |
| 2 | ASL_Reg_Overflow | ASL.L #1,D0($40000000) | V=1 (sign change) | V |
| 3 | ASR_Reg_Count1 | ASR.L #1,D0 | arithmetic right shift | X,N,Z,V,C |
| 4 | ASR_Negative | ASR.L #1,D0($80000000) | sign preserved | N |
| 5 | LSL_Reg_Count8 | LSL.B #8,D0 | all bits out, Z=1 | Z,C |
| 6 | LSR_Reg_Count1 | LSR.L #1,D0 | logical right, 0 fill | X,C |
| 7 | ROL_Reg_Count1 | ROL.L #1,D0 | rotate left, MSB→LSB→C | C |
| 8 | ROR_Reg_Count1 | ROR.L #1,D0 | rotate right | C |
| 9 | ROXL_WithExtend | ROXL.L #1,D0 | rotate left through X | X,C |
| 10 | ROXR_WithExtend | ROXR.L #1,D0 | rotate right through X | X,C |
| 11 | ASL_Memory | ASL.W (A0) | memory shift | X,N,Z,V,C |
| 12 | LSR_Memory | LSR.W (A0) | memory shift | X,N,Z,V,C |
| 13 | ROL_DnCount | ROL.L D1,D0 | variable count | C |
| 14 | ASL_ZeroCount | ASL.L #0,D0 | D0 unchanged, C=0 | C=0 |
| 15 | LSL_Byte | LSL.B #1,D0 | only low byte shifted | X,C |

Est. coverage: **15 tests**

---

## File: `test/gtest/cpu_68000/test_bcd.cpp`

Fixture: `BCDTest`

| # | Test Name | Opcode | Verification | Flags |
|---|-----------|--------|-------------|-------|
| 1 | ABCD_DnDn_NoCarry | ABCD D0,D1 | BCD add, no carry | X,C |
| 2 | ABCD_DnDn_WithCarry | ABCD D0,D1 (9+9) | BCD result, C=1 | X,C |
| 3 | ABCD_DnDn_WithExtend | ABCD D0,D1 (X=1) | +1 from extend | X,C |
| 4 | ABCD_PreDec | ABCD -(A0),-(A1) | memory BCD add | X,C |
| 5 | SBCD_DnDn | SBCD D0,D1 | BCD subtract | X,C |
| 6 | SBCD_Borrow | SBCD D0,D1 (0-1) | borrow, C=1 | X,C |
| 7 | NBCD_Dn | NBCD D0 | 0 - D0 - X (BCD) | X,C |
| 8 | NBCD_Zero | NBCD D0(=0, X=0) | result=0, Z unchanged | X,C |

Est. coverage: **8 tests**

---

## File: `test/gtest/cpu_68000/test_branch.cpp`

Fixture: `BranchTest`

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | BRA_Short | BRA.S +4 | PC += 4 |
| 2 | BRA_Word | BRA.W +$100 | PC += $100 |
| 3 | BEQ_Taken | BEQ (Z=1) | branch taken |
| 4 | BEQ_NotTaken | BEQ (Z=0) | falls through |
| 5 | BNE_Taken | BNE (Z=0) | branch taken |
| 6 | BCS_Taken | BCS (C=1) | branch taken |
| 7 | BCC_Taken | BCC (C=0) | branch taken |
| 8 | BMI_Taken | BMI (N=1) | branch taken |
| 9 | BPL_Taken | BPL (N=0) | branch taken |
| 10 | BVS_Taken | BVS (V=1) | branch taken |
| 11 | BGT_Taken | BGT (N=V, Z=0) | branch taken |
| 12 | BLE_Taken | BLE (N≠V or Z=1) | branch taken |
| 13 | BGE_Taken | BGE (N=V) | branch taken |
| 14 | BLT_Taken | BLT (N≠V) | branch taken |
| 15 | BHI_Taken | BHI (C=0, Z=0) | branch taken |
| 16 | BLS_Taken | BLS (C=1 or Z=1) | branch taken |
| 17 | BSR_Short | BSR.S +10 | PC pushed, branch |
| 18 | BSR_Word | BSR.W +$200 | PC pushed, branch |
| 19 | JMP_Abs | JMP $10100 | PC = $10100 |
| 20 | JMP_Indirect | JMP (A0) | PC = [A0] |
| 21 | JSR_Abs | JSR $10100 | return addr pushed |
| 22 | RTS | RTS | PC = [SP]; SP += 4 |
| 23 | RTR | RTR | CCR + PC restored |
| 24 | DBcc_CountDown | DBNE D0,target | D0--, branch if D0≠-1 |
| 25 | DBcc_Expired | DBNE D0(=-1),target | falls through |
| 26 | DBcc_CondTrue | DBEQ D0,target (Z=1) | falls through (cond true) |
| 27 | Scc_True | SEQ D0 (Z=1) | D0.b = $FF |
| 28 | Scc_False | SEQ D0 (Z=0) | D0.b = $00 |
| 29 | LINK_Word | LINK A6,#-8 | frame pointer setup |
| 30 | UNLK | UNLK A6 | frame pointer teardown |

Est. coverage: **30 tests**

---

## File: `test/gtest/cpu_68000/test_bit.cpp`

Fixture: `BitTest`

| # | Test Name | Opcode | Verification | Flags |
|---|-----------|--------|-------------|-------|
| 1 | BTST_Dn_Set | BTST #0,D0(odd) | Z=0 | Z |
| 2 | BTST_Dn_Clear | BTST #0,D0(even) | Z=1 | Z |
| 3 | BTST_Mem | BTST #7,(A0) | test MSB of byte | Z |
| 4 | BSET_Dn | BSET #3,D0 | bit 3 set, old→Z | Z |
| 5 | BCLR_Dn | BCLR #3,D0 | bit 3 cleared, old→Z | Z |
| 6 | BCHG_Dn | BCHG #3,D0 | bit 3 toggled, old→Z | Z |
| 7 | BTST_DnModulo32 | BTST #33,D0 | bit 1 (33 mod 32) | Z |
| 8 | BTST_MemModulo8 | BTST #9,(A0) | bit 1 (9 mod 8) | Z |

Est. coverage: **8 tests**

---

## File: `test/gtest/cpu_68000/test_misc.cpp`

Fixture: `MiscTest`

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | NOP | NOP | PC += 2, no state change |
| 2 | ILLEGAL | ILLEGAL ($4AFC) | vector 4 exception |
| 3 | TRAP_N | TRAP #0 | vector 32 exception |
| 4 | TRAP_15 | TRAP #15 | vector 47 exception |
| 5 | TRAPV_NoOverflow | TRAPV (V=0) | no trap |
| 6 | TRAPV_Overflow | TRAPV (V=1) | vector 7 exception |
| 7 | STOP_Privileged | STOP #$2700 | halts, loads SR |
| 8 | RESET_Supervisor | RESET | callback fired |
| 9 | LINE_A | $Axxx opcode | vector 10 (Line-A) |
| 10 | LINE_F | $Fxxx opcode | vector 11 (Line-F) |

Est. coverage: **10 tests**

---

## File: `test/gtest/cpu_68000/test_movem_movep.cpp`

Fixture: `MovemMovepTest`

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | MOVEM_RegToMem_Word | MOVEM.W D0-D3,-(A7) | 4 words pushed |
| 2 | MOVEM_MemToReg_Word | MOVEM.W (A7)+,D0-D3 | 4 words popped |
| 3 | MOVEM_RegToMem_Long | MOVEM.L D0-D7/A0-A6,-(A7) | 15 longs pushed |
| 4 | MOVEM_MemToReg_Long | MOVEM.L (A7)+,D0-D7/A0-A6 | 15 longs popped |
| 5 | MOVEM_Partial | MOVEM.L D0/D3/A2,(A0) | selected registers |
| 6 | MOVEP_Word_ToMem | MOVEP.W D0,0(A0) | bytes at +0,+2 |
| 7 | MOVEP_Long_ToMem | MOVEP.L D0,0(A0) | bytes at +0,+2,+4,+6 |
| 8 | MOVEP_Word_FromMem | MOVEP.W 0(A0),D0 | read alternating bytes |
| 9 | MOVEP_Long_FromMem | MOVEP.L 0(A0),D0 | read alternating bytes |

Est. coverage: **9 tests**

---

## File: `test/gtest/cpu_68020/test_bitfield.cpp`

Fixture: `BitfieldTest` (cpu=68020+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | BFTST_Dn_AllOnes | BFTST D0{0:32} | Z=0, N=1 if MSB set |
| 2 | BFTST_Dn_AllZeros | BFTST D0{0:32} (D0=0) | Z=1 |
| 3 | BFEXTU_Dn | BFEXTU D0{4:8},D1 | extract unsigned bits |
| 4 | BFEXTS_Dn | BFEXTS D0{4:8},D1 | extract signed (sign-extend) |
| 5 | BFINS_Dn | BFINS D1,D0{4:8} | insert bit field |
| 6 | BFSET_Dn | BFSET D0{8:16} | set bits 8..23 |
| 7 | BFCLR_Dn | BFCLR D0{8:16} | clear bits 8..23 |
| 8 | BFCHG_Dn | BFCHG D0{0:32} | toggle all bits |
| 9 | BFFFO_Dn | BFFFO D0{0:32},D1 | find first one |
| 10 | BFFFO_AllZero | BFFFO D0{0:32},D1 (D0=0) | D1 = offset+width |
| 11 | BFEXTU_Memory | BFEXTU (A0){4:8},D0 | cross-byte extract |
| 12 | BFINS_Memory | BFINS D0,(A0){4:8} | cross-byte insert |
| 13 | BFTST_DynOffset | BFTST D0{D1:8} | Dn as offset |
| 14 | BFEXTU_Width32 | BFEXTU D0{0:0} | width 0 = 32 bits |
| 15 | BFSET_CrossWord | BFSET (A0){28:16} | spans word boundary |

Est. coverage: **15 tests**

---

## File: `test/gtest/cpu_68020/test_long_mul_div.cpp`

Fixture: `LongMulDivTest` (cpu=68020+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | MULS_Long_32x32to32 | MULS.L D0,D1 | D1 = signed product (low 32) |
| 2 | MULS_Long_32x32to64 | MULS.L D0,D2:D1 | D2:D1 = full 64-bit product |
| 3 | MULU_Long_32x32to32 | MULU.L D0,D1 | unsigned product |
| 4 | MULU_Long_32x32to64 | MULU.L D0,D2:D1 | full 64-bit |
| 5 | DIVS_Long_64by32 | DIVS.L D0,D2:D1 | D1=quot, D2=rem |
| 6 | DIVU_Long_64by32 | DIVU.L D0,D2:D1 | unsigned div |
| 7 | DIVS_Long_32by32 | DIVS.L D0,D1 | 32/32 form |
| 8 | DIVS_Long_ByZero | DIVS.L #0,D1 | exception vector 5 |
| 9 | DIVU_Long_Overflow | DIVU.L D0,D1 | V=1 on overflow |
| 10 | MULS_Long_NegResult | MULS.L (neg*pos) | N=1 |

Est. coverage: **10 tests**

---

## File: `test/gtest/cpu_68020/test_cas.cpp`

Fixture: `CASTest` (cpu=68020+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | CAS_Byte_Match | CAS.B Dc,Du,(A0) | match: [A0]=Du | Z=1 |
| 2 | CAS_Byte_NoMatch | CAS.B Dc,Du,(A0) | no match: Dc=[A0] | Z=0 |
| 3 | CAS_Word_Match | CAS.W | match: memory updated |
| 4 | CAS_Long_Match | CAS.L | match: memory updated |
| 5 | CAS_Long_NoMatch | CAS.L | Dc loaded with memory |
| 6 | CAS_Flags | CAS.L | N,Z,V,C from Dc-[EA] |
| 7 | CAS_LK_Bit | CAS.L to fault addr | SSW LK=1 in bus error |
| 8 | CAS2_Word | CAS2.W | dual compare-and-swap |
| 9 | CAS2_Long | CAS2.L | dual compare-and-swap |

Est. coverage: **9 tests**

---

## File: `test/gtest/cpu_68020/test_chk2_cmp2.cpp`

Fixture: `CHK2CMP2Test` (cpu=68020+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | CMP2_Byte_InRange | CMP2.B (A0),D0 | C=0, Z=0 |
| 2 | CMP2_Byte_AtBound | CMP2.B | Z=1 (at lower/upper) |
| 3 | CMP2_Byte_OutOfRange | CMP2.B | C=1 |
| 4 | CHK2_Byte_InRange | CHK2.B | no exception |
| 5 | CHK2_Byte_OutOfRange | CHK2.B | CHK exception |
| 6 | CMP2_Long | CMP2.L | 32-bit bounds |
| 7 | CHK2_An | CHK2.L (A0),A1 | address register bounds |

Est. coverage: **7 tests**

---

## File: `test/gtest/cpu_68020/test_trapcc.cpp`

Fixture: `TRAPccTest` (cpu=68020+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | TRAPcc_Taken | TRAPEQ (Z=1) | vector 7 |
| 2 | TRAPcc_NotTaken | TRAPEQ (Z=0) | no trap |
| 3 | TRAPcc_WithWord | TRAPT.W #$1234 | trap with operand |
| 4 | TRAPcc_WithLong | TRAPT.L #$12345678 | trap with operand |
| 5 | RTD | RTD #8 | RTS + deallocate 8 |

Est. coverage: **5 tests**

---

## File: `test/gtest/cpu_68020/test_pack_unpk.cpp`

Fixture: `PackUnpkTest` (cpu=68020+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | PACK_DnDn | PACK D0,D1,#0 | BCD pack |
| 2 | PACK_PreDec | PACK -(A0),-(A1),#0 | memory pack |
| 3 | UNPK_DnDn | UNPK D0,D1,#0 | BCD unpack |
| 4 | UNPK_PreDec | UNPK -(A0),-(A1),#0 | memory unpack |
| 5 | PACK_Adjustment | PACK D0,D1,#$30 | ASCII adjustment |

Est. coverage: **5 tests**

---

## File: `test/gtest/cpu_68020/test_link_long.cpp`

Fixture: `LinkLongTest` (cpu=68020+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | LINK_Long | LINK.L A6,#-$10000 | 32-bit displacement |
| 2 | EXTB_Long | EXTB.L D0 | sign-extend B→L |
| 3 | MOVES_Read | MOVES.L (A0),D0 | read with FC=SFC |
| 4 | MOVES_Write | MOVES.L D0,(A0) | write with FC=DFC |

Est. coverage: **4 tests**

---

## File: `test/gtest/cpu_68040/test_move16.cpp`

Fixture: `MOVE16Test` (cpu=68040+)

| # | Test Name | Opcode | Verification |
|---|-----------|--------|-------------|
| 1 | MOVE16_AxPlusAyPlus | MOVE16 (A0)+,(A1)+ | 16 bytes copied, both inc |
| 2 | MOVE16_AxPlusToAbs | MOVE16 (A0)+,xxx.L | 16 bytes to abs addr |
| 3 | MOVE16_AbsToAxPlus | MOVE16 xxx.L,(A0)+ | 16 bytes from abs |
| 4 | MOVE16_AxToAbs | MOVE16 (A0),xxx.L | no post-increment |
| 5 | MOVE16_AbsToAx | MOVE16 xxx.L,(A0) | no post-increment |
| 6 | MOVE16_Alignment | MOVE16 with A0=$1003 | masked to $1000 |
| 7 | MOVE16_16ByteContent | MOVE16 | verify all 16 bytes |

Est. coverage: **7 tests**

---

## Cumulative Integer Test Summary

| File | Test Count |
|------|:---------:|
| test_data_move.cpp | 30 |
| test_arithmetic.cpp | 47 |
| test_logic.cpp | 14 |
| test_shift.cpp | 15 |
| test_bcd.cpp | 8 |
| test_branch.cpp | 30 |
| test_bit.cpp | 8 |
| test_misc.cpp | 10 |
| test_movem_movep.cpp | 9 |
| test_bitfield.cpp | 15 |
| test_long_mul_div.cpp | 10 |
| test_cas.cpp | 9 |
| test_chk2_cmp2.cpp | 7 |
| test_trapcc.cpp | 5 |
| test_pack_unpk.cpp | 5 |
| test_link_long.cpp | 4 |
| test_move16.cpp | 7 |
| test_040_fline.cpp | (see FPU plan) |
| **Total** | **233** |
