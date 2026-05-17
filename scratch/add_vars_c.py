with open("m68kcpu.c", "r") as f:
    content = f.read()

if "int     m68ki_aerr_restore_reg;" not in content:
    content = content.replace("int     m68ki_aerr_pc_offset = 0;", "int     m68ki_aerr_pc_offset = 0;\nint     m68ki_aerr_restore_reg = -1;\nint     m68ki_aerr_restore_val = 0;")

with open("m68kcpu.c", "w") as f:
    f.write(content)
