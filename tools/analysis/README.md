# Analysis helpers

Small programs used during the research. None of them touches hardware.

| File | What it does | Build / run |
|---|---|---|
| `firestorm_i2c_calls.py` | lists the I²C helper call sites in a FireStorm 5.0.0.015E disassembly | `objdump -d -M intel --no-show-raw-insn FireStorm.exe > fs.asm`; `python3 -I firestorm_i2c_calls.py fs.asm` |
| `nv-i2c-info-v3-layout-check.c` | checks the i386 offsets of `NV_I2C_INFO_V3` (copied field list from NVIDIA `nvapi.h`) with `_Static_assert` | `gcc -m32 -fsyntax-only nv-i2c-info-v3-layout-check.c` |
| `nv402c-speed-flags.c` | prints `NV402C_CTRL_I2C_FLAGS_SPEED_MODE` values from the official macros | `gcc -I<ogkm>/src/common/sdk/nvidia/inc nv402c-speed-flags.c && ./a.out` |
| `abi-compare/` | prints sizes/offsets of the RM ABI structs from the official headers (`official.c`) and from OpenRGB MR !3380's `nvrm_api.h` (`mr.cpp`) for `diff` | see below |

`abi-compare`: `nvrm_api.h` (GPL-2.0-only) is not stored here. Fetch it from MR !3380 into
this folder, then:

```sh
gcc -I<ogkm>/src/common/sdk/nvidia/inc -I<ogkm>/kernel-open/common/inc -o official official.c
g++ -o mr mr.cpp
diff <(./official) <(./mr)
```

FireStorm itself (proprietary) and its disassembly are not stored in the repository.
