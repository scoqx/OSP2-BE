import re
import json

with open("code/cgame/cg_local.h", "r", encoding="utf-8") as f:
    content = f.read()
    match = re.search(r'#define\s+OSP_VERSION\s+"(.+?)"', content)
    if not match:
        raise ValueError("OSP_VERSION не найден в cg_local.h")
    version = match.group(1)

with open("version.json", "w", encoding="utf-8") as f:
    json.dump({"version": version}, f)
