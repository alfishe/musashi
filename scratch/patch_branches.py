import re

with open("m68k_in.c", "r") as f:
    content = f.read()

def insert_pc_check(op, body_matcher, is_jmp=False):
    global content
    pattern = rf"(M68KMAKE_OP\({op}\)\n\{{\n)([\s\S]+?)(\n\}})"
    def repl(m):
        body = m.group(2)
        if "m68ki_check_pc_address_error_010_less" in body:
            return m.group(0)
        
        # We inject uint original_pc = REG_PC; at the top
        new_body = "\tuint original_pc = REG_PC;\n" + body
        
        # We inject m68ki_check_pc_address_error_010_less(original_pc); before the end
        if not is_jmp:
            new_body += "\n\tm68ki_check_pc_address_error_010_less(original_pc);"
        else:
            # JMP and JSR and RTS have it at the end
            new_body += "\n\tm68ki_check_pc_address_error_010_less(original_pc);"
        return m.group(1) + new_body + m.group(3)
    content = re.sub(pattern, repl, content)

# bsr 8, bsr 16, bcc 8, bcc 16, bra 8, bra 16
for op in ["bsr, 8, \., \.", "bsr, 16, \., \.", "bcc, 8, \., \.", "bcc, 16, \., \.", "bra, 8, \., \.", "bra, 16, \., \."]:
    insert_pc_check(op, "")

# dbcc, dbf, dbcc_f (not sure exactly what they are named, let's search)
for op in ["dbcc, 16, \., \.", "dbf, 16, \., \."]:
    insert_pc_check(op, "")

# jmp, jsr, rts, rtr, rte
for op in ["jmp, 32, \., \.", "jsr, 32, \., \.", "rts, 32, \., \.", "rtr, 32, \., \.", "rte, 32, \., \."]:
    insert_pc_check(op, "")

with open("m68k_in.c", "w") as f:
    f.write(content)
print("Patched branches")
