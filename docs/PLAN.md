# Project plan

## Scope

Provide a CoolerControl integration that reads the NVIDIA GPU temperature and drives the ZOTAC
SPECTRA LEDs through the verified NVIDIA RM transaction path. The production runtime is separate
from `cc-plugin-thermal-rgb` and does not depend on OpenRGB.

The settings page is experimental and remains isolated on `test/ui-settings` until its save path
and runtime propagation are independently verified.

## Implemented

- Identified the SPECTRA controller at `0x4A`, NVIDIA RM port 1, 10 kHz.
- Added guarded read-only identification and status tools.
- Verified static and breathing packets with read-back and a restore packet.
- Added GPU temperature selection that excludes the unrelated AMD GPU.
- Added thermal colour mapping, EMA smoothing, 75% software RGB output scaling, and static LED
  runtime updates.
- Separated runtime, configuration parsing, and the experimental UI branch.
- Added reproducible standard-library automated tests and deployment documentation.

## Acceptance boundary

Accepted evidence covers controller identification, packet read-back, static output, breathing
output, GPU sensor selection, and live temperature-driven RGB updates.

The following remain separate validation items:

- A fresh controller rebuild from an exact `open-gpu-kernel-modules` checkout matching the
  installed NVIDIA driver.
- Deployment verification after installing the current reviewed source.
- Visual timing and brightness semantics for effect modes beyond the directly observed modes.
- The experimental settings-page save path.

## Hardware safety

Every physical write requires explicit authorization immediately before execution. The controller
must pass its device identity, GPU count, device minor, strict driver-version, and controller-ID
checks. Read-only status and read-back are required, and the known restore packet must remain
available before testing a new write.

## Next work

1. Install the reviewed source and verify the deployed file set and service logs.
2. Rebuild the controller only after the exact NVIDIA source checkout and headers are verified.
3. Perform the remaining controlled hardware checks.
4. Develop and validate the settings page on its test branch.
