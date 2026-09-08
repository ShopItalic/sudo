# Upstream libopus 1.6.1 import

This directory holds unmodified files from `opus-1.6.1.tar.gz`
(SHA-256 `6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1`, published at https://downloads.xiph.org/releases/opus/opus-1.6.1.tar.gz).
It is the pinned codec source for the Sudo Voice Opus recording profile.

- Every imported file and its SHA-256 is listed in `sudo-import-manifest.json`.
- `python3 tools/firmware/import_opus.py --verify` checks the files against that manifest.
- Only the portable C encoder/decoder subset is imported: `include/`, `celt/`, `silk/`,
  `silk/fixed/` and `src/` top-level sources plus `COPYING`, `AUTHORS`, `README`, `NEWS`
  and the upstream source lists. The optional neural-network code (`dnn/`), platform
  intrinsics, float SILK, demos, tests and build-system files are not imported and the
  firmware never enables DRED, OSCE or deep PLC.
- The separate supplier `opus-1.5.2` drop is preserved as imported and is not selected
  by the Sudo Voice profile.
- Do not edit files in this directory; re-run the import from the exact tarball instead.
