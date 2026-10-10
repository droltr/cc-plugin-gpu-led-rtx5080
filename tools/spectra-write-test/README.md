# spectra-write-test

First write test for the ZOTAC SPECTRA LED controller (0x4A, RM port 1, 10 kHz) of the
RTX 5080 SOLID CORE OC. Build like `tools/nvrm-i2c-probe` (`make NVIDIA_SRC=…`).

```sh
./spectra-write-test 615.71.09 status    # read only
./spectra-write-test 615.71.09 test      # static blue, then read back
./spectra-write-test 615.71.09 restore   # breathing green, then read back
./spectra-controller 615.71.09 set static 255 0 0  # arbitrary RGB colour
```

Same card, GPU-count, minor and strict driver-version checks as `nvrm-i2c-probe`. It writes
only if the controller ID is `N765E-1001`. The `spectra-controller` target also accepts
validated `static` or `breathe` RGB values for the CoolerControl plugin. The legacy test
commands write two fixed packets:

| Packet | Bytes 0–8 (bytes 9–31 are zero) |
|---|---|
| test | `00 01 00 00 00 FF 2D 0A 08` |
| restore | `00 01 01 11 FF 28 14 0A 08` |

Result on 2026-10-10: both writes `NV_OK` with matching read-back; the owner saw steady
blue after `test` and breathing green after `restore`.
