import re
import subprocess

# Read the CMakeLists.txt file
with open('CMakeLists.txt', 'r') as file:
    content = file.read()

# Extract the SOURCES block
sources_block = re.search(r'set\(SOURCES\s+(.*?)\)', content, re.DOTALL).group(1)

# Extract individual file paths
file_paths = re.findall(r'"(.*?)"', sources_block)

# Apply cpplint to each file
for file_path in file_paths:
    if 'third-party' in file_path:
        continue

    if 'third-party-impl' in file_path:
        continue

    if 'MD5.cpp' in file_path:
        continue

    result = subprocess.run(['cpplint', file_path], capture_output=True, text=True)
    print(result.stdout)
    print(result.stderr)