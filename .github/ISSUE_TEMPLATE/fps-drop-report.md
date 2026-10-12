---
name: FPS drop report
about: Report your result of the FPS drop test protocol
title: "FPS drop report: <GPU> / <game>"
labels: ""
---

Protocol: [docs/FPS-DROP-TESTING.md](../blob/main/docs/FPS-DROP-TESTING.md)

## System
- GPU and board:
- Driver version:
- Kernel and distribution:
- Game, scene, resolution, refresh rate:

## Results (average FPS / 1 % low FPS / largest frame time)
| Run | Plugin | Avg FPS | 1 % low | Max frame time |
|---|---|---|---|---|
| A1 | disabled | | | |
| B1 | enabled | | | |
| B2 | enabled | | | |
| A2 | disabled | | | |

## Questions
- Did the drops stop in the disabled runs and return in the enabled runs?
- Do frame-time spikes line up with `spectra-controller` calls?
- Anything that differs from the original report?
