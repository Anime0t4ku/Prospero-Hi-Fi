#!/usr/bin/env python3
"""Generate an lld version script: SDK-stub imports stay global, all else local."""
import re
import subprocess
import sys
from pathlib import Path

elf, stub_dir, out = sys.argv[1], sys.argv[2], sys.argv[3]

dyn = subprocess.check_output(
    ['readelf', '--dyn-syms', '--wide', elf], text=True)
undefs = set()
for line in dyn.splitlines():
    m = re.search(r'\bUND\b\s+(\S+)', line)
    if m:
        undefs.add(m.group(1).split('@')[0])

exports = set()
nm = subprocess.check_output(
    ['nm', '-D', '--defined-only', *map(str, sorted(Path(stub_dir).glob('*.so')))], text=True, stderr=subprocess.DEVNULL)
for line in nm.splitlines():
    parts = line.split()
    if len(parts) >= 3 and parts[1] in 'TWRD':
        exports.add(parts[2].split('@')[0])

shared = sorted(undefs & exports)
missing = sorted(undefs - exports)
with open(out, 'w') as f:
    # Undefined imports stay dynamic automatically; hide everything defined.
    f.write('{\nlocal:\n  *;\n};\n')
print(f'imports kept global: {len(shared)}')
print(f'non-stub undefs (must be weak or else fail): {len(missing)}')
for s in missing[:40]:
    print(f'  {s}')
