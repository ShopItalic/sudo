#!/usr/bin/env python3
"""Fetch pinned Newlib headers for GCC object checks; no firmware is produced."""
import hashlib
from pathlib import Path
import tarfile
import urllib.request

root=Path(__file__).resolve().parents[2]
url='https://sourceware.org/pub/newlib/newlib-4.5.0.20241231.tar.gz'
digest='33f12605e0054965996c25c1382b3e463b0af91799001f5bb8c0630f2ec8c852'
destination=root/'.local/toolchains'
destination.mkdir(parents=True,exist_ok=True)
archive=destination/'newlib.tar.gz'
if not archive.exists():
    temporary=archive.with_suffix('.partial')
    with urllib.request.urlopen(url,timeout=30) as response, temporary.open('wb') as out:
        while chunk:=response.read(1024*1024):
            out.write(chunk)
    assert hashlib.sha256(temporary.read_bytes()).hexdigest()==digest
    temporary.replace(archive)
assert hashlib.sha256(archive.read_bytes()).hexdigest()==digest
with tarfile.open(archive) as source:
    members=[m for m in source.getmembers() if '/newlib/libc/include/' in m.name]
    source.extractall(destination,members=members,filter='data')
print('Pinned Newlib 4.5.0 headers ready for ARM object checks')
