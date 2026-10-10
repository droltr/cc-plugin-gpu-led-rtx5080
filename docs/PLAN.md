# Plan — gpu-rgb-rtx5080

Date: 2026-10-10. Status: core runtime implemented; validation and source review pending.
**Highest hardware risk of the RGB projects:** a wrong I²C write to a GPU can misconfigure on-board controllers.
Every write step needs explicit approval from the owner immediately beforehand.

## Steps

1. **Identify the model:** `nvidia-smi -q` (board part number), box or label, and
   ZOTAC's spec page. Record whether it has ARGB and which zones it has.
2. **Read-only probe:** find the NVIDIA I²C bus that has a device at `0x4B` and read
   the same register the OpenRGB detector reads (`0x10`), without writing.
   `ZotacBlackwellGPUControllerDetect.cpp` checks only that this read succeeds
   (`result >= 0`); it does not compare a value. Done 2026-10-10, see
   [probe-log.md](probe-log.md): no response.
3. **Local OpenRGB build** (distrobox, release_1.0 or master) with one added
   `REGISTER_I2C_PCI_DETECTOR` line for subsystem `0x1765`. Run detection only.
4. **First write, with the owner watching:** one static colour through OpenRGB;
   verify, then restore. Stop on any anomaly.
5. If it works: propose the PCI ID upstream (OpenRGB, Codeberg) with the probe evidence,
   and keep the local patch on a branch of our OpenRGB fork meanwhile.
6. Integration: the GPU becomes a zone for `cc-plugin-thermal-rgb` (GPU temperature
   is a natural source) and optionally for `cc-plugin-game-rgb`.

## Dependencies

- The OpenRGB build and fork decision from `cc-plugin-thermal-rgb` phase 0.
- `i2c-dev` is loaded and the buses carry `uaccess` ACLs (from `openrgb-udev-rules`). Already true.

## Status (2026-10-10)

- Step 1 done: SOLID CORE OC, `ZT-B50800J2-10P`, SPECTRA RGB (ZOTAC brochure).
- Step 2 done, with a change of method: the controller is at 0x4A (not 0x4B), answers
  only at 10 kHz, and is read through `/dev/nvidiactl` (RM port 1), not `/dev/i2c-*`.
  ID `N765E-1001`. See `research-report.md`.
- Step 3 changes: OpenRGB's existing ZOTAC Blackwell controller (0x4B) does not apply.
  The candidate is OpenRGB MR !3380 (0x4A protocol + RM API backend, open, not merged).
- Core plugin implementation exists: GPU temperature is the source, the thermal-rgb colour
  sequence is reused at 40/56/72/88 °C, and static GPU LEDs are enabled in the deployed
  configuration.
- The settings page is deferred on `test/ui-settings` and is excluded from stable acceptance.
- The source is being separated into the temperature/LED runtime and the configuration/UI layer.
- The controller binary source and exact build inputs still require a reproducible review before
  any rebuild is treated as necessary.

## Current evidence boundary

- Verified: controller reachability, device identification, packet read-back, static output,
  restore output, GPU temperature selection, and live temperature-driven RGB updates.
- Not yet accepted: the settings page save path, a stable release, a fresh controller rebuild,
  and the repository's final automated test run.
- Modes 2–5 have accepted packet values and status read-back, but their visual effect names and
  timing/brightness semantics remain separate evidence questions. See `docs/research-report.md`.
