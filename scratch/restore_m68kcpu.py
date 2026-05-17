with open("m68kcpu.h", "r") as f:
    content = f.read()

import re

# 1. Add externs
if "extern uint           m68ki_aerr_fc;" in content:
    content = content.replace("extern uint           m68ki_aerr_fc;", 
        "extern uint           m68ki_aerr_fc;\nextern uint           m68ki_aerr_pc;\nextern int            m68ki_aerr_pc_offset;\nextern int            m68ki_aerr_restore_reg;\nextern int            m68ki_aerr_restore_val;")

# 2. Update m68ki_check_address_error
check_pattern = r"#define m68ki_check_address_error\(ADDR, WRITE_MODE, FC\) \\\n\s+if\(\(ADDR\)&1\) \\\n\s+\{ \\[\s\S]+?siglongjmp\(m68ki_aerr_trap, 1\); \\\n\s+\}"

def repl_check(m):
    orig = m.group(0)
    if "m68ki_aerr_restore_reg != -1" not in orig:
        lines = orig.split("\\\n")
        new_lines = []
        for line in lines:
            if "siglongjmp" in line:
                new_lines.append("\t\tm68ki_aerr_pc = REG_PC - 2 + m68ki_aerr_pc_offset; ")
                new_lines.append("\t\tm68ki_aerr_pc_offset = 0; ")
                new_lines.append("\t\tif(m68ki_aerr_restore_reg != -1) { REG_A[m68ki_aerr_restore_reg] += m68ki_aerr_restore_val; m68ki_aerr_restore_reg = -1; } ")
            new_lines.append(line)
        return "\\\n".join(new_lines)
    return orig
content = re.sub(check_pattern, repl_check, content)

# 3. Update m68ki_stack_frame_buserr
buserr_pattern = r"static inline void m68ki_stack_frame_buserr\(uint sr, uint address, uint write_mode, uint instruction, uint fc\)\n\{[\s\S]+?m68ki_write_16_fc\(REG_A\[7\], fc, instruction\);"
def repl_buserr(m):
    orig = m.group(0)
    orig = orig.replace("REG_PC - 2", "m68ki_aerr_pc")
    return orig
content = re.sub(buserr_pattern, repl_buserr, content)

# 4. Remove m68ki_check_address_error_010_less from ea helpers
ea_block = r"(static inline uint m68ki_ea_[a-z]+_p[di]_[0-9]+\(void\)\s+\{[\s\S]+?\})"
def repl_ea(m):
    return re.sub(r"\s+m68ki_check_address_error_010_less\([^;]+;", "", m.group(1))

content = re.sub(ea_block, repl_ea, content)

with open("m68kcpu.h", "w") as f:
    f.write(content)

print("Restored m68kcpu.h")
