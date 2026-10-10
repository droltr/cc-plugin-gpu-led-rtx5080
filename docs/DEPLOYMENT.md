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
sudo SPECTRA_NVIDIA_SRC=/path/to/open-gpu-kernel-modules bash install.sh
sudo systemctl restart coolercontrold
```

The installer builds the controller, stages the complete plugin, copies the runtime package,
preserves the CoolerControl token from the configured token file, and atomically replaces the
target directory. The production install does not include the experimental UI.

When a previously built controller binary has been independently verified against the current
source and driver, it may be supplied explicitly:

```sh
sudo SPECTRA_CONTROLLER_BIN=/path/to/verified/spectra-controller bash install.sh
```

This mode skips compilation. Do not use it for a binary whose source, driver compatibility, or
controller checks have not been verified.

## Verify

```sh
systemctl is-active coolercontrold
systemctl is-active cc-plugin-gpu-led-rtx5080.service
journalctl -u cc-plugin-gpu-led-rtx5080.service -n 50 --no-pager
```

Verify that the deployed directory contains `lib/gpu_led/runtime.py`, `run.sh`,
`config.json`, and `spectra-controller`. Confirm the configured GPU model and `GPU Temp` sensor
before enabling writes.

## Rollback

Stop the plugin service, restore the previous reviewed plugin directory, and restart
CoolerControl. Preserve the previous configuration and controller binary before replacing a
deployment. A known-good LED restore packet is documented in `tools/spectra-write-test/README.md`.
