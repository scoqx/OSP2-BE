import re

file_path = 'code/qcommon/osp_version.h'

with open(file_path, 'r') as file:
    lines = file.readlines()

version = None
for line in lines:
    if re.match(r'^\s*#define\s+OSP_VERSION\s+"', line) and not line.strip().startswith('//'):
        match = re.search(r'"(.*?)"', line)
        if match:
            version = match.group(1)
            break

if version:
    import json
    with open('version.json', 'w') as json_file:
        json.dump({"version": version}, json_file)

    print(f"✅ Version extracted: {version}")
else:
    print("Version not found")
