# Production Ring engineering

This repository is for the Bravechip 603V1.23.2 production Ring. Read
`README.md`, `docs/firmware.md`, and the component reference before firmware work.
The older `botnetai/ring-firmware` Nordic/Seeed test-unit prototype has a
different board and BLE contract; do not treat it as production source.

Keep app adapters in `ShopItalic/app` and Caption firmware in
`ShopItalic/caption`. Preserve vendor licenses and source provenance when the
SDK is imported. Do not invent missing pin maps, fitted parts, codec settings,
or release commands. Record compiler/build evidence and distinguish it from
physical-device verification. Add unresolved work to `docs/backlog.md`.

Do not commit credentials, device provisioning secrets, build output, or a
supplier archive containing unreviewed files. Do not flash a physical device
unless the user explicitly requests that operation and the target is verified.
