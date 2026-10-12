# FPS drop: test protocol, fix steps and how to report

Tracked in [issue #4](https://github.com/droltr/cc-plugin-gpu-led-rtx5080/issues/4).

## Status

- Verified (source: the original reporter's repeated in-game A/B): on an RTX 5080 with driver
  615.71.09, enabling the plugin causes periodic FPS drops in Counter-Strike 2 (reported as
  about 599 -> 400 fps) and disabling it removes them.
- Verified (source: measurement with a probe loop and a negative control, see issue #4): one
  `spectra-controller set` call performs 3 I²C transactions of 32 bytes at 10 kHz, and each
  transaction blocks other NVIDIA RM calls for about 32 ms.

> **⚠️ UNVERIFIED:** That the ~32 ms block is the mechanism behind the drops.
> To verify: the frame-time test below, with spikes aligned to the controller calls.

> **⚠️ UNVERIFIED:** Whether other GPUs, drivers, games or boards are affected. This guide
> exists so that other users can test and report their own results.

## Test protocol (A/B with frame times)

Goal: compare frame times with the plugin disabled and enabled, in the same game scene.

Requirements: MangoHud, a game that is reproducible (for example a replay or a fixed map
walk), and the plugin installed as described in the README. Do not change any other setting
between runs.

1. Disable the plugin in CoolerControl (plugin settings) and confirm no
   `spectra-controller` process is running: `pgrep -a spectra-controller` prints nothing.
2. Enable frame-time logging in MangoHud. The official `MangoHud.conf` documents these keys
   (source: MangoHud repository, `data/MangoHud.conf`): `log_interval=0` (log every frame),
   `output_folder=<directory>`, `toggle_logging=Shift_L+F2`, `autostart_log`, `log_duration`.
   Example Steam launch option: `MANGOHUD_CONFIG=log_interval=0,output_folder=$HOME/mangologs,toggle_logging=Shift_L+F2 mangohud %command%`

   > **⚠️ UNVERIFIED:** That this exact launch option string produces a frame-time CSV for the
   > game. To verify: run it once and check that a CSV file appears in the output folder.

3. Run A (plugin disabled): play the same scene for at least 5 minutes with logging on.
4. Enable the plugin. Run B (plugin enabled): repeat the same scene for the same duration.
5. Repeat A and B once more (order B, A) to rule out drift such as thermals or shader caching.
6. While run B is active, record when the controller runs. Every start of the plugin's helper
   shows up as a `spectra-controller set ...` process; a one-second sampler is enough:
   `while true; do date +%T.%N; pgrep -a spectra-controller; sleep 1; done > controller-calls.txt`

   > **⚠️ UNVERIFIED:** That a one-second sampler catches every call (a call lasts under one
   > second). To verify: compare the number of sampled calls with the plugin's own log.

Negative control: run A must show the same spike pattern whether or not the sampler runs.
If run A also shows the drops, the cause is not the plugin on that system, so say so in the report.

## What to report

Please open an issue using the "FPS drop report" template and include:

- GPU model and board, driver version, kernel, distribution.
- Game and scene, resolution and refresh rate.
- For each run (A, B, A, B): average FPS, 1 % low FPS, and the largest frame time.
- Whether the drops stopped in run A and returned in run B.
- Whether frame-time spikes line up with the `spectra-controller` calls.

Do not include personal paths, hostnames or serial numbers.

## Fix steps (not implemented)

Each step that writes to hardware needs the maintainer's approval before it runs.

1. Gate colour updates on GPU load: read GPU utilisation through NVML (a cheap call that does
   not reach the RM lock), keep the wanted colour in memory while a game is rendering, and apply
   it when the GPU is idle again, with hysteresis so the colour does not flap.
2. Remove the per-call status read (check identity once at start) and consider dropping the
   read-back. This cuts 3 blocked windows per colour change to 1.
3. Raise the colour-change threshold (currently 2 per channel) so fewer updates happen.
4. Keep one long-lived RM client instead of allocating one per update.
5. Verify the fix with the same A/B protocol: run B with the fix must match run A.
