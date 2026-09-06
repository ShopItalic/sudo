# Sudo Ring Master Feature Spec

Aligned on 2026-04-13.

This document is the canonical alignment layer across:
- `Sudo PRD - 2026-04-10.xlsx`
- `Sudo Fenda Software IT戒指软件需求表V0.0.1_20260407.xlsx`
- `Sudo Ring Spec` in Notion

The goal is to map each feature to the right primitive, remove document conflicts, and define one master behavior for vendor communication and internal product documentation.

## Primitive Taxonomy

- `Read`: App queries device state or metadata and displays it.
- `Command`: App sends a one-shot instruction to the ring.
- `Config`: App writes a persistent setting or behavior mapping.
- `Transfer`: Audio or structured payload moves between ring and app.
- `Event`: Device or app emits an asynchronous status or alert.
- `Indicator`: LED or haptic behavior derived from a device state.
- `Policy`: Internal system rule or limit; not a user-facing operation.
- `Diagnostic`: Engineering or service capability.
- `Contract`: Protocol or media-format agreement between systems.
- `Hardware State`: Behavior driven by charge or battery state rather than an app control.

## Canonical Feature Decisions

### Device Identity and Status

- Hardware version
  - Primitive: `Read`
  - Master behavior: App can query and display hardware version.
- Firmware version
  - Primitive: `Read`
  - Master behavior: App can query and display firmware version.
- BLE MAC address
  - Primitive: `Read`
  - Master behavior: App can query and display BLE MAC address.
- Battery level and charge state
  - Primitive: `Read`
  - Master behavior: App displays battery percentage and charging/full/discharging state.
- Low battery alert
  - Primitive: `Event` + `Indicator`
  - Master behavior: Low battery is surfaced both in the app and on-device LED behavior.

### Audio Capture and Storage

- Button mapping
  - Primitive: `Config`
  - Master behavior: App can configure press once / press twice / press and hold behavior.
- Recording trigger
  - Primitive: `Policy`
  - Master behavior: Default recording flow is press-and-hold, release to stop.
- Max clip duration
  - Primitive: `Policy`
  - Master behavior: Each clip auto-stops at 10 seconds.
- Audio format
  - Primitive: `Contract`
  - Master behavior: 16 kHz, mono, Opus-compressed audio.
- Ring-side audio buffer
  - Primitive: `Policy`
  - Master behavior: Ring storage is a temporary buffer only, not a 5-day rolling recorder.
- Unsynced clip protection
  - Primitive: `Policy`
  - Master behavior: Unsynced clips must never be overwritten.
- Ring to app clip sync
  - Primitive: `Transfer`
  - Master behavior: On release, the clip is marked pending sync and transferred by BLE.
- ACK and buffer reclaim
  - Primitive: `Transfer` + `Policy`
  - Master behavior: Ring deletes local audio only after the app successfully receives it and sends ACK.
- Storage full
  - Primitive: `Event` + `Indicator`
  - Master behavior: New recording is blocked when the ring buffer is full.
- Clear ring recording cache
  - Primitive: `Command`
  - Master behavior: App can explicitly clear ring-side recording cache only; confirmation required; disabled while recording; no deletion of phone or cloud data.
- Phone/cloud retention
  - Primitive: `Policy`
  - Master behavior: Any multi-day retention rule belongs to phone-side or cloud-side storage, not ring-side storage.

### Power and Lifecycle

- Power on when charger is connected
  - Primitive: `Hardware State`
  - Master behavior: Charging contact can wake the ring.
- Auto shutdown on battery depletion
  - Primitive: `Hardware State`
  - Master behavior: Device powers down automatically when battery is exhausted.
- User-facing shutdown
  - Primitive: none for MVP
  - Master behavior: Not exposed in the normal app flow.
- User-facing reboot
  - Primitive: none for MVP
  - Master behavior: Not exposed in the normal app flow.
- Factory reset
  - Primitive: `Command`
  - Master behavior: App settings can trigger a confirmed factory reset.
- Ship mode
  - Primitive: `Diagnostic`
  - Master behavior: Reserved for factory, shipping, and engineering workflows.

### Update, Diagnostics, and Engineering

- Firmware OTA
  - Primitive: `Command` + `Event`
  - Master behavior: App settings can trigger OTA; progress and result states must be observable.
- Logs
  - Primitive: `Diagnostic`
  - Master behavior: Log capture, transfer, and parsing are engineering/service capabilities rather than end-user features.
- BLE protocol
  - Primitive: `Contract`
  - Master behavior: Current project may use Fenda proprietary BLE protocol, but the protocol must be disclosed to us and documented for future multi-OEM compatibility.
- BLE encryption
  - Primitive: `Contract`
  - Master behavior: AES-128 at the BLE link layer.

### Haptics and On-Ring Feedback

- Recording haptics
  - Primitive: `Indicator`
  - Master behavior: Ring can vibrate during recording and on recording stop.
- App-triggered haptic
  - Primitive: `Command`
  - Master behavior: App can send a haptic trigger notification to the ring.

## Canonical LED State Matrix

Use the PRD dated 2026-04-10 as the LED source of truth.

### No Charger Contact

- Recording
  - Primitive: `Indicator`
  - Behavior: White pulsing, 0.5s on / 0.5s off, continuous until recording ends.
- Transferring to device
  - Primitive: `Indicator`
  - Behavior: Green pulsing, 0.5s on / 0.5s off.
- Error
  - Primitive: `Indicator`
  - Behavior: Red pulsing, 0.5s on / 0.5s off, 3 cycles.
- Storage full
  - Primitive: `Indicator`
  - Behavior: Red solid, stays on until local cache is cleared.
- Low battery
  - Primitive: `Indicator`
  - Behavior: Red pulsing, 0.5s on / 0.5s off, 3 cycles.

### Charger Contact Present

- Charging
  - Primitive: `Indicator`
  - Behavior: White solid.
- Charging full
  - Primitive: `Indicator`
  - Behavior: Green solid.

## Document-Level Changes Required

### Fenda RFI Workbook

- Update ring storage language from 5-day rolling overwrite to temporary buffer plus ACK-based deletion.
- Add a dedicated `clear ring recording cache` command.
- Mark shutdown and reboot as not user-facing in MVP.
- Keep factory reset and ship mode, but position ship mode as engineering/factory only.
- Replace the old LED rows with the canonical matrix above.

### PRD Workbook

- Add explicit rows for storage policy, cache clear command, and MVP power-control scope.
- Keep the current PRD LED matrix as the canonical reference.
- Clarify that factory reset is confirmed and user-triggered from settings.

### Notion

- Update the software spec so it explicitly states:
  - ring storage is temporary
  - unsynced clips are never overwritten
  - ACK is required before local deletion
  - app can clear ring recording cache
  - low battery and LED behavior follow the canonical matrix

