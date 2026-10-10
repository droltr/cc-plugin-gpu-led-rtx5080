# Testing and validation

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
