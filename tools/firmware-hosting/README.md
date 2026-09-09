# Ring firmware hosting

Cloudflare Worker `italic-ring-firmware`, Italic account
`1ea14927bf3044670c4c530a38be7b5a`, custom domain `firmware.italic.com`.
The Worker serves only static assets from ignored `build/firmware-hosting/public`.
No app/backend deployment or Ring flashing is involved.

Current S05 source: `8632de604b2bd79c103e43292e91eab4ba13aed1`.
CI run: https://github.com/ShopItalic/sudo/actions/runs/34237323221
Artifact: `sudo-voice-gnu-34237323221`.
The binary checksum must match both the artifact build summary and
`s05-manifest.json` before upload. Preserve the versioned path permanently;
new binary bytes require a new path and manifest.

The public metadata at `/ring/s05/manifest.json` has `otaAvailable: false`
and a null OTA package URL. The downloadable BIN is unsigned application-only
engineering output. Do not feed it to Nordic DFU or advertise it as installable.
Supplier signing, accepted counters/key, board identity and recovery remain
required before exposing an OTA package. ShopItalic/app commit `6e9ceb3` adds hosted checks/downloads alongside ZIP import.
The installable manifest schema is documented in that repository at
`docs/how-to/ring-firmware-updates.md`.

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
