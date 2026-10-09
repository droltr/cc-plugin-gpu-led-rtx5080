# gpu-rgb-rtx5080

**Status: investigation.** See [docs/PLAN.md](docs/PLAN.md).

Control the LEDs of the owner's **ZOTAC GeForce RTX 5080** from Linux, so the card
can join the temperature and game lighting.

## Hardware

- PCI `10de:2c02` (GB203, RTX 5080), subsystem `19da:1765` (ZOTAC).
- NVIDIA I²C buses are visible (`/dev/i2c-8` … `/dev/i2c-12`, "Device 10DE:2C02 Subsystem 19DA:1765").
- The exact retail model (for example SOLID, SOLID OC or AMP) is still to be confirmed
  from the box or `nvidia-smi -q`.

## Findings so far

- OpenRGB release_1.0 / `master` has a **ZOTAC Blackwell GPU controller**
  (`Controllers/ZotacBlackwellGPUController`, added 2026-04-09 by Eder Sánchez). It is
  registered for:
  - RTX 5080 AMP Extreme INFINITY, subsystem `0x1762`, I²C address `0x4B`
  - RTX 5090 SOLID OC and RTX 5090 D SOLID OC
- The owner's card is subsystem **`0x1765`**, which is **not** in the list. It is likely a
  sibling model that uses the same controller, but that is unverified.
- OpenRGB 1.0rc2 has no ZOTAC Blackwell support at all.

## Owner's note

"If we can reach the GPU LEDs too, that would be great." Planned as its own project.
