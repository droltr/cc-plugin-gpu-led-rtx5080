# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- Initial `gpu-led-rtx5080` CoolerControl integration: GPU temperature colour mapping and a
  stable static-output runtime. The deferred settings UI remains experimental.
- `spectra-controller` build target for validated arbitrary RGB packets through the
  already verified NVIDIA RM/SPECTRA path.
- Accepted mode packet values 2–5 with read-back; the controller accepted them, but their
  human-visible effect names and timing/brightness semantics remain separately unverified.
- GPU plugin defaults now use the GPU temperature and the thermal-rgb colours remapped to
  40/56/72/88 °C, with static LED output enabled.
- First live plugin update selected the NVIDIA `GPU Temp` sensor at 50.0 °C and wrote
  static RGB `00 D3 77`; RM status read-back matched.
- Added thermal-rgb-style EMA smoothing and a two-channel RGB change threshold to avoid
  abrupt colour jumps and redundant static controller writes.
- Project plan and technical validation records (`docs/PLAN.md`, `docs/TESTING.md`,
  `docs/DEPLOYMENT.md`).
- `tools/nvrm-i2c-probe`: read-only RM API probe at 10 kHz; identified the LED
  controller `N765E-1001` at 0x4A on RM port 1.
- `tools/spectra-write-test`: first LED write test with two fixed packets (static
  blue, restore to breathing green) and read-back; verified on hardware.
- `tools/analysis`: FireStorm call-site decoder, NvAPI struct layout check, RM ABI
  comparison, speed-flag dump.
- `docs/research-report.md`, `docs/probe-log.md`, and `THIRD_PARTY_NOTICES.md`.

### Removed
- Obsolete references to the removed i2c-dev shell probe and an untracked license directory.
