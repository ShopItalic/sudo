# Sudo Voice firmware

This directory contains the production Ring firmware source for the
Bravechip 603V1.23.2 / nRF52840 target. It preserves the vendor baseline and
adds the Sudo Voice recording, BLE, power, touch, and haptic work. The source
is S04, published as an unsigned engineering RC. No physical qualification is claimed.

## Source status

- The vendor source baseline is preserved at commit `102bfd2`. The factory
  distribution is identified as `6.0.3.3Z62`; its extraction and hashes are
  documented in the [factory firmware evidence](../docs/reference/ring-firmware.md).
- The Sudo Voice engineering profile is `6.0.3.3S04`, a ten-byte build string
  plus NUL in the legacy version field. It is distinct from the factory
  `6.0.3.3Z62` image and has no release or anti-rollback approval.
- One Sudo worker owns recording and archive operations. The durable store
  uses the actual LittleFS implementation (`lfs.c` and `lfs_util.c`) with a
  shared externally mounted `lfs_t`; public store calls are serialized by the
  worker and use bounded memory.
- The standard 1.23.2 PDM adapter has an eight-slot capture queue and a
  completed-block tail guard. Stop drains complete DMA blocks only, so an
  unfinished DMA tail is never reported as audio.
- Raw ADPCM is stored under `/.sudo-rec/<16 lowercase hex ID>.raw`. Versioned,
  checksummed metadata on the same file records the Start parameters, durable
  byte/frame counts, running raw CRC-32/ISO-HDLC, completion/recovery state,
  and the legacy export name. Checkpoints expose only the last durable prefix;
  staged read-only recovery verifies raw bytes in bounded steps and keeps
  damaged or partial files intact.
- The native versioned voice protocol and the firmware app adapter provide
  ID-based Start/Stop/query/catalog/resume/transfer/receipt operations. The
  matching iOS adapter is in [ShopItalic/app](https://github.com/ShopItalic/app).
  A phone receipt is custody evidence only after the exact durable byte count
  and CRC have been checked and a receipt tombstone has been persisted before
  raw deletion.
- PTT until release, three mappable inputs, configurable hold activation and memo limits, touch tuning, battery error propagation and
  filtering, and haptic settings are wired into the Sudo profile. PTT has no
  artificial duration cap; a memo limit of `0` means unlimited. Device ranges,
  calibration, power draw, and physical behavior still require measurement.

## Validation boundary

The recording, capture, protocol, touch-tuning, and battery host tests include
real LittleFS RAM-NOR fault tests and sanitizer runs for the checked-in source.
The test script remains the entry point:

```sh
sh tools/firmware/test.sh
```

The integrated host run passes 16,902 C checks and six archive-normalizer
tests. Arm GNU 15.2.rel1 compiles and links all 225 sources with zero undefined
symbols and passing startup/vector checks. See the [candidate record](../docs/reference/ring-firmware-candidate.md)
for memory use, runtime locking, ABI warnings and local artifact hashes.
Vendor Arm Compiler 5.06 update 7 (build 960) reproduction, physical stack/heap
measurements and release-package checks remain pending. No physical ring has
been dumped or flashed; S04 RC1 is an unsigned prerelease, not hardware qualification.

## Evidence and build references

- [Firmware baseline and build gates](../docs/firmware.md)
- [Documentation index](../docs/README.md)
- [Factory firmware extraction and hashes](../docs/reference/ring-firmware.md)
- [Production hardware, BOM, and factory evidence](../docs/reference/sudo-ring-hardware.md)
- [Component BOM](../docs/reference/sudo-ring/bom.csv), [quoted cost BOM](../docs/reference/sudo-ring/quoted-bom.csv), [workbook extracts](../docs/reference/sudo-ring/source-extracts.json), and [source manifest](../docs/reference/sudo-ring/sources.json)
- [Source manifest](source-manifest.json)

Keep vendor filenames, licenses, build projects, linker scripts, bootloader,
SoftDevice requirements, and factory evidence unchanged. Keep local build
output and device-specific provisioning material out of Git.
