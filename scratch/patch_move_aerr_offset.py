import re

with open("m68k_in.c", "r") as f:
    content = f.read()

def patch_template(pattern, dest_type, size):
    def repl(m):
        code = m.group(0)
        # Find the EA call
        if "EA_AX_PD_32" in code:
            ea_call = "EA_AX_PD_32"
            pc_offset = 2
            reg_offset = 2
        elif "EA_AX_PD_16" in code:
            ea_call = "EA_AX_PD_16"
            pc_offset = 2
            reg_offset = 0
        elif "EA_AX_PI_32" in code:
            ea_call = "EA_AX_PI_32"
            pc_offset = 0
            reg_offset = -4
        elif "EA_AX_PI_16" in code:
            ea_call = "EA_AX_PI_16"
            pc_offset = 0
            reg_offset = -2
        else:
            return code
            
        write_call = f"m68ki_write_{size}"
        
        # Build replacement
        lines = code.split("\n")
        new_lines = []
        for line in lines:
            if write_call in line and "ea" in line:
                if pc_offset != 0:
                    new_lines.append(f"\tm68ki_aerr_pc_offset = {pc_offset};")
                if reg_offset != 0:
                    new_lines.append(f"\tm68ki_aerr_restore_reg = AX;")
                    new_lines.append(f"\tm68ki_aerr_restore_val = {reg_offset};")
                
                new_lines.append(line)
                
                if reg_offset != 0:
                    new_lines.append(f"\tm68ki_aerr_restore_reg = -1;")
                if pc_offset != 0:
                    new_lines.append(f"\tm68ki_aerr_pc_offset = 0;")
            else:
                new_lines.append(line)
                
        return "\n".join(new_lines)
        
    return re.sub(pattern, repl, content, flags=re.MULTILINE | re.DOTALL)

# Match move templates
content = patch_template(r"M68KMAKE_OP\(move, 32, pd, \.\)\n\{.*?\n\}", "pd", 32)
content = patch_template(r"M68KMAKE_OP\(move, 16, pd, \.\)\n\{.*?\n\}", "pd", 16)
content = patch_template(r"M68KMAKE_OP\(move, 32, pi, \.\)\n\{.*?\n\}", "pi", 32)
content = patch_template(r"M68KMAKE_OP\(move, 16, pi, \.\)\n\{.*?\n\}", "pi", 16)

# also handle pd, d and pd, a?
content = patch_template(r"M68KMAKE_OP\(move, 32, pd, [a-z]\)\n\{.*?\n\}", "pd", 32)
content = patch_template(r"M68KMAKE_OP\(move, 16, pd, [a-z]\)\n\{.*?\n\}", "pd", 16)
content = patch_template(r"M68KMAKE_OP\(move, 32, pi, [a-z]\)\n\{.*?\n\}", "pi", 32)
content = patch_template(r"M68KMAKE_OP\(move, 16, pi, [a-z]\)\n\{.*?\n\}", "pi", 16)

with open("m68k_in.c", "w") as f:
    f.write(content)
print("Patched m68k_in.c")
