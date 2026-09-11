# Ring firmware hosting

GitHub Releases are the only firmware distribution channel. There is no
Cloudflare Worker, custom domain, R2 bucket or other third-party asset host in
the path. The `italic-ring-firmware` Worker (last deployment
`e6dfa457-cc26-4cea-b807-6e758db5bae5`) and the `firmware.italic.com/ring/`
catalog were deleted on 2026-09-11, so the retired S05 URL no longer serves.

## Where each object lives

| Object | Location | Public URL |
| --- | --- | --- |
| Canonical catalog (committed source) | `tools/firmware-hosting/s05-manifest.json` | `https://raw.githubusercontent.com/ShopItalic/sudo/main/tools/firmware-hosting/s05-manifest.json` |
| OTA package / application image | GitHub Release asset | `https://github.com/ShopItalic/sudo/releases/download/<tag>/<asset>` |
| Release evidence | GitHub Release assets | `manifest.json`, `SHA256SUMS`, `provenance.json`, `ci-build-summary.json`, `toolchain-version.txt`, notices |

`ShopItalic/sudo` is public, so `raw.githubusercontent.com` and release assets
are anonymously readable; this supersedes the earlier private-repository
assumption. Keep the catalog as the single supported entry point: the app
discovers the package from `otaPackageURL`, never from a guessed file name.

Every release also attaches `manifest.json`, a byte-for-byte copy of the
committed catalog at that tag. That copy exists for provenance and offline
consumption; the committed file remains authoritative and is what clients
fetch.

## Trusted origins for the app

`ShopItalic/app` must allow downloads only from:

- `raw.githubusercontent.com` under `/ShopItalic/sudo/` (catalog);
- `github.com` under `/ShopItalic/sudo/releases/` (release page and asset URL);
- `objects.githubusercontent.com` and `release-assets.githubusercontent.com`,
  the signed redirect targets GitHub uses for release assets.

The catalog host is fixed at
`https://raw.githubusercontent.com/ShopItalic/sudo/main/tools/firmware-hosting/s05-manifest.json`.
Every other host, and any redirect that leaves this allowlist, must be refused.

## Withdrawn S05 (2026-09-11)

The S05 GNU build (`8632de604b2bd79c103e43292e91eab4ba13aed1`, CI run
34237323221, artifact `sudo-voice-gnu-34237323221`, BIN SHA-256
`e99d68e970476da98034e47c6f6a4872766f490be2fe7ea3ea55bf0c4eae3f3d`) uses the
GNU Newlib lock backend that calls `NVIC_SystemReset()` when a lock is taken in
interrupt context. The [S05 failure audit](../../docs/reference/ring-s05-failure-audit.html)
reproduced those reset routes and this is the exact binary that failed physical
acceptance on a 603V1.23.2 Ring. It is **not** uploaded to any GitHub Release.
`s05-manifest.json` has `withdrawn: true`, `otaAvailable: false`, a null
`binary.url` and `binary.flashable: false`.

No GitHub Release ever carried the S05 GNU binary, so there is nothing to
delete for it; the committed catalog is the only artifact and it advertises no
download. Do not create an S05 release for this build.

## Publish a release

1. Build with the authorized Arm Compiler 5 toolchain and run the identity and
   host checks (see [qualify firmware](../../docs/how-to/qualify-firmware.md)).
2. Update `s05-manifest.json` with the exact `sourceCommit`, `ciRunURL`,
   `toolchain`, `binary.sha256`, `binary.bytes` and, once supplier-signed, the
   `otaPackage` fields. A public downloadable image must be Arm Compiler 5
   built and marked `flashable: true`.
3. Verify:

   ```sh
   python3 tests/firmware/test_release_manifest.py
   sh tools/firmware/test.sh
   ```

4. Commit the catalog change.
5. Create the release with the helper (it re-runs the manifest policy, verifies
   the linked BIN identity when present, writes `SHA256SUMS` and attaches
   `manifest.json`):

   ```sh
   python3 tools/firmware-hosting/publish_release.py \
     --tag v6.0.3.3S05 \
     --assets-dir build/release
   ```

   The equivalent manual form is:

   ```sh
   gh release create v6.0.3.3S05 --repo ShopItalic/sudo \
     --title 'Italic Ring 6.0.3.3S05' --prerelease \
     build/release/*
   ```

6. Optionally enable [immutable releases](https://docs.github.com/code-security/concepts/supply-chain-security/immutable-releases)
   for the repository so tags and assets are locked and attested after
   publication. Withdrawing a bad release is still possible by deleting the
   whole release (see below).

## Withdraw a release

1. In `s05-manifest.json`, set `withdrawn: true`, a null `binary.url` and
   `otaAvailable: false`, clear `otaPackageURL`/`otaPackage`, and record the
   reason. Commit and push; the raw catalog updates within minutes.
2. Delete the release and its tag so the asset URLs return 404:

   ```sh
   gh release delete v6.0.3.3S05 --repo ShopItalic/sudo --cleanup-tag --yes
   ```

3. Confirm the dead URL and do not reuse the tag:

   ```sh
   curl -sSI https://github.com/ShopItalic/sudo/releases/download/v6.0.3.3S05/<asset>
   ```

   The request must return 404. `raw.githubusercontent.com` caches the catalog
   for a few minutes; pushing the updated commit is enough to refresh it, so no
   manual cache purge is required.

## Assets, licenses and provenance

Release assets include the build summary, compiler version, `SHA256SUMS`, and
the license notices from the published S04 supplier package plus libopus 1.6.1
COPYING. No supplier source archive, unit dump,
signing key or credentials are uploaded, and generated binary output stays
untracked (`build/` is ignored). A replacement package may only be published
after it is built with the authorized Arm Compiler 5 toolchain, inspected for
interrupt-context safety, embeds the expected version, and is referenced by a
manifest whose `binary.flashable` is `true`. Keep `otaAvailable` false until a
supplier-signed Nordic DFU package exists.