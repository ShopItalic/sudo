# Ring firmware hosting

Cloudflare Worker `italic-ring-firmware`, Italic account
`1ea14927bf3044670c4c530a38be7b5a`, custom domain `firmware.italic.com`.
The Worker serves only static assets from ignored `build/firmware-hosting/public`.
No app/backend deployment or Ring flashing is involved.

**WITHDRAWN 2026-09-11 — do not distribute or flash.** The S05 GNU build
(`8632de604b2bd79c103e43292e91eab4ba13aed1`, CI run 34237323221, artifact
`sudo-voice-gnu-34237323221`, BIN SHA-256
`e99d68e970476da98034e47c6f6a4872766f490be2fe7ea3ea55bf0c4eae3f3d`) uses the
GNU Newlib lock backend that calls `NVIC_SystemReset()` when a lock is taken in
interrupt context. The [S05 failure audit](../../docs/reference/ring-s05-failure-audit.html)
reproduced those reset routes and this is the exact binary that failed physical
acceptance on a 603V1.23.2 Ring. `s05-manifest.json` now has `withdrawn: true`,
`otaAvailable: false`, a null `binary.url` and `binary.flashable: false`.

To complete the withdrawal, an operator must redeploy and remove the live
asset. Wrangler uploads the current directory and does not delete files that are
no longer present, so delete the remote path explicitly before/after redeploy:

```sh
wrangler r2 object delete ...        # only if the Worker used R2
wrangler deploy --config tools/firmware-hosting/wrangler.jsonc
curl -sSI https://firmware.italic.com/ring/s05/8632de6/italic-ring-6.0.3.3S05-unsigned.bin
```

The last request must return 404. Also purge the Cloudflare cache for the path
and re-check `/ring/s05/manifest.json`. Until the 404 is confirmed, treat the
public URL as a live unsafe artifact.

A replacement may only be hosted after it is built with the authorized Arm
Compiler 5 toolchain, inspected for interrupt-context safety, embeds the
expected version, and references a manifest with a non-null `toolchain` and
`binary.flashable: true` plus `otaAvailable` still false until supplier signing.

Assets include build summary, compiler version, SHA256SUMS, the license notices
from the published S04 supplier package plus libopus 1.6.1 COPYING, and a
small download page. No supplier source archive, unit dump, signing key or
credentials are uploaded. Generated binary output stays untracked.

Deploy from the repository root using Wrangler 4:

```sh
wrangler deploy --dry-run --config tools/firmware-hosting/wrangler.jsonc
wrangler deploy --config tools/firmware-hosting/wrangler.jsonc
```

Verify the public manifest and download the binary, checking its SHA-256
against `s05-manifest.json`. A successful deployment alone is insufficient:
zone-level Cloudflare challenges may prevent native clients from downloading.

Deployment verified 2026-09-09: `e6dfa457-cc26-4cea-b807-6e758db5bae5`.
Public HTTPS BIN readback: 464,220 bytes, exact SHA-256 match.
Cloudflare rule `2b2c1081a1714735a75f22c155287cea` skips only Super Bot
Fight Mode for GET/HEAD on firmware.italic.com at `/` and `/ring/s05/`.
Matching requests remain logged; managed WAF and rate limiting are unchanged.
