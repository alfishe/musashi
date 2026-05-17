import re

with open("m68k_in.c", "r") as f:
    content = f.read()

def patch_op(op):
    pattern = rf"(M68KMAKE_OP\({op}, 32, mm, \.\)\n\{{\n)([\s\S]+?)(\n\}})"
    def repl(m):
        body = m.group(2)
        # Instead of uint src = OPER_AY_PD_32();
        # uint ea = EA_AX_PD_32();
        # dst = m68ki_read_32(ea);
        # We replace with manual words
        new_body = f"""	uint src_l;
	uint src_h;
	uint dst_l;
	uint dst_h;
	uint src;
	uint dst;
	uint res;

	AY -= 2;
	src_l = m68ki_read_16(AY);
	AY -= 2;
	src_h = m68ki_read_16(AY);
	src = MAKE_INT_32(src_h, src_l);

	AX -= 2;
	dst_l = m68ki_read_16(AX);
	AX -= 2;
	dst_h = m68ki_read_16(AX);
	dst = MAKE_INT_32(dst_h, dst_l);

	res = dst { '+' if op == 'addx' else '-' } src { '+' if op == 'addx' else '-' } XFLAG_AS_1();

	FLAG_N = NFLAG_32(res);
	FLAG_V = VFLAG_{'ADD' if op == 'addx' else 'SUB'}_32(src, dst, res);
	FLAG_C = CFLAG_{'ADD' if op == 'addx' else 'SUB'}_32(src, dst, res);
	FLAG_X = FLAG_C;

	if(MASK_OUT_ABOVE_32(res) != 0)
		FLAG_Z = ZFLAG_CLEAR;

	m68ki_write_16(AX + 2, res & 0xffff);
	m68ki_write_16(AX, (res >> 16) & 0xffff);"""
        return m.group(1) + new_body + m.group(3)
    return re.sub(pattern, repl, content)

content = patch_op('addx')
content = patch_op('subx')

with open("m68k_in.c", "w") as f:
    f.write(content)
print("Patched ADDX/SUBX 32 mm")
