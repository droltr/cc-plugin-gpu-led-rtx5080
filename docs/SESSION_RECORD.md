# Technical project record

This document records verified project state and implementation decisions. It does not contain
conversation transcripts, personal data, access credentials, or machine-specific filesystem
paths.

## Hardware and protocol

- Target device: ZOTAC GeForce RTX 5080 SOLID CORE OC with SPECTRA RGB lighting.
- Controller: SPECTRA MCU identified as `N765E-1001` at I²C address `0x4A` through NVIDIA RM
  port 1.
- Required transfer speed: 10 kHz. The exposed NVIDIA Linux i2c-dev adapters use 100 kHz and
  do not reach this controller.
- The controller is accessed through `/dev/nvidiactl` using the NVIDIA RM transaction API.
- The first verified write used static blue output and read-back; the restore packet produced
  breathing green output and matched the read-back bytes.

## Plugin scope

The production plugin is independent of `cc-plugin-thermal-rgb`. It reads the NVIDIA GPU
temperature from CoolerControl, maps it to the documented thermal colour stops, applies the
configured software RGB limit, and sends changes through the RM-backed controller.

The settings page is experimental and isolated on `test/ui-settings`. Its save path is outside
the production acceptance boundary.

## Verification boundary

Verified: controller identification, read-back, static output, breathing output, GPU temperature
selection, AMD GPU exclusion, and live temperature-driven RGB updates.

Pending acceptance: fresh controller rebuild from a matching NVIDIA source checkout, final
deployment verification after installation, and broader hardware effect semantics. Read-back of
an effect packet proves controller acceptance; it does not independently prove its visual timing
or brightness behavior.

## Deployment state

The reviewed source is the `gpu_led/` runtime and configuration parser on the verification branch.
The deployed directory must contain the complete package, including `lib/gpu_led/runtime.py`.
The experimental UI must not be present in the production deployment.
