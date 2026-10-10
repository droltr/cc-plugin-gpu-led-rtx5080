# Research report — reaching the RTX 5080 LEDs from Linux

Period: 2026-10-09 (project created) to 2026-10-10 (controller identified, first write).
Detailed per-step log: [probe-log.md](probe-log.md). Plan: [PLAN.md](PLAN.md).

Labels: unlabelled statements are verified and cite their source.
`⚠️ UNVERIFIED` = not confirmed yet (with what would verify it).
`⚠️ ASSUMPTION` = taken as given without proof (with what it rests on).

## 1. Result

The card's LED controller is a ZOTAC SPECTRA MCU at I²C address **0x4A** on NVIDIA
**RM port 1**. It reports the ID **`N765E-1001`** and only answers at **10 kHz**.
Linux's `/dev/i2c-*` adapters for NVIDIA GPUs always run at 100 kHz, so the controller
is invisible through them. It is reachable from user space through the NVIDIA Resource
Manager API (`/dev/nvidiactl`, `NV402C_CTRL_CMD_I2C_TRANSACTION` with the 10 kHz speed
flag). The first write test on 2026-10-10 set the LEDs to steady blue and restored them
to breathing green; the physical result was observed during the controlled test (section 4.5).

The status bytes read back (`01 01 11 FF 28 14 0A 08`) decode, using the layout in OpenRGB
MR !3380, to "on, breathe, RGB 11 FF 28". The controlled observation showed green breathing
output at that moment (2026-10-10). This confirms the status layout for this card.

## 2. Environment

| Item | Value | Source |
|---|---|---|
| OS | Bazzite (Kinoite), image `bazzite-nvidia-open`, booted deployment 44.20261006.1 (`/etc/os-release` reports 44.20261002.0) | `rpm-ostree status`, `/etc/os-release` |
| Layered packages | coolercontrol, liquidctl | `rpm-ostree status` |
| Kernel | 7.2.8-ogc5.1.fc44.x86_64 | `uname -r` |
| CPU / board | AMD Ryzen 7 9800X3D; MSI MAG B850 TOMAHAWK MAX WIFI (MS-7E62), BIOS 2.A64 | `/proc/cpuinfo`, DMI |
| GPU | NVIDIA GeForce RTX 5080, PCI `10de:2c02`, subsystem `19da:1765`, GPU part `2C02-400-A1`, bus `0000:01:00.0` | sysfs, `nvidia-smi -q` |
| Card | ZOTAC GAMING GeForce RTX 5080 SOLID CORE OC, SKU `ZT-B50800J2-10P` | ZOTAC brochure |
| VBIOS | 98.03.3B.C0.35 | `nvidia-smi` |
| Driver | NVIDIA open kernel module 615.71.09 (akmods build, 2026-10-06) | `/proc/driver/nvidia/version` |
| NVIDIA i2c adapters | i2c-8/9/10/11/12 = "NVIDIA i2c adapter 1/3/4/5/6 at 1:00.0" | `/sys/class/i2c-dev/*/name` |
| OpenRGB | flatpak `org.openrgb.OpenRGB` 1.0 (flathub, stable), running as SDK server on 127.0.0.1:6742 | `flatpak list` |
| Other lighting software running | coolercontrold 5.0.1 with plugin `thermal-rgb` (openrgb-python 0.3.8) | `ps`, plugin venv |
| Tools | i2c-tools 4.4, GNU objdump 2.46.1, gcc 16.2.1, Python 3.14.7, git 2.55.0, gh 2.102.0, innoextract 1.9 | `--version` |

## 3. Sources used

| Source | Version | Used for |
|---|---|---|
| ZOTAC brochure `ZT-B50800J2-10P-brochure.pdf` (zotac.com) | 2025 | model, "SPECTRA RGB lighting", "FireStorm Utility" |
| ZOTAC FireStorm installer artifact | 5.0.0.015E; local artifact hashes are retained outside the published repository | static disassembly (not executed) |
| NVIDIA NVAPI SDK, github.com/NVIDIA/nvapi | commit `70d337d` (2026-09-18) | NvAPI function IDs, `NV_I2C_INFO_V3`, I²C API docs |
| NVIDIA open-gpu-kernel-modules | tag `615.71.09`, commit `61dcc93` | Linux i2c adapter code, RM I²C control, ABI headers |
| OpenRGB, gitlab.com/CalcProgrammer1/OpenRGB | local clone at `release_1.0` / master 2026-09-11 (`81bbe18a`) | existing ZOTAC controllers |
| OpenRGB issue #5641 | opened 2026-05-28 | same subsystem `176519DA`, no captures |
| OpenRGB MR !3380 (open) | head `09496552`, 2026-07-13 | RM API backend idea, 0x4A protocol layout |
| OpenRGB MR !3186 (draft) | 2026-02-02 | "must use 10 kHz" note for ZOTAC 50 series |

## 4. Findings

### 4.1 FireStorm (Windows) — from disassembly, verified against NVAPI headers

- I²C is done with `NvAPI_I2CRead` (`0x2FDE12C5`) and `NvAPI_I2CWrite` (`0xE812EB07`)
  (`nvapi_interface.h` lines 90–91).
- Request struct `NV_I2C_INFO_V3`, version `0x3002C`; i386 offsets checked with
  `_Static_assert` from the archived analysis helper. Fixed values:
  displayMask 1, bIsDDCPort 1 (required by the NvAPI docs), portId 1, bIsPortIdSet 1,
  i2cSpeed `0xFFFF` (deprecated marker).
- Addresses used: 0x49, 0x4A, 0x4B (i2cDevAddress is documented as shifted left by one).
- Identification (function `0x41bba0`): read 32 bytes from 0x4A register 0x5A at 10 kHz
  and compare with `N764E`, `N765E`, `N717E`, `N777C`; otherwise read 32 bytes from 0x4B
  register 0xF0 at 100 kHz and compare with `N761W`, `N762E`, `N762A`, `N774C`, `N822A`,
  `N822E`.
- 0x4A call sites: two 32-byte reads and one **32-byte write** of register 0x5A at
  10 kHz (function `0x430880`).
- 63 I²C call sites in total; the archived analysis output is retained as research evidence.

### 4.2 Linux driver (Bazzite) — from NVIDIA source 615.71.09

- Adapter name "NVIDIA i2c adapter N" uses the RM port number N (`kernel-open/nvidia/nv-i2c.c:252`).
- Adapters are created only for RM ports reported IMPLEMENTED and VALID, plus DP-AUX
  adapters for on-chip DisplayPort outputs (`osapi.c`, `RmI2cAddGpuPorts`). The decision
  is made outside the open source tree. Ports 0 and 2 are not exposed on this system.
- Every adapter transfer uses `NV402C_CTRL_CMD_I2C_TRANSACTION` with `flags = 0`, which is
  `SPEED_MODE_100KHZ` (`osapi.c` `RmNonDPAuxI2CTransfer`; `ctrl402c.h:151–159`).
- `i2ctransfer` sends each message as a separate transaction (no repeated start);
  `i2cget` uses SMBus/I²C-buffer commands (`nv-i2c.c`).
- DP-AUX adapters accept only plain read/write; SMBus commands return
  `NV_ERR_NOT_SUPPORTED` (`osapi.c` ~4230).
- `NV402C_CTRL_CMD_I2C_TRANSACTION` flags `0x48` = NON_PRIVILEGED | ROUTE_TO_PHYSICAL:
  any client may call it, it is forwarded to GSP, and it takes the GPU lock.
- On fd close the driver frees all RM clients of that fd (`rm_cleanup_file_private`).

### 4.3 Windows vs Linux differences that mattered

| | Windows (FireStorm / NvAPI) | Linux (nvidia.ko i2c-dev) |
|---|---|---|
| Speed | per call; 10 kHz for 0x4A | fixed 100 kHz |
| Register read | combined format (repeated start) | `i2cget`: RM buffer read; `i2ctransfer`: separate messages |
| Ports | NvAPI portId 1 | only IMPLEMENTED+VALID RM ports as i2c-N; DP-AUX adapters separate |

### 4.4 Probe results

| Date | Method | Result |
|---|---|---|
| 2026-10-10 | `i2cget` 0x4B reg 0x10, all NVIDIA adapters | `Read failed` |
| 2026-10-10 | `i2cget` 0x4A reg 0x5A ×32, all NVIDIA adapters | `Read failed` (100 kHz) |
| 2026-10-10 | `i2ctransfer` on i2c-11 | success with `0x00` for every address → method invalid on that adapter |
| 2026-10-10 | `nvrm-i2c-probe` A: 0x4A reg 0x5A, 100 kHz, RM port 1 | `NV_ERR_I2C_ERROR` |
| 2026-10-10 | `nvrm-i2c-probe` B: plain read 0x30, 10 kHz (negative control) | `NV_ERR_GENERIC` |
| 2026-10-10 | `nvrm-i2c-probe` C: 0x4A reg 0x5A, 10 kHz, RM port 1 | `NV_OK`, `N765E-1001` + `01 01 11 FF 28 14 0A 08` |

GPU state (temperature, fan, power, clock, P-state) was recorded before and after every
probe and did not change beyond normal idle variation.

### 4.5 Write protocol (verified on hardware, 2026-10-10)

Write: RM port 1, address 0x4A, register 0x5A, `I2C_BUFFER_RW`, 32 bytes, 10 kHz.

| Byte | Field | Verified |
|---|---|---|
| 0 | reset | written as 0 only |
| 1 | on | 1 |
| 2 | mode | 0 = static (steady blue seen), 1 = breathe (breathing green seen) |
| 3–5 | R, G, B | `00 00 FF` = blue, `11 FF 28` = green |
| 6 | on-time | `0x2D` (static), `0x14` (breathe), as FireStorm sets them |
| 7 | off-time | `0x0A` |
| 8 | preset | `0x08` |
| 9–31 | zero | — |

Status read (same register, 32 bytes): 10-byte ID `N765E-1001`, then bytes 1–8 of the last
packet. Write test: `test` → read back matched, the physical observation showed steady blue; `restore` → read
back matched, the physical observation showed breathing green.

## 5. Code written

| Path | Purpose | Status |
|---|---|---|
| Historical RM probe and analysis helpers | retained in the local research archive and excluded from the runtime repository | results are recorded in this report and the probe log |
| `tools/spectra-write-test/` | `status` / `test` / `restore` with two fixed packets and read-back | first write verified (blue steady, restore to green breathe) |

## 6. Evidence controls

The probe and controller utilities use negative controls, strict device identity checks, strict
driver-version checks, read-back, and a known restore packet. Tool exit status alone is not
treated as evidence of device presence, command acceptance, or visible output. Unverified
protocol interpretations remain labelled in the probe log and are excluded from production
acceptance.

## 7. Open items

Modes 2–5 have since been written with the validated tool and accepted by the controller;
status read-back matched each packet and the verified restore packet followed each write. The
packet acceptance is verified. The human-visible names and exact timing/brightness semantics
remain separate evidence questions and are not treated as verified by read-back alone.

> **⚠️ UNVERIFIED:** that NvAPI port 1 and RM port 1 are the same port. Not needed for Linux
> control. To verify: NVIDIA documentation or a Windows NvAPI trace.

> **⚠️ UNVERIFIED:** why RM ports 0 and 2 are not exposed. To verify: NVIDIA documentation.

> **⚠️ UNVERIFIED:** the write made on 2026-10-10 to 0x49 on i2c-11 (adapter 5). Whether it
> reached any device cannot be determined from that adapter.

## 8. Next steps

Done: FireStorm write path decoded (`0x430880`); first write verified on hardware.

The project uses a **separate CoolerControl plugin** to drive the GPU LEDs
directly through the RM API. It is not part of `cc-plugin-thermal-rgb` and does not use
OpenRGB. It is the only writer to the GPU LEDs.

1. Keep the stable temperature/LED runtime separate from the deferred settings UI.
2. Verify the exact NVIDIA open-gpu-kernel-modules source and headers before deciding whether a
   controller rebuild is needed.
3. Run the final repository and hardware checks after source review; commit only reproducible
   results.
