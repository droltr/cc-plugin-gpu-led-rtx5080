# cc-plugin-gpu-led-rtx5080

CoolerControl integration for temperature-driven control of the ZOTAC SPECTRA LEDs on a
ZOTAC GAMING GeForce RTX 5080 SOLID CORE OC. The production runtime reads the NVIDIA GPU
temperature from CoolerControl, maps it to the thermal-rgb colour scale, and sends validated
RGB packets to the GPU's SPECTRA controller through the NVIDIA Resource Manager interface.

## Problem

The target card exposes NVIDIA I²C adapters at 100 kHz, while its SPECTRA controller responds
at 10 kHz. Direct access through the normal `/dev/i2c-*` adapters therefore failed. The controller
was reachable through `/dev/nvidiactl`, using the NVIDIA RM I²C transaction path and the correct
device identity checks.

The CoolerControl host also exposes an AMD integrated GPU. Selecting the first GPU would be
unsafe because it could address the wrong device. The runtime selects the NVIDIA device by its
configured model and requires the `GPU Temp` sensor; it does not fall back to the AMD device.

## Verified environment

These versions identify the environment in which the runtime and hardware checks were performed.
They are recorded as test evidence, not as universal minimum versions.

| Component | Verified value | Evidence |
|---|---|---|
| Operating system | Bazzite 44.20261002.0, Kinoite, `bazzite-nvidia-open` | `/etc/os-release` |
| Linux kernel | `7.2.8-ogc5.1.fc44.x86_64` | `uname -a` |
| CPU | AMD Ryzen 7 9800X3D | `/proc/cpuinfo` |
| Motherboard | MSI MAG B850 TOMAHAWK MAX WIFI, MS-7E62, BIOS 2.A64 | DMI data |
| Discrete GPU | NVIDIA GeForce RTX 5080, GB203, PCI `10de:2c02` | PCI/sysfs and `nvidia-smi -q` |
| GPU board | ZOTAC GAMING GeForce RTX 5080 SOLID CORE OC, SKU `ZT-B50800J2-10P` | ZOTAC brochure |
| GPU subsystem | `19da:1765` | PCI/sysfs |
| NVIDIA driver | Open kernel module `615.71.09` | `/proc/driver/nvidia/version` |
| CoolerControl | `5.0.1` | `coolercontrol --version` and `coolercontrold --version` |
| Python | `3.14.7` | `python3 --version` |
| OpenRGB used for protocol research | Flatpak `org.openrgb.OpenRGB` `1.0` | Flatpak package information |

The board's SPECTRA controller was identified as MCU `N765E-1001` at I²C address `0x4A` on
NVIDIA RM port 1. The controller accepts the validated 10 kHz transaction path. The underlying
probe and reverse-engineering helpers are retained in the local ignored archive; the published
repository contains the resulting technical evidence and the guarded controller source.

## Technical solution

1. Read-only probing established that the normal NVIDIA i2c-dev adapters use 100 kHz and cannot
   reach the target controller.
2. NVIDIA RM transactions through `/dev/nvidiactl` reached the controller at 10 kHz and returned
   the expected controller ID and status bytes.
3. The guarded C controller validates PCI identity, GPU count, device minor, driver version,
   controller ID, and packet mode before a write.
4. A known static-blue packet was written and read back, then the previously verified
   breathing-green packet restored the LED state.
5. The Python runtime reads the NVIDIA `GPU Temp` sensor from CoolerControl, applies the
   temperature colour scale at 40/56/72/88 °C, smooths samples with a 3-second EMA, and writes
   only meaningful RGB changes.
6. The configured 75% value is a software RGB command limit. The available protocol evidence
   does not prove that it represents 75% electrical LED current or optical brightness.

The production configuration uses static LED output continuously and temperature-derived RGB
values. The settings UI remains an experimental branch and is excluded from the production
runtime and acceptance criteria because its save path has not been verified.

## Build and install

The Python runtime uses only the Python standard library. `requirements.txt` is intentionally
comment-only and documents that no third-party Python package is required.

The controller must be built against the exact NVIDIA `open-gpu-kernel-modules` checkout matching
the installed driver. The verified source tag is `615.71.09`, commit
`61dcc93722ecb418bb5f2e00923f05b4b8051dd1`.

```sh
make -C tools/spectra-write-test clean spectra-controller \
  NVIDIA_SRC=/path/to/open-gpu-kernel-modules \
  SPECTRA_SOURCE_COMMIT=61dcc93722ecb418bb5f2e00923f05b4b8051dd1 \
  SPECTRA_DRIVER_VERSION=615.71.09
./tools/spectra-write-test/spectra-controller --build-info
```

On Bazzite, install and restart CoolerControl with `run0`:

```sh
run0 env SPECTRA_NVIDIA_SRC=/path/to/open-gpu-kernel-modules bash install.sh
run0 systemctl restart coolercontrold
```

The installer verifies the controller hash and embedded build metadata, stages the complete
plugin, and retains the previous deployment in a timestamped backup. Do not delete a backup
until deployment files, service state, logs, and controlled hardware behavior have been checked.

## Validation

Run the repository checks with:

```sh
python3 -m unittest discover -s tests -v
python3 -m compileall -q gpu_led tests
python3 -m json.tool config.json >/dev/null
bash -n install.sh run.sh
git diff --check
```

The verified test suite contains 8 standard-library tests covering configuration parsing, colour
conversion, brightness limits, GPU selection, AMD exclusion, and error handling. Automated tests
do not access `/dev/nvidiactl` or write to physical LEDs. Hardware validation requires the
read-only status path, strict identity checks, read-back, and the known restore packet.

## Known issues

### In-game FPS drops while LEDs update

The reporter observed periodic FPS drops in Counter-Strike 2 on an RTX 5080 (driver
615.71.09) while the plugin was running. The drops appeared when the plugin was enabled and
went away when it was stopped (direct in-game observation, repeated A/B; no frame-time log
yet). Tracked in issue #4.

- Verified: each `spectra-controller set` call performs 3 I²C transactions of 32 bytes at
  10 kHz, and each transaction blocks other NVIDIA RM calls for about 32 ms (measured with
  a probe loop and a negative control; see [issue #4](https://github.com/droltr/cc-plugin-gpu-led-rtx5080/issues/4)).

> **⚠️ UNVERIFIED:** That this ~32 ms block is what lowers the in-game frame rate.
> To verify: record frame times (for example MangoHud with `log_interval=0`) with the plugin
> running and stopped, and align the spikes with the `spectra-controller` start timestamps.

Workaround until a fix lands: stop the plugin while gaming. No fix is implemented yet; the
planned direction is to send no I²C traffic while a game is rendering and to reduce the
number of transactions per colour change.

## Documentation

- [Research report](docs/research-report.md): hardware, protocol, source versions, and evidence.
- [Probe log](docs/probe-log.md): chronological technical observations and write validation.
- [Deployment guide](docs/DEPLOYMENT.md): Bazzite installation, provenance, rollback, and logs.
- [Testing guide](docs/TESTING.md): automated and hardware validation boundaries.
- [FPS drop test protocol](docs/FPS-DROP-TESTING.md): how to reproduce, test and report the in-game FPS drops.
- [Project plan](docs/PLAN.md): accepted scope and remaining validation work.
- [Third-party notices](THIRD_PARTY_NOTICES.md): NVIDIA and OpenRGB source attribution.
