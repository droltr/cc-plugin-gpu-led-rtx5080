# Session record

## 2026-10-10 — Claude research and plugin handoff

The archived Claude sessions for this project were inspected before updating this record.
The session established the following user requirements and verified findings:

- The target is the owner's ZOTAC GeForce RTX 5080 SOLID CORE OC, SKU `ZT-B50800J2-10P`.
- The GPU LED controller was identified at address `0x4A` on NVIDIA RM port 1. It answers at
  10 kHz through the NVIDIA RM path exposed by `/dev/nvidiactl`; the Linux i2c-dev path uses
  100 kHz and did not reach this controller.
- The controller identification read returned `N765E-1001`. The 32-byte packet layout and the
  first static write were read back successfully. The owner observed static blue output, then
  observed the verified restore packet as breathing green.
- The owner chose a separate CoolerControl GPU LED plugin. The GPU plugin must not be mixed with
  `cc-plugin-thermal-rgb` or `coolerdash` ownership.
- The initial plugin scope was GPU temperature input plus the thermal-rgb colour sequence. The
  settings page was deferred until that core path was tested.

## Corrections retained as project rules

The research record includes withdrawn findings caused by missing negative controls, treating
tool success as device acceptance, and presenting inferences as facts. Those findings remain
labelled in `docs/probe-log.md` and `docs/research-report.md`. No labelled assumption is a basis
for a hardware write.

The current UI is experimental. It is kept on `test/ui-settings`; its save operation has not
been accepted as working. The stable scope is the temperature-driven LED runtime and its
configuration parser. Tests and commits are deferred until the source separation and evidence
review are complete.

## Current deployed state

The installed service is configured for static LED output with the GPU temperature colour
source and a 75% software RGB limit. This is a runtime observation from the active service and
does not certify the deferred UI.

## Open work

1. Keep the settings page isolated on `test/ui-settings`.
2. Keep the LED/temperature runtime separate from configuration and UI code.
3. Verify the exact NVIDIA open-gpu-kernel-modules source and driver headers before rebuilding
   the controller. Rebuilding is required only when the C controller source changes or the
   installed driver ABI/headers require a new binary; it is not required for a configuration-
   only change.
4. Run the repository tests and hardware checks only after the source and records are reviewed.
5. Commit only after those checks produce reproducible evidence.
