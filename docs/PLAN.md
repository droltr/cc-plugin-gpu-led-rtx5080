# Plan — gpu-rgb-rtx5080

Date: 2026-10-09. Status: investigation. **Highest hardware risk of the RGB
projects:** a wrong I²C write to a GPU can misconfigure on-board controllers.
Every write step needs explicit approval from the owner immediately beforehand.

## Steps

1. **Identify the model:** `nvidia-smi -q` (board part number), box or label, and
   ZOTAC's spec page. Record whether it has ARGB and which zones it has.
2. **Read-only probe:** find the NVIDIA I²C bus that has a device at `0x4B` and read
   the same identification register the OpenRGB detector reads (`0x10`), without
   writing. Compare with the AMP Extreme value expected by `ZotacBlackwellGPUControllerDetect.cpp`.
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
