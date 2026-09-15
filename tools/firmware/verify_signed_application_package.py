#!/usr/bin/env python3
"""Independently verify a factory-family application-only Nordic DFU package.

Uses the preserved supplier dfu-cc.proto field numbers and OpenSSL ECDSA
verification. Does not load a private key, publish, connect, or flash.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
FACTORY = ROOT / 'artifacts/ring-firmware/603v1.23.2-6.0.3.3z62'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def varint(data, offset):
    value = 0
    for shift in range(0, 70, 7):
        require(offset < len(data), 'Truncated protobuf varint')
        byte = data[offset]
        offset += 1
        value |= (byte & 127) << shift
        if byte < 128:
            return value, offset
    raise ValueError('Oversized protobuf varint')


def fields(data):
    result = {}
    offset = 0
    while offset < len(data):
        tag, offset = varint(data, offset)
        number, wire = tag >> 3, tag & 7
        require(number > 0 and number not in result, 'Unexpected or repeated field')
        if wire == 0:
            value, offset = varint(data, offset)
        elif wire == 2:
            size, offset = varint(data, offset)
            require(size <= len(data) - offset, 'Truncated protobuf field')
            value = data[offset:offset + size]
            offset += size
        else:
            raise ValueError('Unexpected protobuf wire type')
        result[number] = value
    return result


def der_integer(little_endian):
    value = little_endian[::-1].lstrip(b'\0') or b'\0'
    if value[0] & 128:
        value = b'\0' + value
    return bytes((2, len(value))) + value


def verify(package, binary, version):
    require(package.stat().st_size <= 4 * 1024 * 1024, 'Oversized ZIP')
    with zipfile.ZipFile(package) as archive:
        entries = archive.infolist()
        require(len(entries) == 3, 'Expected application BIN, DAT and manifest only')
        require(sum(i.file_size for i in entries) <= 4 * 1024 * 1024, 'Oversized expanded ZIP')
        names = [i.filename for i in entries]
        require(len(set(names)) == 3 and 'manifest.json' in names, 'Invalid ZIP directory')
        require(all(Path(n).name == n and '\\' not in n and ':' not in n for n in names), 'Unsafe ZIP path')
        require(archive.testzip() is None, 'ZIP CRC failure')
        manifest = json.loads(archive.read('manifest.json'))['manifest']
        require(set(manifest) == {'application'}, 'Package changes more than application')
        app = manifest['application']
        require(set(app) == {'bin_file', 'dat_file'}, 'Unexpected application manifest')
        require(set(names) == {'manifest.json', app['bin_file'], app['dat_file']}, 'Manifest entry mismatch')
        image = archive.read(app['bin_file'])
        packet = archive.read(app['dat_file'])
    require(image == binary.read_bytes(), 'Package image differs from qualified binary')
    require(image.count(version.encode() + b'\0') == 2, 'Firmware identity mismatch')
    initial_sp, reset = struct.unpack('<II', image[:8])
    require(0x20004758 < initial_sp <= 0x20040000 and reset & 1, 'Invalid vectors')
    require(0x27000 <= (reset & ~1) < 0x27000 + len(image) <= 0xe0000, 'Application flash bounds')
    outer = fields(packet)
    require(set(outer) == {2}, 'Package must contain signed command only')
    signed = fields(outer[2])
    require(set(signed) == {1, 2, 3} and signed[2] == 0 and len(signed[3]) == 64, 'Expected ECDSA P-256 signature')
    command = fields(signed[1])
    require(set(command) == {1, 2} and command[1] == 1, 'Expected INIT command')
    init_bytes = command[2]
    init = fields(init_bytes)
    require(set(init) <= set(range(1, 11)), 'Unknown init field')
    require(init.get(1) == 1 and init.get(2) == 52, 'Factory version counter / hardware mismatch')
    sd = init.get(3, b'')
    if isinstance(sd, bytes):
        sd_value, end = varint(sd, 0)
        require(end == len(sd) and sd_value == 0x100, 'Wrong SoftDevice requirement')
    else:
        require(sd == 0x100, 'Wrong SoftDevice requirement')
    require(init.get(4) == 0 and init.get(5, 0) == 0 and init.get(6, 0) == 0, 'Not application-only')
    require(init.get(7) == len(image) and init.get(9, 0) == 0, 'Wrong image size or debug bypass enabled')
    digest = fields(init[8])
    require(set(digest) == {1, 2} and digest[1] == 3, 'Expected SHA-256 image hash')
    require(digest[2] == hashlib.sha256(image).digest()[::-1], 'Signed image hash mismatch')
    if 10 in init:
        validation = fields(init[10])
        require(validation.get(1) == 1 and validation.get(2) == b'', 'Unexpected boot validation policy')
    signature = der_integer(signed[3][:32]) + der_integer(signed[3][32:])
    signature = bytes((0x30, len(signature))) + signature
    public = FACTORY / 'verification/dfu-public-key.pem'
    with tempfile.TemporaryDirectory(prefix='dfu-public-verification-') as folder:
        sig_path = Path(folder) / 'signature.der'
        sig_path.write_bytes(signature)
        result = subprocess.run(['openssl', 'dgst', '-sha256', '-verify', str(public),
                                 '-signature', str(sig_path)], input=init_bytes, capture_output=True)
        require(result.returncode == 0, 'ECDSA signature verification failed')
    der = subprocess.check_output(['openssl', 'pkey', '-pubin', '-in', str(public), '-outform', 'DER'])
    require(len(der) == 91 and der[-65] == 4, 'Unexpected factory public-key encoding')
    point = der[-64:]
    boot_key = point[:32][::-1] + point[32:][::-1]
    boot = (FACTORY / 'regions/000f8000-000fddf7.bin').read_bytes()
    require(boot.count(boot_key) == 1, 'Public key not uniquely present in preserved factory bootloader')
    return {'status': 'pass-independent-signed-application-package', 'version': version,
            'packageSha256': hashlib.sha256(package.read_bytes()).hexdigest(),
            'packageBytes': package.stat().st_size, 'binarySha256': hashlib.sha256(image).hexdigest(),
            'binaryBytes': len(image), 'applicationVersionCounter': 1, 'hardwareVersion': 52,
            'softdeviceRequirement': '0x0100', 'signature': 'ECDSA-P256-SHA256',
            'signatureVerified': True, 'publicKeyFoundInFactoryBootloader': True,
            'applicationOnly': True, 'debugBypass': False,
            'physicalBootProven': False, 'recoveryProven': False, 'flashed': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), 'Evidence file already exists')
    result = verify(args.package, args.binary, args.version)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
