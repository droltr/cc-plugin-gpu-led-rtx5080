# Build against the official headers of the installed NVIDIA driver:
#   git clone --depth 1 --branch <driver-version> \
#       https://github.com/NVIDIA/open-gpu-kernel-modules.git <dir>
#   make NVIDIA_SRC=<dir>
NVIDIA_SRC ?= $(error set NVIDIA_SRC to an open-gpu-kernel-modules checkout)
CFLAGS += -O2 -Wall -Wextra -Werror \
	-I$(NVIDIA_SRC)/src/common/sdk/nvidia/inc \
	-I$(NVIDIA_SRC)/kernel-open/common/inc \
	-I$(NVIDIA_SRC)/src/nvidia/arch/nvalloc/unix/include

nvrm-i2c-probe: nvrm-i2c-probe.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f nvrm-i2c-probe
