# gpu-rgb-rtx5080

**Status: LED control verified from Linux (first write test passed, 2026-10-10).**
Summary: [docs/research-report.md](docs/research-report.md) · step log:
[docs/probe-log.md](docs/probe-log.md) · plan: [docs/PLAN.md](docs/PLAN.md).

Control the LEDs of a **ZOTAC GeForce RTX 5080** from Linux, so the card
can join the temperature and game lighting.

## Hardware

Unlabelled statements are verified and cite their source. Unverified statements and
assumptions are marked `⚠️ UNVERIFIED` / `⚠️ ASSUMPTION`.

- PCI `10de:2c02`, subsystem `19da:1765` (source: sysfs).
- NVIDIA I²C adapters `/dev/i2c-8` … `/dev/i2c-12`, named "NVIDIA i2c adapter 1, 3, 4, 5, 6
  at 1:00.0" (source: `/sys/class/i2c-dev/*/name`).
- Retail model **RTX 5080 SOLID CORE OC**, SKU `ZT-B50800J2-10P`, "SPECTRA RGB lighting",
  "ZOTAC GAMING FireStorm Utility" (source: ZOTAC brochure, see
  [docs/probe-log.md](docs/probe-log.md)).

## Findings so far

- OpenRGB tags `release_1.0` and `master` (2026-09-11) contain
  `Controllers/ZotacBlackwellGPUController`, first committed 2026-04-09 by Eder Sánchez
  (`b3a50d66`). Its detector is registered for: RTX 5080 AMP Extreme INFINITY (subsystem
  `0x1762`), RTX 5090 SOLID OC (`0x1761`), RTX 5090 D SOLID OC (`0x2761`), all at I²C
  address `0x4B` (source: local OpenRGB clone, `ZotacBlackwellGPUControllerDetect.cpp`,
  `pci_ids.h`, `git log`).
- The target card (subsystem `0x1765`) is not in that list. Its LED controller is a
  ZOTAC SPECTRA MCU at **0x4A on NVIDIA RM port 1**, ID **`N765E-1001`**, which only answers
  at **10 kHz**. Linux `/dev/i2c-*` NVIDIA adapters run at a fixed 100 kHz, so it is
  reached through `/dev/nvidiactl` instead (`tools/nvrm-i2c-probe`, read-only). A write
  test (`tools/spectra-write-test`) set steady blue and restored breathing green.
  Source: probe on 2026-10-10; details in [docs/research-report.md](docs/research-report.md).
- Tag `release_candidate_1.0rc2` has only `ZotacTuringGPUController` and
  `ZotacV2GPUController`, no Blackwell controller (source: `git ls-tree`).

## Initial CoolerControl plugin

The stable runtime is the `gpu_led` package without the optional settings page. It reads the
GPU temperature from CoolerControl, uses the same colour sequence as `cc-plugin-thermal-rgb`,
and invokes the RM-backed controller only when the colour changes. Static LED output is enabled
in the checked-in `config.json`.

The checked-in `brightness_pct` is `75`. For this exact 0x4A controller, the
available Linux protocol evidence does not establish a hardware brightness field
for Static mode: OpenRGB maps the timing byte to brightness only for Wink/Random,
while Static uses the byte as its fixed timing value. The plugin therefore limits
the RGB command channels to 75% while preserving continuous Static output. This is
a software output-level limit, not a claim that the LED current or electrical power
is exactly 75%.

### Effect settings

The plugin UI exposes the verified 0x4A ranges in this order:

| Effect | Mode | Colour source | Speed | Hardware effect brightness |
|---|---:|---|---:|---:|
| Breathe | 1 | selected / temperature | 0–255 | not exposed |
| Wink | 3 | selected / temperature | 0–255 | 0–255 |
| Static | 0 | selected / temperature | none | not exposed |
| Fade | 2 | controller colour cycle | 0–255 | not exposed |
| Spectrum breathe | 4 | controller colour cycle | 0–255 | not exposed |
| Random | 5 | controller random effect | 0–255 | 0–255 |

The separate software brightness control is `0–100%` and scales the RGB command.
The ranges and mode capabilities follow the OpenRGB 0x4A driver analysis; the
controller does not expose millisecond durations or a measured optical brightness
percentage. The UI's random source uses host-generated colours for colour-capable
modes and the controller's own random/cycle effect for the other modes.

The settings page is experimental and remains on the `test/ui-settings` branch. Its save path
has not been verified and it is not part of the stable runtime acceptance criteria. The stable
runtime and its settings contract are kept separate: `gpu_led/` contains the temperature and
LED controller path, `config.json` and `gpu_led/config.py` contain configuration parsing, and
`ui/index.html` is the deferred test interface.

The GPU stop positions are `40/56/72/88 °C`. NVIDIA lists `88 °C` as the RTX 5080's
maximum GPU temperature; 40 °C is the practical lower endpoint used for the colour
scale, based on NVIDIA's typical cool/idle GPU range. Sources: [NVIDIA RTX 5080
specifications](https://www.nvidia.com/en-us/geforce/graphics-cards/50-series/rtx-5080/)
and [NVIDIA temperature guidance](https://nvidia.custhelp.com/app/answers/detail/a_id/2343/).
Temperature samples use a 3-second EMA and the controller is updated only after a
meaningful two-channel RGB change, matching the thermal-rgb update behavior.

Build the controller against the exact installed NVIDIA driver headers:

```sh
make -C tools/spectra-write-test NVIDIA_SRC=/path/to/open-gpu-kernel-modules spectra-controller
```

The controller source is `tools/spectra-write-test/spectra-write-test.c`. Its headers come from
NVIDIA's official `open-gpu-kernel-modules` source, recorded for the installed 615.71.09 driver
as tag `615.71.09`, commit `61dcc93`, in `docs/research-report.md`. The binary uses NVIDIA RM
ioctl structures and flags, so the exact header/driver pair matters. A rebuild is required when
the C controller source changes or the installed driver ABI/header revision changes; a
configuration-only change or a Python runtime refactor does not by itself require a rebuild.
Before any rebuild, verify the checkout, tag/commit, compiler, generated binary, strict driver
version check, and read-only controller status path. A wrong header/ABI combination could make
the controller fail or send an unintended RM transaction, so no rebuilt binary is accepted from
an unverified source tree.

The plugin launcher expects that binary as `spectra-controller` beside `run.sh`, with
`driver_version` set to the installed driver version before enabling writes.

Install the complete plugin with the matching header checkout:

```sh
sudo SPECTRA_NVIDIA_SRC=/path/to/open-gpu-kernel-modules bash install.sh
sudo systemctl restart coolercontrold
```

The stable install omits the deferred settings page. The experimental UI branch can include it
only when explicitly requested with `INSTALL_UI=1`; that branch is not a stable acceptance
target.

The plugin selects the GPU by the configured `gpu_name` (`NVIDIA GeForce RTX 5080`),
and requires the selected device to expose the `GPU Temp` sensor. On the current
CoolerControl status, the NVIDIA device exposes `GPU Temp` and the AMD device exposes
`temp1`, so the AMD GPU is ignored.

## Repository layout

| Path | Content |
|---|---|
| `docs/research-report.md` | consolidated findings, environment, sources, mistakes |
| `docs/probe-log.md` | chronological log of every probe |
| `tools/nvrm-i2c-probe/` | read-only 10 kHz identification probe (RM API) |
| `tools/spectra-write-test/` | guarded controller status/write utility |
| `tools/analysis/` | helpers used for FireStorm and ABI analysis (no hardware access) |
| `docs/DEPLOYMENT.md` | installation, upgrade, rollback, and runtime verification |
| `docs/TESTING.md` | automated and hardware validation boundaries |
| `THIRD_PARTY_NOTICES.md` | dependency and source attributions |
