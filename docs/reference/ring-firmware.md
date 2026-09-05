# Italic Ring raw firmware extraction

Extracted **2026-09-05** from the factory SDK for **603V1.23.2**. The factory
image identifies its software as **6.0.3.3Z62**. Historical Sudo and Bravechip
filenames remain unchanged so the artifacts can be matched to their source.

The raw application and the combined factory image are recovered and verified.
This is extraction of an existing factory distribution, not a readout from a
physical ring. For the corresponding electronics and mechanical revisions,
see the [component stack and BOM](sudo-ring-hardware.md).

## Files

All outputs are in
[`artifacts/ring-firmware/603v1.23.2-6.0.3.3z62`](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62).

| File | Contents | Size |
| --- | --- | ---: |
| [application.bin](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/application.bin) | Original raw ARM application payload from the OTA package. Load address `0x00027000`. | 184,132 bytes |
| [BCL603S2P_6.0.3.3Z62.hex](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/BCL603S2P_6.0.3.3Z62.hex) | Original combined Intel HEX: MBR, S140, application, bootloader, settings and two UICR words. Preserves sparse addressing. | 1,024,883 bytes |
| [BCL603S2P_6.0.3.3Z62.zip](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/BCL603S2P_6.0.3.3Z62.zip) | Original application-only Nordic OTA distribution, including signed init packet. | 184,715 bytes |
| [application.dat](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/application.dat) | Original signed DFU metadata. | 141 bytes |
| [internal-flash-1mib-ff-padded.bin](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/internal-flash-1mib-ff-padded.bin) | Derived image covering `0x00000000–0x000fffff`; absent HEX addresses filled with `0xff`. UICR is separate. | 1,048,576 bytes |
| [regions](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions) | Seven exact contiguous binary regions from the combined HEX, with no inserted padding. | 364,336 bytes total |
| [extraction.json](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/extraction.json) | Source/output hashes, addresses, DFU fields, component comparisons, and verification evidence. | JSON |
| [SHA256SUMS](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/SHA256SUMS) | Checksums for the extracted and derived outputs. | Text |
| [licenses](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/licenses) | Original S140, MBR, and Nordic SDK notices from the factory archive, with their original bytes and hashes preserved. | Text |

The padded 1 MiB file is useful for address-based analysis. Its synthesized
`0xff` gaps are **not measured device contents** and must not be treated as a
complete device backup. The original HEX is the preserved factory image.

Application SHA-256:

```text
96f1186e20f09ea2b97f48090626965437fb5e8cc4ff8b87679294cea8d853bf
```

## Recovered memory map

Addresses in this table are inclusive. The application bytes exactly match
the OTA payload. The bootloader and its UICR words exactly match the factory's
`s2p_mac_fd3_boot.hex` and byte-identical `s2p_mac_fd3_boot_no_reset.hex`.

| Address range | Bytes | Interpretation and binary |
| --- | ---: | --- |
| `0x00000000–0x00000aff` | 2,816 | [Master boot record](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions/00000000-00000aff.bin) |
| `0x00001000–0x00026633` | 153,140 | [Nordic S140 7.2.0 SoftDevice](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions/00001000-00026633.bin) |
| `0x00027000–0x00053f43` | 184,132 | [Application](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions/00027000-00053f43.bin) |
| `0x000f8000–0x000fddf7` | 24,056 | [Factory bootloader](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions/000f8000-000fddf7.bin) |
| `0x000fe000–0x000fe05b` | 92 | [DFU settings backup / MBR parameter page](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions/000fe000-000fe05b.bin) |
| `0x000ff000–0x000ff05b` | 92 | [Primary DFU settings](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions/000ff000-000ff05b.bin) |
| `0x10001014–0x1000101b` | 8 | [UICR boot addresses](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/regions/10001014-1000101b.bin): bootloader `0x000f8000`, MBR parameter page `0x000fe000` |

The combined HEX supplies 364,328 bytes of internal-flash content plus eight
UICR bytes. It does not supply the rest of UICR, including evidence of a
particular device's access-protection state. Nordic documents UICR as a separate
register region at `0x10001000`. [Nordic UICR reference](https://docs.nordicsemi.com/r/bundle/ps_nrf52840/page/uicr.html).

S140 identification is also read from the image itself. The SoftDevice
information structure at `0x00003000` reports family ID **140**, firmware ID
**0x0100**, version integer **7002000** (7.2.0), and reserved size **0x27000**.
Offsets and version encoding come from the SDK's S140 `nrf_sdm.h`; the reserved
size agrees with the recovered application's start address.

## Integrity and signature checks

The following checks passed locally:

- Verified the 69,437,016-byte source SDK against its recorded SHA-256.
- Verified ZIP member CRCs for the extracted OTA package and all of its files.
- Validated every Intel HEX record's length and checksum, the EOF record,
  and overlapping-address consistency. Independently reproduced all seven
  regions, the application, and the padded image with **IntelHex 2.3.0**.
- Matched the entire 184,132-byte OTA application to the combined HEX at
  `0x27000`; confirmed the embedded board and software version strings.
- Decoded the DFU protobuf and matched its embedded SHA-256 to the application.
- Verified the **ECDSA P-256 / SHA-256** signature over the serialized
  `InitCommand`, using the factory SDK's public verification key. That key's
  64-byte representation is also present in the matching bootloader at offset
  `0x5720`. Repeated verification with OpenSSL: **Verified OK**.
- Verified both 92-byte settings copies are identical, their settings CRC is
  `0x5363681e`, and their application size/CRC match the payload:
  **184,132 bytes / `0x15da5709`**.

The DFU metadata declares an application image, version counter **1**, hardware
code **52**, required SoftDevice **0x100**, and `is_debug = false`. Hardware code
52 is a generic DFU family value; it does not replace the app's specific
`603V1.23.2` board check.

Signature verification establishes consistency between this package and the
public key supplied in the factory SDK and bootloader. It is not independent
attestation of who controls the corresponding signing key. Only the public
verification material was used or copied.

The signed payload and byte order follow Nordic's
[DFU schema](https://raw.githubusercontent.com/NordicSemiconductor/pc-nrfutil/master/nordicsemi/dfu/dfu-cc.proto),
[package implementation](https://raw.githubusercontent.com/NordicSemiconductor/pc-nrfutil/master/nordicsemi/dfu/package.py),
and [signature implementation](https://raw.githubusercontent.com/NordicSemiconductor/pc-nrfutil/master/nordicsemi/dfu/signing.py).

To repeat the saved-output and public-signature checks from the repository root:

```bash
cd artifacts/ring-firmware/603v1.23.2-6.0.3.3z62
shasum -a 256 -c SHA256SUMS
openssl dgst -sha256 \
  -verify verification/dfu-public-key.pem \
  -signature verification/dfu-signature.der \
  verification/dfu-init-command.bin
```

## Source and variant boundaries

The source is `SUDO_DRIVE/Firmware/1.23.2_6033固件SDK.zip`, identified as SW2 in
the [BOM source manifest](sudo-ring/sources.json). Its SHA-256 is:

```text
ea3db9a85e153a49f77e716cd60c4127463484792235e91311b281f05a111d15
```

The original distribution members are:

```text
BCL603S2X/dfu/ota_output/BCL603S2P_6.0.3.3Z62.hex
BCL603S2X/dfu/ota_output/BCL603S2P_6.0.3.3Z62.zip
```

The factory's `creat_1232_dfu.bat` describes merging S140 7.2.0, the
`s2p_mac_fd3_boot.hex` bootloader, generated settings, and the application.
The recovered bytes and embedded metadata corroborate that combination.
Other files in `softdevice_boot/` include a different `app.hex`, S132, and
several bootloader variants; byte comparisons show they are not interchangeable
with this production image. The extraction manifest records those comparisons.

No ELF, AXF, or linker-map output was present in the inspected archive. The
raw application is available for analysis; this extraction did not rebuild
the factory C sources or generate debug symbols.

The preserved license notices accompany the identified Nordic components.
A complete file-level and third-party license inventory belongs to the
remaining SDK import and build review; this snapshot does not replace that
inventory.

## What would require a physical ring

A device readout is still needed to determine what a particular ring currently
contains: its exact flashed revision, runtime settings, bonds, calibration,
access-protection state, and any external 16 MiB recording-flash contents.
The factory distribution contains none of those per-device observations.

The MBA's USB inventory had no matching Nordic/SEGGER/debug probe during this
pass. No ring was connected through a debugger or BLE, and no flash, erase,
unlock, or recovery operation was performed. A physical extraction would start
with the confirmed board test-point mapping and read-only debug access;
erasing or recovering the chip would defeat the purpose of preserving its
existing contents.
