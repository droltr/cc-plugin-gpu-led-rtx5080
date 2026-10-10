# Testing and validation

## Verified test environment

The documented hardware and software test evidence was collected on Bazzite 44.20261002.0
(Kinoite, `bazzite-nvidia-open`) with Linux kernel `7.2.8-ogc5.1.fc44.x86_64`, Python 3.14.7,
CoolerControl 5.0.1, and NVIDIA open kernel module 615.71.09. The target board is a ZOTAC
GAMING GeForce RTX 5080 SOLID CORE OC (`ZT-B50800J2-10P`), PCI `10de:2c02`, subsystem
`19da:1765`, on an AMD Ryzen 7 9800X3D system with an MSI MAG B850 TOMAHAWK MAX WIFI
(`MS-7E62`).

## Automated tests

The repository uses the Python standard library test runner. No third-party test package is
required.

```sh
python3 -m unittest discover -s tests -v
python3 -m compileall -q gpu_led tests
python3 -m json.tool config.json >/dev/null
bash -n install.sh run.sh
git diff --check
```

Automated tests do not access `/dev/nvidiactl`, CoolerControl, or physical LEDs. They validate
configuration parsing, colour conversion, GPU selection, and error handling only.

## Runtime validation

After installation, verify the deployed file set, service state, recent logs, selected GPU
sensor, configured mode, and the controller status path. A successful process start or command
exit does not prove a visible LED result. Physical LED validation requires a controlled test on
the selected GPU and a known restore packet.

## Hardware boundary

The RM controller utilities enforce the supported PCI identity, GPU count, device minor, strict
driver version, and controller ID before a write. Read-only status checks are required before
each write test. Automated tests must never perform physical hardware writes.
