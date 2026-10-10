# Project plan

## Scope

Provide a CoolerControl integration that reads the NVIDIA GPU temperature and drives the ZOTAC
SPECTRA LEDs through the verified NVIDIA RM transaction path. The production runtime is separate
from `cc-plugin-thermal-rgb` and does not depend on OpenRGB.

The settings page is experimental and remains isolated on `test/ui-settings` until its save path
and runtime propagation are independently verified.

## Implemented

- Identified the SPECTRA controller at `0x4A`, NVIDIA RM port 1, 10 kHz.
- Recorded guarded read-only identification and status evidence; the temporary probe sources are
  retained in the local archive rather than the runtime repository.
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
- Visual timing and brightness semantics for effect modes beyond the directly observed modes.
- The experimental settings-page save path.

## Hardware safety

Every physical write requires explicit authorization immediately before execution. The controller
must pass its device identity, GPU count, device minor, strict driver-version, and controller-ID
checks. Read-only status and read-back are required, and the known restore packet must remain
available before testing a new write.

## Current repository state

- The production runtime is deployed and its file hashes, embedded controller build metadata, and
  service status were verified on Bazzite.
- The GitHub repository contains the runtime, controller build source, tests, deployment material,
  and technical evidence. Temporary reverse-engineering helpers and session-only material are
  excluded and retained in the local ignored archive.
- The settings UI remains an experimental branch and is outside the production acceptance target.

## Remaining work

1. Rebuild the controller only after verifying the exact NVIDIA source checkout and headers.
2. Perform controlled hardware checks for effect timing and brightness semantics.
3. Develop and validate the settings page on its test branch.
4. Run dedicated `gitleaks` and `trufflehog` scans if those tools become available; the current
   environment does not contain either scanner.

## Status recorded 2026-10-10

The stable runtime, repository cleanup, technical documentation, local validation, and GitHub
Actions CI are complete. The repository is public at
`https://github.com/droltr/cc-plugin-gpu-led-rtx5080`. The deployed CoolerControl service is
active with zero restarts, and the runtime is producing temperature-driven RGB updates on the
verified Bazzite test environment. The four items above remain open and are intentionally deferred
until the next work session.
