import re

with open("m68kcpu.h", "r") as f:
    content = f.read()

# Remove m68ki_check_address_error_010_less from ea helpers
pattern = r"m68ki_check_address_error_010_less\([^;]+;\n"
content = re.sub(pattern, "", content)

with open("m68kcpu.h", "w") as f:
    f.write(content)
print("Patched EA helpers")
