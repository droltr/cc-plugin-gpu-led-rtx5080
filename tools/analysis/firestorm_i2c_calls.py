"""List the I2C helper call sites in a FireStorm.exe disassembly.

Input: `objdump -d -M intel --no-show-raw-insn FireStorm.exe` output (FireStorm
5.0.0.015E). The two helpers wrap NvAPI_I2CRead / NvAPI_I2CWrite:

    0x420fe0 read, 0x421090 write
    register args: dl = 8-bit device address, ecx = GPU index
    stack args (first pushed last): reg value, regAddrSize, buffer, length, speed

The register value is passed by address and stored in pbI2cRegAddress, so it is the
register index. Speed is the NV_I2C_SPEED enum (2 = 10 kHz).

Usage: python3 -I firestorm_i2c_calls.py firestorm.asm
"""
import re
import sys

HELPERS = {"0fe0": "READ ", "1090": "WRITE"}


def main(path):
    lines = open(path).read().splitlines()
    func = None
    for i, line in enumerate(lines):
        if re.search(r"\tint3$", line) and i + 1 < len(lines) and "int3" not in lines[i + 1]:
            func = lines[i + 1].split(":")[0].strip()
        m = re.search(r"call +0x42(0fe0|1090)$", line)
        if not m:
            continue
        pushes, dl, imm = [], None, {}
        for j in range(i - 1, max(i - 40, 0), -1):
            text = lines[j].split("\t", 1)[1] if "\t" in lines[j] else ""
            if text.startswith("call"):
                break
            mm = re.match(r"push +(\S+)$", text)
            if mm and len(pushes) < 5:
                pushes.append(mm.group(1))
            mm = re.match(r"mov +dl,(0x[0-9a-f]+)$", text)
            if mm and dl is None:
                dl = int(mm.group(1), 16)
            mm = re.match(r"mov +BYTE PTR \[esp\+(0x[0-9a-f]+)\],(0x[0-9a-f]+)$", text)
            if mm:
                imm.setdefault(int(mm.group(1), 16), int(mm.group(2), 16))
        # The nearest push is the first argument.
        reg, regsize, buf, length, speed = (pushes + ["?"] * 5)[:5]
        addr = f"0x{dl >> 1:02X}" if dl else "?"
        data = " ".join(f"{v:02X}" for _, v in sorted(imm.items()))
        print(f"{func} {line.split(':')[0].strip()} {HELPERS[m.group(1)]} addr={addr} "
              f"reg={reg} regsize={regsize} len={length} speed={speed} buf={buf} imm=[{data}]")


if __name__ == "__main__":
    main(sys.argv[1])
