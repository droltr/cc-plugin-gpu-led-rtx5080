/*
 * nvrm-i2c-probe: read-only I2C identification probe for the ZOTAC RTX 5080
 * SOLID CORE OC (10de:2c02, subsystem 19da:1765) through the NVIDIA Resource
 * Manager API (/dev/nvidiactl).
 *
 * Why: Linux /dev/i2c-* adapters of the NVIDIA driver always use 100 kHz
 * (RmNonDPAuxI2CTransfer leaves flags = 0 = SPEED_MODE_100KHZ). ZOTAC FireStorm
 * reads the 0x4A controller at 10 kHz (NVAPI_I2C_SPEED_10KHZ).
 *
 * Safety:
 *   - built only against the official headers of the installed driver
 *     (NVIDIA/open-gpu-kernel-modules, same tag);
 *   - refuses to run unless the GPU is exactly 10de:2c02 / 19da:1765, minor 0;
 *   - performs a fixed list of three READ transactions; bWrite is always NV_FALSE,
 *     and there are no command-line options for address, register or data;
 *   - the only bytes put on the bus besides addresses are the register index
 *     0x5A of the identification read, as FireStorm does.
 *
 * Builds against NVIDIA open-gpu-kernel-modules headers (MIT). The approach of
 * using NV402C_CTRL_CMD_I2C_TRANSACTION with a speed flag follows OpenRGB MR !3380;
 * no OpenRGB code is included. See THIRD_PARTY_NOTICES.md.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "nvtypes.h"
#include "nvmisc.h"
#include "nvos.h"
#include "class/cl0080.h"
#include "class/cl2080.h"
#include "class/cl402c.h"
#include "ctrl/ctrl402c.h"
#include "nv-ioctl.h"
#include "nv-ioctl-numbers.h"
#include "nv_escape.h"

#define PCI_DIR      "/sys/bus/pci/devices/0000:01:00.0"
#define GPUS_DIR     "/proc/driver/nvidia/gpus"
#define EXPECT_DEV   0x2c02
#define EXPECT_SUBV  0x19da
#define EXPECT_SUBD  0x1765

#define H_DEVICE 0xA5080001u
#define H_SUBDEV 0xA5080002u
#define H_I2C    0xA5080003u

#define RM_PORT  1

static int ctl_fd = -1;
static NvHandle h_client;

static int nv_ioctl(int fd, unsigned nr, void *arg, size_t size)
{
    return ioctl(fd, _IOC(_IOC_READ | _IOC_WRITE, NV_IOCTL_MAGIC, nr, size), arg);
}

static unsigned read_hex(const char *name)
{
    char path[256], buf[32] = "";
    snprintf(path, sizeof(path), "%s/%s", PCI_DIR, name);
    FILE *f = fopen(path, "r");
    if (!f || !fgets(buf, sizeof(buf), f)) {
        if (f)
            fclose(f);
        return 0;
    }
    fclose(f);
    return (unsigned)strtoul(buf, NULL, 16);
}

/* RM device instance 0 and /dev/nvidia0 must be this card: exactly one NVIDIA GPU,
 * 0000:01:00.0, reporting "Device Minor: 0". */
static int check_single_gpu_minor0(void)
{
    DIR *d = opendir(GPUS_DIR);
    struct dirent *e;
    int count = 0, minor = -1;
    char line[128];
    FILE *f;

    if (!d) {
        perror(GPUS_DIR);
        return -1;
    }
    while ((e = readdir(d)) != NULL)
        if (e->d_name[0] != '.')
            count++;
    closedir(d);

    f = fopen(GPUS_DIR "/0000:01:00.0/information", "r");
    while (f && fgets(line, sizeof(line), f))
        sscanf(line, "Device Minor: %d", &minor);
    if (f)
        fclose(f);

    printf("nvidia gpus: %d, device minor: %d\n", count, minor);
    if (count != 1 || minor != 0) {
        fprintf(stderr, "refusing: expected exactly one GPU with minor 0\n");
        return -1;
    }
    return 0;
}

static int check_card(void)
{
    unsigned dev = read_hex("device"), subv = read_hex("subsystem_vendor"),
             subd = read_hex("subsystem_device");
    printf("card: %04x subsystem %04x:%04x\n", dev, subv, subd);
    if (dev != EXPECT_DEV || subv != EXPECT_SUBV || subd != EXPECT_SUBD) {
        fprintf(stderr, "refusing: not the expected card\n");
        return -1;
    }
    return check_single_gpu_minor0();
}

static NvU32 rm_alloc(NvHandle parent, NvHandle hnew, NvU32 cls, void *parms, NvU32 size,
                      NvHandle *out)
{
    NVOS21_PARAMETERS p;
    memset(&p, 0, sizeof(p));
    p.hRoot = h_client;
    p.hObjectParent = parent;
    p.hObjectNew = hnew;
    p.hClass = cls;
    p.pAllocParms = NV_PTR_TO_NvP64(parms);
    p.paramsSize = size;
    if (nv_ioctl(ctl_fd, NV_ESC_RM_ALLOC, &p, sizeof(p)) < 0)
        return NV_ERR_GENERIC;
    if (p.status == NV_OK && out)
        *out = p.hObjectNew;
    return p.status;
}

static int open_rm(const char *version)
{
    nv_ioctl_rm_api_version_t v;
    NV0080_ALLOC_PARAMETERS dev;
    NV2080_ALLOC_PARAMETERS sub;
    NvU32 st;

    ctl_fd = open("/dev/nvidiactl", O_RDWR);
    if (ctl_fd < 0 || open("/dev/nvidia0", O_RDWR) < 0) {
        perror("open");
        return -1;
    }

    memset(&v, 0, sizeof(v));
    v.cmd = NV_RM_API_VERSION_CMD_STRICT;
    snprintf(v.versionString, sizeof(v.versionString), "%s", version);
    if (nv_ioctl(ctl_fd, NV_ESC_CHECK_VERSION_STR, &v, sizeof(v)) < 0 ||
        v.reply != NV_RM_API_VERSION_REPLY_RECOGNIZED) {
        fprintf(stderr, "refusing: driver version is not %s\n", version);
        return -1;
    }

    if ((st = rm_alloc(0, 0, NV01_ROOT_CLIENT, NULL, 0, &h_client)) != NV_OK) {
        fprintf(stderr, "root client alloc: 0x%08x\n", st);
        return -1;
    }
    memset(&dev, 0, sizeof(dev));
    dev.deviceId = 0;
    dev.hClientShare = h_client;
    if ((st = rm_alloc(h_client, H_DEVICE, NV01_DEVICE_0, &dev, sizeof(dev), NULL)) != NV_OK ||
        (memset(&sub, 0, sizeof(sub)),
         (st = rm_alloc(H_DEVICE, H_SUBDEV, NV20_SUBDEVICE_0, &sub, sizeof(sub), NULL)) != NV_OK) ||
        (st = rm_alloc(H_SUBDEV, H_I2C, NV40_I2C, NULL, 0, NULL)) != NV_OK) {
        fprintf(stderr, "object alloc: 0x%08x\n", st);
        return -1;
    }
    return 0;
}

static void close_rm(void)
{
    NVOS00_PARAMETERS f;
    if (!h_client)
        return;
    memset(&f, 0, sizeof(f));
    f.hRoot = h_client;
    f.hObjectParent = h_client;
    f.hObjectOld = h_client;
    nv_ioctl(ctl_fd, NV_ESC_RM_FREE, &f, sizeof(f));
}

struct probe {
    const char *what;
    NvU8 addr7;
    NvU32 speed;     /* NV402C_CTRL_I2C_FLAGS_SPEED_MODE_* value */
    int with_reg;    /* 1: I2C_BUFFER_RW read of register 0x5A; 0: plain I2C_BLOCK_RW read */
    NvU32 len;
};

static const struct probe probes[] = {
    { "control A: 0x4A reg 0x5A at 100 kHz (same as i2cget -y 8 0x4A 0x5A i 32)",
      0x4A, NV402C_CTRL_I2C_FLAGS_SPEED_MODE_100KHZ, 1, 32 },
    { "control B: plain read at 0x30, 10 kHz (no device expected)",
      0x30, NV402C_CTRL_I2C_FLAGS_SPEED_MODE_10KHZ, 0, 1 },
    { "target: 0x4A reg 0x5A at 10 kHz (FireStorm identification read)",
      0x4A, NV402C_CTRL_I2C_FLAGS_SPEED_MODE_10KHZ, 1, 32 },
};

static void run_probe(const struct probe *pr)
{
    NV402C_CTRL_I2C_TRANSACTION_PARAMS t;
    NVOS54_PARAMETERS c;
    NvU8 buf[32];
    NvU32 i;

    memset(&t, 0, sizeof(t));
    memset(buf, 0xEE, sizeof(buf));
    t.portId = RM_PORT;
    t.flags = DRF_NUM(402C_CTRL, _I2C_FLAGS, _SPEED_MODE, pr->speed);
    t.deviceAddress = (NvU16)(pr->addr7 << 1);
    if (pr->with_reg) {
        t.transType = NV402C_CTRL_I2C_TRANSACTION_TYPE_I2C_BUFFER_RW;
        t.transData.i2cBufferData.bWrite = NV_FALSE;
        t.transData.i2cBufferData.warFlags = NV402C_CTRL_I2C_BUFFER_RW_WAR_FLAGS_NONE;
        t.transData.i2cBufferData.registerAddress = 0x5A;
        t.transData.i2cBufferData.messageLength = pr->len;
        t.transData.i2cBufferData.pMessage = NV_PTR_TO_NvP64(buf);
    } else {
        t.transType = NV402C_CTRL_I2C_TRANSACTION_TYPE_I2C_BLOCK_RW;
        t.transData.i2cBlockData.bWrite = NV_FALSE;
        t.transData.i2cBlockData.messageLength = pr->len;
        t.transData.i2cBlockData.pMessage = NV_PTR_TO_NvP64(buf);
    }

    memset(&c, 0, sizeof(c));
    c.hClient = h_client;
    c.hObject = H_I2C;
    c.cmd = NV402C_CTRL_CMD_I2C_TRANSACTION;
    c.params = NV_PTR_TO_NvP64(&t);
    c.paramsSize = sizeof(t);

    printf("%s\n", pr->what);
    if (nv_ioctl(ctl_fd, NV_ESC_RM_CONTROL, &c, sizeof(c)) < 0) {
        printf("  ioctl failed: %s\n", strerror(errno));
        return;
    }
    printf("  RM status 0x%08x%s\n", c.status, c.status == NV_OK ? " (NV_OK)" : "");
    if (c.status != NV_OK)
        return;
    printf("  hex:  ");
    for (i = 0; i < pr->len; i++)
        printf("%02X ", buf[i]);
    printf("\n  text: '");
    for (i = 0; i < pr->len; i++)
        putchar(buf[i] >= 0x20 && buf[i] <= 0x7e ? buf[i] : '.');
    printf("'\n");
}

int main(int argc, char **argv)
{
    size_t i;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <driver-version, e.g. 615.71.09>\n", argv[0]);
        return 2;
    }
    if (check_card() || open_rm(argv[1])) {
        close_rm();
        return 1;
    }
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
        run_probe(&probes[i]);
        usleep(200 * 1000);
    }
    close_rm();
    return 0;
}
