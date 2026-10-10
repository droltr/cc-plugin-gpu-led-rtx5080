# Deployment

The production plugin is the temperature-driven runtime in `gpu_led/`. The settings page is
experimental and remains isolated on the `test/ui-settings` branch.

## Build prerequisites

- Python 3.10 or newer.
- A matching checkout of NVIDIA's `open-gpu-kernel-modules` source.
- A compiler and `make`.
- CoolerControl with access to the configured GPU temperature sensor.

The NVIDIA source checkout must match the installed driver. The controller uses NVIDIA RM ioctl
structures and is not rebuilt from an arbitrary header version.

## Install or upgrade

```sh
run0 env SPECTRA_NVIDIA_SRC=/path/to/open-gpu-kernel-modules \
  SPECTRA_NVIDIA_COMMIT=<verified-commit> bash install.sh
run0 systemctl restart coolercontrold
```

The installer builds the controller only from the exact Git commit and driver tag supplied by the
operator, stages the complete plugin, writes controller provenance metadata, preserves the
CoolerControl token from the configured token file, and retains the previous deployment in a
timestamped backup. The production install does not include the experimental UI.

When a previously built controller binary has been independently verified against the current
source and driver, it may be supplied explicitly:

```sh
run0 env SPECTRA_CONTROLLER_BIN=/path/to/verified/spectra-controller \
  SPECTRA_CONTROLLER_SHA256=<verified-sha256> \
  SPECTRA_CONTROLLER_SOURCE_COMMIT=<verified-source-commit> \
  SPECTRA_CONTROLLER_DRIVER_VERSION=615.71.09 bash install.sh
```

This mode skips compilation. The script compares the binary SHA-256 with the supplied digest and
compares the embedded source commit and driver version from `--build-info` with the supplied
values. A binary without embedded build metadata is rejected.

## Verify

```sh
systemctl is-active coolercontrold
systemctl is-active cc-plugin-gpu-led-rtx5080.service
journalctl -u cc-plugin-gpu-led-rtx5080.service -n 50 --no-pager
```

Verify that the deployed directory contains `lib/gpu_led/runtime.py`, `run.sh`,
`config.json`, `spectra-controller`, and `controller-build.txt`. Confirm the configured GPU model
and `GPU Temp` sensor before enabling writes. Compare the recorded controller hash with the
verified artifact before restarting the service.

## Rollback

Stop the plugin service, move the retained timestamped backup back to the deployment path, and
restart CoolerControl. Delete a backup only after the new deployment has passed file, service,
log, and hardware checks. A known-good LED restore packet is documented in
`tools/spectra-write-test/README.md`.
