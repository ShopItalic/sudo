#!/usr/bin/env python3
"""Verify the imported factory bytes from Git without reading signing material."""
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[2]
BASELINE = '102bfd2'
manifest = json.loads((ROOT/'firmware/source-manifest.json').read_text())
expected = {item['path']:item for item in manifest['files']}
process = subprocess.Popen(['git','archive',BASELINE,'firmware'],cwd=ROOT,stdout=subprocess.PIPE)
seen = set()
with tarfile.open(fileobj=process.stdout,mode='r|') as archive:
    for member in archive:
        if not member.isfile():
            continue
        name=member.name.removeprefix('firmware/')
        if name=='source-manifest.json':
            continue
        item=expected[name]
        data=archive.extractfile(member).read()
        assert len(data)==item['bytes'],name
        assert hashlib.sha256(data).hexdigest()==item['sha256'],name
        seen.add(name)
assert process.wait()==0
assert seen==set(expected)
for file in subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0'):
    if not file:
        continue
    assert not file.startswith('.local/'),file
    assert not file.endswith('.key'),file
print(f'PASS: {len(seen)} factory source files match the archive manifest at {BASELINE}; private archive is outside Git')
