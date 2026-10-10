# Probe log

Rule: unlabelled statements are verified and cite their source. Everything else carries
one of these labels at the start of the line:

- `> **⚠️ UNVERIFIED:**` not confirmed by a source or a validated test, plus "To verify: …";
- `> **⚠️ ASSUMPTION:**` taken as given without proof, plus "Based on: …".

Labelled items are never grounds for a hardware write.

## Card identity (verified)

| Fact | Source |
|---|---|
| PCI `10de:2c02`, subsystem `19da:1765` | sysfs `/sys/bus/pci/devices/0000:01:00.0` |
| GPU part `2C02-400-A1`, VBIOS `98.03.3B.C0.35`, board part number `N/A` | `nvidia-smi -q` |
| Retail model ZOTAC GAMING GeForce RTX 5080 SOLID CORE OC, SKU `ZT-B50800J2-10P` | ZOTAC brochure |
| Features list "SPECTRA RGB lighting" and "ZOTAC GAMING FireStorm Utility" | ZOTAC brochure |

ZOTAC brochure: <https://www.zotac.com/download/mediadrivers/External/GraphicsCard/5080/Brochure/ZT-B50800J2-10P-brochure.pdf>.
Its embedded file paths mention `ZT-B50800D2-10P (N765)`.

> **⚠️ UNVERIFIED:** that `ZT-B50800D2-10P` is the non-OC SOLID CORE.
> To verify: ZOTAC's official product page or brochure for that SKU (so far only a retailer listing says so).

> **⚠️ UNVERIFIED:** whether `N765` is also the board code of `ZT-B50800J2-10P`.
> To verify: a ZOTAC document for the J2 SKU naming its board code, or the card's own ID string.

## 2026-10-10 — read-only probe at 0x4B

Pre-flight:
- OpenRGB 1.0 (flatpak) server held no device on the NVIDIA buses (SDK device list).
- NVIDIA adapters exposed by the driver: i2c-8, i2c-9, i2c-10, i2c-11, i2c-12, named
  "NVIDIA i2c adapter 1, 3, 4, 5, 6 at 1:00.0".

`i2cget` SMBus read byte data, address 0x4B, register 0x10, on all five buses:
`Read failed` everywhere. No writes. GPU state unchanged.

## 2026-10-10 — plain read at 0x49, then V2 version query (RETRACTED finding)

- `i2ctransfer r32@0x49` returned 32 × `0x00` on i2c-11 and an I/O error on the others.
- One controlled write was sent on i2c-11:
  `i2ctransfer -y 11 w8@0x49 0xA0 0xF1 0x00 0x00 0x00 0x00 0x00 0x00`; the transfer
  returned success, the following read returned 32 × `0x00`.
- The controlled observation showed no visible lighting change afterwards.

**Retraction.** A later negative control showed that on i2c-11 raw `i2ctransfer`
reads return `0x00` with success for 0x30, 0x55, 0x6A and 0x77, while i2c-8/10/12 return an I/O error for 0x30. A success
on i2c-11 therefore does not show that a device exists. The "device at 0x49" finding
is withdrawn.

> **⚠️ ASSUMPTION:** no devices exist at 0x30, 0x55, 0x6A, 0x77 on i2c-11.
> Based on: i2c-8/10/12 return an I/O error at 0x30; no board device list is available.
> Even without this assumption, success at every tested address means a success says nothing.

> **⚠️ UNVERIFIED:** whether the 0x49 write reached any device.
> To verify: not possible from these transfers; i2c-11 reports success regardless.

## 2026-10-10 — 0x4A and register reads

- `i2cget` block read, 0x4A register 0x5A, 32 bytes: `Read failed` on all five buses.
- `i2cget` register reads at 0x49 on i2c-11 (0x10, 0x20–0x2F, 0xA0, 0xB8): `Read failed`.
- `i2ctransfer` combined write-register/read on i2c-11 returned `0x00` for 0x49, 0x4A
  and 0x4B alike, which per the negative control above proves nothing.
- EDID header `00 FF FF FF FF FF FF 00` read at 0x50 on i2c-9 only.
- GPU state unchanged throughout (45 °C, ~20 W, P8).

## Static analysis of FireStorm 5.0.0.015E (not executed)

Source: `ZOTACGAMING-RTX50-FireStorm.zip` (SHA-256
`bb16ca6147d7de99b166da50ff90c0ccc9d1cb6e684155b15c60ecc4bb6fea58`), Inno Setup
installer unpacked with `innoextract`, `app/FireStorm.exe` disassembled with `objdump`.
Read directly from the code:

- I²C goes through NvAPI QueryInterface IDs `0x2FDE12C5` = `NvAPI_I2CRead` and
  `0xE812EB07` = `NvAPI_I2CWrite` (source: NVIDIA/nvapi `nvapi_interface.h` lines 90–91,
  commit `70d337d`).
- The request is `NV_I2C_INFO_V3` (source: NVIDIA/nvapi `nvapi.h`). Its i386 offsets were
  checked with `_Static_assert` under `gcc -m32` (all match; a wrong size is rejected):
  version 0x00 = `0x3002C`, displayMask 0x04 = 1, bIsDDCPort 0x08 = 1, i2cDevAddress 0x09,
  pbI2cRegAddress 0x0C, regAddrSize 0x10, pbData 0x14, cbSize 0x18, i2cSpeed 0x1C =
  `0xFFFF` (`NVAPI_I2C_SPEED_DEPRECATED`), i2cSpeedKhz 0x20, portId 0x24 = 1,
  bIsPortIdSet 0x28 = 1.
- NVIDIA's documentation for NvAPI_I2CRead/Write says the request must have
  `bIsDDCPort = 1`, and that with a register address the read uses the I²C combined
  format (register write, then read). So `bIsDDCPort = 1` is an API requirement, not a
  hint about the bus.
- i2cDevAddress values: `0x92`, `0x94`, `0x96`. NVIDIA documents this field as the
  address shifted left by one, so the 7-bit addresses are 0x49, 0x4A, 0x4B.
- Function `0x41bba0` calls the read helper with (address 0x4A, value 0x5A, length 32)
  and compares the returned bytes with `N764E`, `N765E`, `N717E`, `N777C`; otherwise it
  calls it with (0x4B, 0xF0, 32) and compares with `N761W`, `N762E`, `N762A`, `N774C`,
  `N822A`, `N822E`. Each match stores a number in one object field (initialised to -1).

  The values 0x5A / 0xF0 go into pbI2cRegAddress with regAddrSize 1, so they are
  register addresses read in combined format; speed field = 2 (`NVAPI_I2C_SPEED_10KHZ`).

  > **⚠️ ASSUMPTION:** this function is the card identification step that selects a
  > lighting protocol.
  > Based on: the ASCII model-code comparisons and OpenRGB's comment that 0x4B register
  > 0xF0 holds a version string like "N762E…".
- No literal `0x1765` subsystem comparison was found in the code searched so far.

None of these reads answered on the five buses Linux exposes.

> **⚠️ UNVERIFIED:** which Linux i2c adapter, if any, corresponds to FireStorm's NvAPI port 1.
> To verify: NVIDIA Linux driver documentation or source on how I²C ports are exposed.

## Windows (NvAPI) vs Linux (nvidia.ko) I²C path

Installed driver: NVIDIA open kernel module 615.71.09 (`/proc/driver/nvidia/version`).
Source: NVIDIA/open-gpu-kernel-modules tag `615.71.09` (commit `61dcc93`).

Verified from that source:
- The number in "NVIDIA i2c adapter N" is the RM port number (`kernel-open/nvidia/nv-i2c.c:252`).
- Regular adapters are created for RM I²C ports reported as both IMPLEMENTED and VALID by
  `NV402C_CTRL_CMD_I2C_GET_PORT_INFO` (`src/nvidia/arch/nvalloc/unix/src/osapi.c`,
  `RmI2cAddGpuPorts`). Further adapters are created for on-chip DisplayPort outputs, using
  `ddcPortId - 1`.
- An adapter tied to a display (displayId ≠ 0) sends transfers over DP AUX
  (`RmDpAuxI2CTransfer`) and accepts only plain READ/WRITE; SMBus commands return
  `NV_ERR_NOT_SUPPORTED` (`osapi.c` around line 4230).
- `i2ctransfer` (master_xfer) sends every message as a separate RM transfer, with no
  repeated start (`nv-i2c.c:55–73`). A `w1 … r1` pair is therefore two transactions on
  Linux, unlike NvAPI's combined format on Windows.
- `i2cget` uses smbus_xfer, which maps to RM SMBus commands (`nv-i2c.c:132–181`).
- The handler of `NV402C_CTRL_CMD_I2C_GET_PORT_INFO` is not in the open source tree; the
  open code only calls it.
- nvidia-drm exposes no `ddc` link for its connectors in sysfs, and `/proc/driver/nvidia`
  has no adapter-to-connector map.

> **⚠️ UNVERIFIED:** whether NvAPI `portId` 1 is RM port 1 (exposed as i2c-8) or RM port 0
> (not exposed on this system). Neither nvapi.h nor the kernel source defines the base.
> To verify: an official NVIDIA statement, or observing NvAPI traffic on Windows.

> **⚠️ UNVERIFIED:** why RM ports 0 and 2 are not exposed (not IMPLEMENTED or not VALID).
> To verify: the decision is made outside the open source tree; needs NVIDIA documentation.

> **⚠️ ASSUMPTION:** i2c-11 is a DP-AUX adapter.
> Based on: on i2c-11 every SMBus command fails while raw read/write report success, which
> matches the DP-AUX code path above. Not confirmed by any driver output.

## I²C speed: Windows 10 kHz vs Linux 100 kHz

Verified:
- FireStorm passes speed value 2 = `NVAPI_I2C_SPEED_10KHZ` (enum in NVIDIA/nvapi `nvapi.h`)
  for the 0x4A identification read.
- On Linux every `/dev/i2c-*` transfer goes through `RmNonDPAuxI2CTransfer`, which zeroes
  the params and never sets `flags` (`osapi.c:4017–4150`, driver 615.71.09). Flags 0 is
  `NV402C_CTRL_I2C_FLAGS_SPEED_MODE_100KHZ` (`ctrl402c.h:151–159`). So all Linux NVIDIA
  i2c-dev transfers run at 100 kHz, and userspace cannot change that.

Community sources (OpenRGB GitLab; claims, not vendor documentation):
- Issue #5641 "[New Device] ZOTAC GAMING GeForce RTX 5080 SOLID CORE" reports
  `SUBSYS_176519DA`, the same subsystem as this card. Only a screenshot, no captures.
- MR !3380 (open, not merged; author Doodcom; 2026-07-13) states that the ZOTAC SPECTRA
  MCU at 0x4A only answers at 10 kHz, and adds a Linux backend that issues
  `NV402C_CTRL_CMD_I2C_TRANSACTION` through `/dev/nvidiactl` with a speed flag on RM port 1.
  Tested by the author on an RTX 4070 Ti SUPER Trinity (controller ID `N702E-1002`).
- The 0x4A protocol in MR !2625 / !3380: detection is an I2C block read of register 0x5A,
  32 bytes, at 10 kHz; known IDs include `N764E-1001` (RTX 5070 SOLID, subsystem 0x1764).

ABI check of MR !3380 `nvrm_api.h` against driver 615.71.09 official headers: all
constants match, and 26 sizes/offsets (NVOS21/54/00, NV0080/2080 alloc params,
NV402C transaction params, I2C_BUFFER_RW, version handshake) are identical. A negative
control (one field changed) was detected.

> **⚠️ UNVERIFIED:** that NvAPI port 1 on Windows is RM port 1. MR !3380 states it in a code
> comment and reports a working test on another card; no NVIDIA documentation.
> To verify: a successful 10 kHz identification read on RM port 1 of this card.

> **⚠️ ASSUMPTION:** this card has the 0x4A SPECTRA controller with ID prefix `N765E`.
> Based on: FireStorm compares 0x4A IDs with `N765E`; the ZOTAC brochure for the sibling
> SKU mentions `N765`; the RTX 5070 SOLID (0x1764) uses the 0x4A protocol per MR !3380.

## 2026-10-10 — RM API probe at 10 kHz (`tools/nvrm-i2c-probe`, controlled test)

Pre-run: OpenRGB server held no NVIDIA device; driver 615.71.09; GPU 48 °C, 0 % fan,
20.9 W, P8. All three transactions were reads on RM port 1 (`bWrite = NV_FALSE`).

| # | Transaction | RM status |
|---|---|---|
| A | 0x4A, register 0x5A, 32 bytes, 100 kHz | `0x14` `NV_ERR_I2C_ERROR` |
| B | plain read, 0x30, 1 byte, 10 kHz (negative control) | `0xFFFF` `NV_ERR_GENERIC` |
| C | 0x4A, register 0x5A, 32 bytes, 10 kHz | `NV_OK` |

C returned:
`4E 37 36 35 45 2D 31 30 30 31 01 01 11 FF 28 14 0A 08 00 …` (rest `00`),
ASCII `N765E-1001`. Status codes from `nvstatuscodes.h`. After: 48 °C, 0 %, 21.1 W, P8.

Verified by this run:
- The method separates absent from present devices: B and A fail, C succeeds.
- The controller at 0x4A on RM port 1 answers at 10 kHz and not at 100 kHz.
- Its ID is `N765E-1001`, one of the IDs FireStorm's identification function compares
  (`N765E`), so FireStorm's 0x4A path applies to this card.
- The controller is reachable on RM port 1 (Linux shows that port as i2c-8, which only
  runs at 100 kHz).

> **⚠️ UNVERIFIED:** that NvAPI port 1 (FireStorm) and RM port 1 are the same port. This run
> shows where the controller is on Linux, not how NvAPI numbers ports.
> To verify: an NVIDIA document, or a Windows NvAPI trace. Not needed for Linux control.

Bytes 10–17 (`01 01 11 FF 28 14 0A 08`), decoded with OpenRGB MR !3380's status layout:
on = 1, mode = 0x01 (breathe), RGB = 11 FF 28 (green), on-time 0x14, off-time 0x0A,
preset 0x08. A controlled observation showed green breathing output, which matches on/off, mode and colour.

> **⚠️ UNVERIFIED:** the meaning of on-time 0x14, off-time 0x0A and preset 0x08.
> To verify: FireStorm's code that builds the 0x4A packet.

## 2026-10-10 — FireStorm 0x4A call sites (re-run of the decoder with speed column)

`tools/analysis/firestorm_i2c_calls.py` (63 call sites, identical to the first run):

| Function | Kind | Register | Length | Speed |
|---|---|---|---|---|
| `0x41bba0` | read | 0x5A | 32 | 2 (10 kHz) |
| `0x427a90` | read | 0x5A | 32 | 2 (10 kHz) |
| `0x430880` | write | 0x5A | 32 | 2 (10 kHz) |

The 0x4B identification read (`0x41bba0`) uses speed 4 (100 kHz).

> **⚠️ UNVERIFIED:** the 32-byte write layout. OpenRGB MR !3380 sends 10 bytes.
> To verify: decode `0x430880` before any write.

## 2026-10-10 — FireStorm 0x4A write packet (static analysis, no hardware access)

Function `0x430880` (this = lighting object, two stack args) builds a 32-byte buffer and
writes it to 0x4A, register 0x5A, regAddrSize 1, speed 2 (10 kHz). Stack offsets traced
instruction by instruction:

| Byte | Value |
|---|---|
| 0 | arg 1 |
| 1 | arg 2 |
| 2 | mode, from `[obj+0x1df]`; UI mode 4 becomes 4 if `[obj+0x1bc] == 0xb`, else 5 |
| 3, 4, 5 | `[obj+0x1e0]`, `[obj+0x1e1]`, `[obj+0x1e2]` |
| 6 | `0x14` for modes 1 and 2; `0x2d` for modes 0, 3, 4, 5 |
| 7 | `0x0a` for modes 0–5 |
| 8 | `[obj+0x1e8]` |
| 9–31 | `0x00` |

Callers: `0x430867` passes (0, 1); `0x4331cb` passes (1, 1); `0x428d6d` passes (0, variable).

`[obj+0x1e8]` is set to 0–5 by six handlers that each also pass the same number to
`0x4080b0`, and to 0x8 by a separate handler at `0x4332d0`.

Consistency check with the status read on the same day: the controller reported
`01 01 11 FF 28 14 0A 08` = byte 1 … byte 8 of this packet (on 1, mode 1, RGB, 0x14, 0x0A,
0x08). Mode 1 → byte 6 = 0x14 and byte 7 = 0x0A matches FireStorm's rule.

Comparison with OpenRGB MR !3380: same field order for bytes 0–8 (reset, on, mode, R, G,
B, on-time, off-time, preset); MR sends 10 bytes, FireStorm 32 with a zero tail.

> **⚠️ ASSUMPTION:** byte 0 = reset, byte 1 = on, colours = R, G, B in that order.
> Based on: the (1, 1) caller sits in a reset path, MR !3380's names, and the status read
> (`11 FF 28` while the physical observation showed green; G = 0xFF is consistent with R, G, B order).

> **⚠️ ASSUMPTION:** preset 0x8 = custom colour, 0–5 = built-in presets.
> Based on: MR !3380's comment and FireStorm setting 0x8 in a separate handler from the
> six preset handlers. Exact UI meaning not traced.

> **⚠️ UNVERIFIED:** every mode number except 1 (breathe, confirmed by controlled
> observation). MR !3380 lists 0 static, 2 fade, 3 wink, 4 spectrum breathe, 5 random.
> To verify: a write under controlled observation.

## 2026-10-10 — first write test (`tools/spectra-write-test`, controlled test and watching)

All transactions: RM port 1, 0x4A, register 0x5A, `I2C_BUFFER_RW`, 32 bytes, 10 kHz.
The write uses the same transaction type as the driver's own i2c block write
(`osapi.c:4109–4117`), plus the 10 kHz flag.

| Step | Bytes | RM status | Read back (bytes 10–17) | Physical observation |
|---|---|---|---|---|
| status | — | `NV_OK` | `01 01 11 FF 28 14 0A 08` | (green, breathing; confirmed earlier) |
| test | `00 01 00 00 00 FF 2D 0A 08` + 23 × `00` | `NV_OK` | `01 00 00 00 FF 2D 0A 08` (match) | **blue, steady** |
| restore | `00 01 01 11 FF 28 14 0A 08` + 23 × `00` | `NV_OK` | `01 01 11 FF 28 14 0A 08` (match) | **green, breathing** |

GPU state before and after: 47 °C, 0 % fan, ~20.6–21.0 W, P8.

Verified by this test:
- The GPU LEDs can be controlled from Linux through the RM API at 10 kHz.
- Packet layout `reset, on, mode, R, G, B, on-time, off-time, preset, 0 × 23` works.
- Byte order R, G, B: `00 00 FF` gave blue.
- Mode 0 = static (controlled observation: steady blue); mode 1 = breathe (controlled observation: breathing green).
- The status read returns bytes 1–8 of the last packet, so a write can be checked by
  reading back.
- Restoring the earlier packet returns the LEDs to the earlier look.

> **⚠️ UNVERIFIED:** modes 2–5, the effect of byte 0 (reset = 1), and the meaning of
> presets 0–5. To verify: further tests under controlled observation.

## 2026-10-10 — effect mode write test (controlled test)

The controller was checked before every write and reported `N765E-1001`. Each packet was
32 bytes: the nine bytes below followed by 23 zero bytes. Every write returned `NV_OK`,
the status read-back matched bytes 1–8 of the packet, and the verified restore packet was
sent after each mode.

| Candidate mode | Packet bytes 0–8 | Read-back | Restore |
|---|---|---|---|
| fade (2) | `00 01 02 00 00 FF 14 0A 08` | match | `NV_OK`, match |
| wink (3) | `00 01 03 00 00 FF 2D 0A 08` | match | `NV_OK`, match |
| spectrum (4) | `00 01 04 00 00 FF 2D 0A 08` | match | `NV_OK`, match |
| random (5) | `00 01 05 00 00 FF 2D 0A 08` | match | `NV_OK`, match |

This verifies that modes 2–5 are accepted by the controller and persist through status
read-back. The visual effect names remain dependent on controlled visual observation. The final
GPU state was restored to the previously verified green breathing packet.

## 2026-10-10 — first CoolerControl plugin write

CoolerControl reported two GPU devices without model names. The NVIDIA device exposed
the sensor `GPU Temp` at 50.0 °C; the AMD device exposed `temp1`. The plugin therefore
selects the unique GPU carrying the configured `GPU Temp` sensor and does not fall back to
the first GPU.

The plugin selected the NVIDIA RTX 5080 device, calculated `#00D377`
from the GPU scale, and wrote static mode through the RM controller. A read-only status
after the run reported `01 00 00 D3 77 2D 0A 08`, confirming mode 0 and RGB `00 D3 77`.
