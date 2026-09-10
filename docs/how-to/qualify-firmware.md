# Recover, reproduce, then qualify Ring firmware

The September 9 S05 engineering update transferred to a verified standard
603V1.23.2 Ring but failed physical acceptance: no version could be read after
the update, and the same iPhone could not reach service discovery in either
Italic or Nordic nRF Connect. See [the incident](../backlog.md#s05-physical-update-failure--2026-09-09).
No replacement image is qualified for installation by the work below.

**September 10 software result:** the restored ArmCC 5 baseline is byte-for-byte
identical to the preserved factory application. The repaired Sudo target cleanly
compiles and links with zero errors, retaining recording and all 123 Opus 1.6.1
sources. Host and linked ARM regressions pass. See the
[software qualification report](../reference/ring-armcc5-qualification.html)
for exact artifacts, all S05 finding dispositions and the task/interrupt budgets.

## Recovery is the first hardware dependency

Obtain the supplier's verified recovery procedure or programming fixture for
this exact board, including accessible contacts, electrical limits and
preservation instructions. Do not infer these from PCB pad labels or use a
generic erase/recover command. Preserve internal flash, UICR, provisioning and
external recording storage before recovery wherever readout is possible. If
protection prevents a backup, stop and report that fact before any erase.

On a verified spare/recoverable board, demonstrate restoration to the original
firmware and confirm version, BLE access and retained device data. Record the
exact image and physical target. Recover BCL6034776 using that verified path.
Neither normal advertising nor the presence of a factory distribution proves
that this path works. A charging case has not exposed a USB programming device
in the observed setup.

## Compile the original supplier baseline

Run from the repository root:

```sh
python3 tools/firmware/verify_baseline.py
python3 tools/firmware/prepare_vendor_baseline.py
python3 tests/firmware/test_vendor_baseline.py
```

The preparer requires a new output directory under `build/` and never overwrites
an existing preparation. It reads Git baseline `102bfd263952caaf532f0d53041b1f35bdfa943a`,
verifies the 7,056 preserved files, and retains the original project and sources.
The additional `vendor_baseline_build_only.uvprojx` contains only target `1.23.2`
and disables user-program build hooks. Its only active original hook was the
post-build `creat_1232_dfu.bat`, which packages/signs firmware. Compiler, linker,
assembly, library and source settings in the retained target are unchanged.
Preparation does not execute the compiler, batch files, signing tools or a debugger.

Use the exact configured **Arm Compiler 5.06 update 7 build 960**, with a
compatible license, `ARM.CMSIS.5.7.0` (CORE 5.4.0), and
`NordicSemiconductor.nRF_DeviceFamilyPack.8.35.0`.
The original µVision workflow runs on Windows. Arm also released this compiler
for Linux; a container can host its command-line tools while retaining the
compiler and runtime. Do not silently substitute Arm Compiler 6 or GNU.
Jeremy confirmed that Bravechip has a license for the sample-unit collaboration.
The Linux container still needs a compatible license. On September 10, a
legitimate MDK Professional evaluation was installed and a Windows build 960
compiler passed a compilation check on MBP-M5, as recorded below.

With the prepared directory as the working directory, the documented µVision
rebuild invocation is:

```text
UV4.exe -r firmware\BCL603S2X\app\project\mdk5\vendor_baseline_build_only.uvprojx -t "1.23.2" -o vendor-build.log
```

Resolve `UV4.exe` to the installed tool; this is a compile command, not a flash
command. The generated `baseline-preparation.json` pins the input project and
explicitly records `compiled`, `signed`, `flashed`, and `physicallyQualified`
as false. It is preparation evidence, not a completed build record.

Retain compiler identification, effective compile/link commands, the complete
build log, executable/map/HEX outputs and their SHA-256 hashes. Extract and
compare the application load bytes with the preserved factory
`artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/application.bin`. A mismatch must
be explained using supplier build provenance and disassembly; do not require
or claim byte identity merely because the SDK and distribution arrived together.
The September 10 build established byte identity: both files are 184,132 bytes
with SHA-256 `96f1186e20f09ea2b97f48090626965437fb5e8cc4ff8b87679294cea8d853bf`.
The only regenerated source-tree file was `RTE_Components.h`; its comments and
formatting changed while the `CMSIS_device_header "nrf.h"` definition remained
identical. The 7,055 other prepared original files remain byte-identical.

### Linux Docker host preparation

The [host Dockerfile](../../tools/firmware/armcc5/Dockerfile) follows the Linux
host choice in [Arm's CMSIS Docker setup](https://github.com/ARM-software/CMSIS_5/blob/develop/docker/dockerfile):
Ubuntu 20.04 for x86-64 with the 32-bit C/C++ and zlib runtime dependencies
needed by the older compiler. It pins the base image digest and direct package versions;
the image records the full resolved package list at
`/opt/armcc5-host-packages.txt`. Retain the resulting image ID and that list,
since transitive dependency resolution is not a frozen package mirror.

From the repository root, build and run the host check:

```sh
docker build --platform linux/amd64 -t sudo-armcc5-host:5.06u7 \
  -f tools/firmware/armcc5/Dockerfile tools/firmware/armcc5
docker run --rm --platform linux/amd64 --network none --read-only \
  sudo-armcc5-host:5.06u7
```

The probe verifies the ELF architecture and executes both a 64-bit x86 program
and the 32-bit x86 C library. On an Apple Silicon host, this tests the required
emulation paths. A passing probe establishes Linux host execution only;
`compilerExecuted`, `licenseValidated`, `firmwareCompiled`, and
`physicallyQualified` remain false. The image contains no Arm compiler or
license. Supply an authorized Linux installation separately and keep license
material outside the repository and image layers.

Completing a Linux build would require a successful compiler license checkout and preserving the selected
µVision compile/assembly/link settings in a Linux command-line build, and
validating its outputs. The Docker host check is not a firmware build entry point.
The qualified software build currently uses the working Windows route below.

**September 9 execution evidence on MBP-M5:** the host image built successfully,
and both x86 execution probes passed. The user-provided
[GitHub mirror](https://github.com/Qliangw/arm_compiler_bak/releases/tag/AC5)
supplied `ARMCompiler_506_Linux_x86_b960.tar.gz` (102,678,154 bytes,
SHA-256 `3ed676b0bc0f01baf17888c719f1b4c10c12a88517f70c4951dc85642ef63ce3`).
Its installer completed with exit code 0 after user approval of the bundled EULA.
The compiler identifies itself as **Arm Compiler 5.06 update 7 for Certification
(build 960)**; retain that product distinction when comparing it with the
supplier's actual build. No official Arm checksum has been matched to this mirror.

Both `armcc --vsn` and a one-function Cortex-M4 compile exited with code 1 and
`C9555E: Failed to check out a license`. The error reports that
`ARMLMD_LICENSE_FILE` is unset and the license file cannot be found. No smoke-test
object or firmware image was produced. This is an observed compiler prerequisite,
not a failure to emulate the Linux executable. The local
`build/diagnostics/armcc5-container/compiler-preflight.json` records the exact
image ID, executable hashes, commands, exit codes and log paths. The compiler
installation and its source archive remain under ignored `.local/` paths;
they are not included in the host image or committed source.

### Windows compiler on MBP-M5

On September 10, MDK 5.43a and Arm Compiler **5.06 update 7 for Certification
(build 960)** were installed in the CrossOver bottle
`Bravechip-ArmCC5-Trial`. Both exact project dependencies, CMSIS 5.7.0 and Nordic
nRF DeviceFamilyPack 8.35.0, are installed. This uses the Windows compiler and
µVision through CrossOver on Apple Silicon; it is separate from the Linux
Docker host above.

Arm issued an MDK Professional evaluation through its
[evaluation request form](https://www.keil.com/MDKEvaluationRequest/index/).
The registered compiler reported MDK Professional 5.43, the required compiler
build, and 30 days remaining on September 10. Both `armcc --vsn` and a Cortex-M4
one-function compilation exited 0; the latter created a 1,540-byte ARM object.
`build/diagnostics/armcc5-container/windows-compiler-preflight.json` records the
executable hashes, installed pack descriptors, object hash and compiler logs.
This establishes compiler execution and license checkout, independently of
firmware compilation or physical qualification.

The local build environment selects `ARM_TOOL_VARIANT=mdk_pro`, and µVision
registers the installed build 960 compiler in its normal compiler-path settings.
License material remains outside the repository. Do not copy `TOOLS.INI`,
license keys, or bottle registry contents into build evidence or source control.
Use the build-only projects above so supplier signing hooks do not execute.

### Repeat the recorded software build

The [CrossOver build runner](../../tools/firmware/armcc5/build_windows.py) checks
compiler selection, the actual licensed compiler version and disabled user-program
hooks. It records the compiler-file hashes, before/after source hashes, complete
build log, linker inputs, output hashes and actual per-object dependency inputs.
Use a new evidence directory for each run:

```sh
python3 tools/firmware/armcc5/build_windows.py \
  --project firmware/BCL603S2X/app/project/mdk5/sudo_voice.uvprojx \
  --target 'Sudo Voice 1.23.2' --source-root firmware \
  --output-stem build/firmware/sudo_voice/sudo_voice_candidate \
  --map firmware/BCL603S2X/app/project/mdk5/Listings/sudo_voice_candidate.map \
  --evidence build/diagnostics/armcc5-next --rebuild
sh tools/firmware/test.sh
```

For the baseline, use the prepared build-only project, target `1.23.2`, its
prepared `firmware` source root, `Objects/app` output stem and `Listings/app.map`.
µVision exit code 1 means a completed build with warnings; the runner also
requires the explicit zero-error summary. The final candidate clean rebuild
produced 351 objects, zero errors and 588 compiler warnings. There were no linker
warnings. Existing supplier/Opus warnings are retained in the full log.

Run the separate linked-image regression with Python packages
`unicorn==2.1.4` and `pyelftools==0.33` installed in a virtual environment:

```sh
python tools/firmware/qualify_armcc5_runtime.py \
  --elf build/firmware/sudo_voice/sudo_voice_candidate.axf \
  --bin build/firmware/sudo_voice/sudo_voice_candidate.bin \
  --map firmware/BCL603S2X/app/project/mdk5/Listings/sudo_voice_candidate.map \
  --callgraph build/firmware/sudo_voice/sudo_voice_candidate.htm \
  --output build/diagnostics/armcc5-next/runtime-regression.json
```

This executes the original Arm compressed-data startup, then the selected
callbacks, task logging paths, Opus self-test, kernel allocations and fault
hooks. It checks ELF/BIN identity, vectors, physical flash/RAM bounds and known
task paths with a Cortex-M4F context reserve. It deliberately expects a reset
only after injecting corruption into a real allocated task's stack guard.
The original S05 forensic reproducer remains unchanged and is not a pass/fail
qualification test for a new artifact.

The supplier linker configuration advertises RAM through `0x200405c8` and ROM
through `0x100000`, beyond the physical/reserved application boundaries. Keep
the original baseline settings for reproduction, but require the independent
post-link checks: actual RAM must end at or below `0x20040000`, and application
load bytes must end at or below `0xe0000`. The repaired candidate ends at
`0x20039048` in RAM and `0x77b98` in flash. Its 8 KiB C-heap reservation is
discarded by the linker because it is unused; the 8 KiB MSP stack is retained.

## Qualify one change at a time on recoverable hardware

Start with the unmodified supplier-toolchain build. Keep GNU porting and feature
changes separate. Pin the source diff and binary hash for each tested step;
do not change compiler, codec, scheduling and storage behavior in one baseline
comparison. First establish boot and BLE, then introduce dependent changes in
reviewed batches. A new failing batch stops advancement and is narrowed before
continuing.

For each exact candidate binary, retain physical evidence of:

- Hardware identity and firmware-version readback after boot.
- Two successful connect/disconnect/reconnect cycles on Jeremy's iPhone.
- One short recording transferred and played in Italic, with its file hash and
  transfer result; preservation of recordings that existed before the test.
- Per-task stack high-water measurements, minimum free RTOS heap and applicable
  C-heap use, plus reset/fault observations during that scenario. Explain limits
  and margins from measured behavior; successful linking is insufficient.
- Successful return to the verified baseline using the established recovery
  route, with identity, firmware and preserved-data checks afterward.

Host tests remain required for changed logic but cannot satisfy these physical
checks. The signature authenticates package bytes; it does not qualify them.
Do not publish a signed OTA catalog entry or use a working personal Ring as the
first unqualified hardware test. Public S05 OTA remains unavailable until its
exact build has passed physical acceptance and the failed-update cause is resolved.

Official references: [µVision command line](https://www.keil.com/support/man/docs/uv4/uv4_commandline.asp),
[µVision user-program hooks](https://www.keil.com/support/man/docs/uv4cl/uv4cl_dg_user.htm),
[Arm compiler download index](https://developer.arm.com/documentation/ka005198/1-0).
