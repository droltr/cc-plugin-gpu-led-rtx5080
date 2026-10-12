# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Known issues

- In-game FPS drops reported while LED updates run (issue #4). Cause not yet confirmed by a
  frame-time log; stopping the plugin removes the drops.

### Added

- Production CoolerControl integration `gpu-led-rtx5080` for the ZOTAC GAMING GeForce RTX
  5080 SOLID CORE OC (`ZT-B50800J2-10P`).
- NVIDIA RM controller path for the SPECTRA MCU `N765E-1001` at I²C address `0x4A`, RM port 1,
  and 10 kHz.
- GPU temperature selection that requires the NVIDIA `GPU Temp` sensor and excludes the AMD
  integrated GPU exposed by the same CoolerControl host.
- Thermal colour mapping at 40/56/72/88 °C, 3-second EMA smoothing, meaningful RGB-change
  filtering, and a 75% software RGB command limit.
- Guarded `spectra-controller` build target with embedded NVIDIA source commit and driver
  metadata, strict device checks, status read-back, and a known restore packet.
- Bazzite `run0` deployment flow with SHA-256 verification, timestamped rollback backups, and
  deployment provenance records.
- Standard-library automated tests, GitHub Actions CI, and technical deployment/research
  documentation.

### Changed

- Renamed the GitHub repository to `cc-plugin-gpu-led-rtx5080` and updated the manifest URL,
  README, and repository metadata.
- Replaced exploratory README wording with a problem, evidence, implementation, validation, and
  deployment description using the verified Bazzite, Linux, Python, CoolerControl, NVIDIA driver,
  CPU, motherboard, GPU, and board identifiers.
- Kept temporary protocol-analysis helpers, session-only records, and agent-only guidance in the
  local ignored archive instead of publishing them as runtime project files.

### Removed

- Temporary read-only probe sources and reverse-engineering helper sources from the published
  repository after preserving them in the local archive.
- Session records and workspace agent instructions from the public technical documentation.

### Validation status

- Verified environment: Bazzite 44.20261002.0, kernel `7.2.8-ogc5.1.fc44.x86_64`, Python 3.14.7,
  CoolerControl 5.0.1, and NVIDIA driver 615.71.09.
- Local automated checks: 8 tests passed; Python compilation, JSON validation, shell syntax, and
  whitespace checks passed.
- GitHub Actions CI passed for the published `main` commit.
- Experimental effect-settings UI remains outside the production acceptance target because its
  save path has not been verified.
