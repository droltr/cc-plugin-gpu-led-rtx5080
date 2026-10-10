# nvrm-i2c-probe

Read-only probe that reads the ZOTAC SPECTRA controller ID at 10 kHz through the NVIDIA
Resource Manager API (`/dev/nvidiactl`). Linux `/dev/i2c-*` NVIDIA adapters run at a fixed
100 kHz and cannot reach this controller (see `docs/research-report.md`).

## Build

```sh
git clone --depth 1 --branch 615.71.09 \
    https://github.com/NVIDIA/open-gpu-kernel-modules.git /path/to/ogkm
make NVIDIA_SRC=/path/to/ogkm
```

The tag must match the installed driver (`/proc/driver/nvidia/version`).

## Run

```sh
./nvrm-i2c-probe 615.71.09
```

It refuses to run unless the GPU is `10de:2c02` / `19da:1765`, the only NVIDIA GPU, with
device minor 0, and the driver version matches exactly (strict RM API version check).
It then performs three fixed reads on RM port 1 (`bWrite = NV_FALSE`, no options):

| # | Read | Purpose |
|---|---|---|
| A | 0x4A register 0x5A, 32 bytes, 100 kHz | reproduces the i2c-dev result |
| B | plain read at 0x30, 1 byte, 10 kHz | negative control |
| C | 0x4A register 0x5A, 32 bytes, 10 kHz | FireStorm identification read |

Result on 2026-10-10: A `NV_ERR_I2C_ERROR`, B `NV_ERR_GENERIC`, C `NV_OK`
`N765E-1001` + `01 01 11 FF 28 14 0A 08`.

Third-party material: see `THIRD_PARTY_NOTICES.md`.
