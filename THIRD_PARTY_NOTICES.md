# Third-party notices

## NVIDIA open GPU kernel modules

The controller and probe compile against headers from NVIDIA's
[open-gpu-kernel-modules](https://github.com/NVIDIA/open-gpu-kernel-modules) project.
The build must use the checkout matching the installed NVIDIA driver version. The project
license and notices are available in that upstream repository. This repository does not copy
the upstream source tree.

## OpenRGB protocol research

Protocol and packet-layout research references OpenRGB issues and merge requests as documented in
`docs/research-report.md`. OpenRGB is not a runtime dependency of this plugin, and no OpenRGB
source files are included here.
