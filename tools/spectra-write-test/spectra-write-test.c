/*
 * spectra-write-test: first write test for the ZOTAC SPECTRA LED controller of the
 * RTX 5080 SOLID CORE OC (10de:2c02, subsystem 19da:1765) through the NVIDIA Resource
 * Manager API (/dev/nvidiactl), RM port 1, address 0x4A, register 0x5A, 10 kHz.
 *
 * Commands:
 *   status   read the controller status (no write)
 *   test     write the fixed TEST packet (static blue), then read back
 *   restore  write the fixed RESTORE packet (green breathe, the state read on
 *            2026-10-10), then read back
 *
 * Packet layout (32 bytes) from ZOTAC FireStorm 5.0.0.015E function 0x430880:
 *   reset, on, mode, R, G, B, on-time, off-time, preset, 23 x 0x00
 * Status read: 10-byte ID, then bytes 1..8 of the last packet (on .. preset).
 *
 * Safety:
 *   - built only against the official headers of the installed driver;
 *   - refuses to run unless the GPU is exactly 10de:2c02 / 19da:1765, the only NVIDIA
 *     GPU, device minor 0, and the driver version matches exactly;
 *   - refuses to write unless the controller ID read first is "N765E-1001";
 *   - only two fixed packets can be written; no option sets address, register or data.
 *
 * Builds against NVIDIA open-gpu-kernel-modules headers (MIT). See THIRD_PARTY_NOTICES.md.
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

#define H_DEVICE 0xA5080011u
#define H_SUBDEV 0xA5080012u
#define H_I2C    0xA5080013u

#define RM_PORT      1
#define SPECTRA_ADDR 0x4A
#define SPECTRA_REG  0x5A
#define PKT_LEN      32
#define ID_LEN       10
#define EXPECT_ID    "N765E-1001"

#ifndef SPECTRA_SOURCE_COMMIT
#define SPECTRA_SOURCE_COMMIT "unknown"
#endif
#ifndef SPECTRA_DRIVER_VERSION
#define SPECTRA_DRIVER_VERSION "unknown"
#endif

/* reset, on, mode, R, G, B, on-time, off-time, preset; the rest is zero */
static const NvU8 PKT_TEST[9]    = { 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x2D, 0x0A, 0x08 };
static const NvU8 PKT_RESTORE[9] = { 0x00, 0x01, 0x01, 0x11, 0xFF, 0x28, 0x14, 0x0A, 0x08 };

_Static_assert(DRF_DEF(402C_CTRL, _I2C_FLAGS, _SPEED_MODE, _10KHZ) == 0x8, "10 kHz flag");
_Static_assert(sizeof(PKT_TEST) == 9 && sizeof(PKT_RESTORE) == 9, "packet head size");

static int ctl_fd = -1;
static int dev_fd = -1;
static NvHandle h_client;

static int nv_ioctl(int fd, unsigned nr, void *arg, size_t size)
{
    return ioctl(fd, _IOC(_IOC_READ | _IOC_WRITE, NV_IOCTL_MAGIC, nr, size), arg);
}

static unsigned read_hex(const char *name)
{
    char path[256], buf[32] = "";
    FILE *f;

    snprintf(path, sizeof(path), "%s/%s", PCI_DIR, name);
    f = fopen(path, "r");
    if (!f || !fgets(buf, sizeof(buf), f)) {
        if (f)
            fclose(f);
        return 0;
    }
    fclose(f);
    return (unsigned)strtoul(buf, NULL, 16);
}

static int check_card(void)
{
    unsigned dev = read_hex("device"), subv = read_hex("subsystem_vendor"),
             subd = read_hex("subsystem_device");
    DIR *d;
    struct dirent *e;
    int count = 0, minor = -1;
    char line[128];
    FILE *f;

    printf("card: %04x subsystem %04x:%04x\n", dev, subv, subd);
    if (dev != EXPECT_DEV || subv != EXPECT_SUBV || subd != EXPECT_SUBD) {
        fprintf(stderr, "refusing: not the expected card\n");
        return -1;
    }

    d = opendir(GPUS_DIR);
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
    dev_fd = open("/dev/nvidia0", O_RDWR);
    if (ctl_fd < 0 || dev_fd < 0) {
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
    if ((st = rm_alloc(h_client, H_DEVICE, NV01_DEVICE_0, &dev, sizeof(dev), NULL)) != NV_OK) {
        fprintf(stderr, "device alloc: 0x%08x\n", st);
        return -1;
    }
    memset(&sub, 0, sizeof(sub));
    if ((st = rm_alloc(H_DEVICE, H_SUBDEV, NV20_SUBDEVICE_0, &sub, sizeof(sub), NULL)) != NV_OK) {
        fprintf(stderr, "subdevice alloc: 0x%08x\n", st);
        return -1;
    }
    if ((st = rm_alloc(H_SUBDEV, H_I2C, NV40_I2C, NULL, 0, NULL)) != NV_OK) {
        fprintf(stderr, "i2c alloc: 0x%08x\n", st);
        return -1;
    }
    return 0;
}

static void close_rm(void)
{
    NVOS00_PARAMETERS f;

    if (h_client) {
        memset(&f, 0, sizeof(f));
        f.hRoot = h_client;
        f.hObjectParent = h_client;
        f.hObjectOld = h_client;
        nv_ioctl(ctl_fd, NV_ESC_RM_FREE, &f, sizeof(f));
    }
    if (dev_fd >= 0)
        close(dev_fd);
    if (ctl_fd >= 0)
        close(ctl_fd);
}

/* One I2C_BUFFER_RW transaction on 0x4A register 0x5A at 10 kHz; returns the RM status. */
static NvU32 spectra_xfer(NvBool write, NvU8 *buf)
{
    NV402C_CTRL_I2C_TRANSACTION_PARAMS t;
    NVOS54_PARAMETERS c;

    memset(&t, 0, sizeof(t));
    t.portId = RM_PORT;
    t.flags = DRF_DEF(402C_CTRL, _I2C_FLAGS, _SPEED_MODE, _10KHZ);
    t.deviceAddress = (NvU16)(SPECTRA_ADDR << 1);
    t.transType = NV402C_CTRL_I2C_TRANSACTION_TYPE_I2C_BUFFER_RW;
    t.transData.i2cBufferData.bWrite = write;
    t.transData.i2cBufferData.warFlags = NV402C_CTRL_I2C_BUFFER_RW_WAR_FLAGS_NONE;
    t.transData.i2cBufferData.registerAddress = SPECTRA_REG;
    t.transData.i2cBufferData.messageLength = PKT_LEN;
    t.transData.i2cBufferData.pMessage = NV_PTR_TO_NvP64(buf);

    memset(&c, 0, sizeof(c));
    c.hClient = h_client;
    c.hObject = H_I2C;
    c.cmd = NV402C_CTRL_CMD_I2C_TRANSACTION;
    c.params = NV_PTR_TO_NvP64(&t);
    c.paramsSize = sizeof(t);

    if (nv_ioctl(ctl_fd, NV_ESC_RM_CONTROL, &c, sizeof(c)) < 0) {
        fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
        return NV_ERR_GENERIC;
    }
    return c.status;
}

static void print_bytes(const char *label, const NvU8 *b, int n)
{
    int i;

    printf("%s", label);
    for (i = 0; i < n; i++)
        printf("%02X ", b[i]);
    printf("\n");
}

/* Reads the status into st[PKT_LEN]; returns 0 on NV_OK with the expected ID. */
static int read_status(NvU8 *st)
{
    NvU32 rc;

    memset(st, 0xEE, PKT_LEN);
    rc = spectra_xfer(NV_FALSE, st);
    if (rc != NV_OK) {
        fprintf(stderr, "status read: RM status 0x%08x\n", rc);
        return -1;
    }
    print_bytes("status: ", st, PKT_LEN);
    printf("id: '%.*s'\n", ID_LEN, (const char *)st);
    if (memcmp(st, EXPECT_ID, ID_LEN) != 0) {
        fprintf(stderr, "refusing: controller ID is not %s\n", EXPECT_ID);
        return -1;
    }
    return 0;
}

static int write_packet(const NvU8 *head)
{
    NvU8 pkt[PKT_LEN], st[PKT_LEN];
    NvU32 rc;

    memset(pkt, 0, sizeof(pkt));
    memcpy(pkt, head, 9);
    print_bytes("write:  ", pkt, PKT_LEN);
    rc = spectra_xfer(NV_TRUE, pkt);
    printf("write RM status 0x%08x%s\n", rc, rc == NV_OK ? " (NV_OK)" : "");
    if (rc != NV_OK)
        return -1;

    /* 200 ms between requests, as OpenRGB MR !3380 does for this controller */
    usleep(200 * 1000);
    if (read_status(st))
        return -1;
    if (memcmp(st + ID_LEN, pkt + 1, 8) == 0) {
        printf("readback: bytes 10..17 match the packet (on .. preset)\n");
        return 0;
    }
    printf("readback: MISMATCH with the packet\n");
    return -1;
}

static int parse_byte(const char *text, NvU8 *out)
{
    char *end;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno || *text == '\0' || *end != '\0' || value > 255)
        return -1;
    *out = (NvU8)value;
    return 0;
}

static int set_color(const char *mode_text, const char *r_text, const char *g_text,
                     const char *b_text, const char *speed_text, const char *brightness_text)
{
    NvU8 packet[9] = { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x0A, 0x08 };
    NvU8 speed = 235;
    NvU8 brightness = 210;

    if (speed_text && parse_byte(speed_text, &speed)) {
        fprintf(stderr, "speed must be an integer from 0 to 255\n");
        return -1;
    }
    if (brightness_text && parse_byte(brightness_text, &brightness)) {
        fprintf(stderr, "brightness must be an integer from 0 to 255\n");
        return -1;
    }

    if (!strcmp(mode_text, "static")) {
        packet[2] = 0x00;
        packet[6] = 0x2D;
    } else if (!strcmp(mode_text, "breathe")) {
        packet[2] = 0x01;
        packet[6] = (NvU8)(255 - speed);
    } else if (!strcmp(mode_text, "fade")) {
        packet[2] = 0x02;
        packet[6] = (NvU8)(255 - speed);
    } else if (!strcmp(mode_text, "wink")) {
        packet[2] = 0x03;
        packet[6] = brightness;
        packet[7] = (NvU8)(255 - speed);
    } else if (!strcmp(mode_text, "spectrum")) {
        packet[2] = 0x04;
        packet[6] = (NvU8)(255 - speed);
    } else if (!strcmp(mode_text, "random")) {
        packet[2] = 0x05;
        packet[6] = brightness;
        packet[7] = (NvU8)(255 - speed);
    } else {
        fprintf(stderr, "mode must be static, breathe, fade, spectrum, wink or random\n");
        return -1;
    }
    if (parse_byte(r_text, &packet[3]) || parse_byte(g_text, &packet[4]) ||
        parse_byte(b_text, &packet[5])) {
        fprintf(stderr, "RGB values must be integers from 0 to 255\n");
        return -1;
    }
    return write_packet(packet);
}

int main(int argc, char **argv)
{
    NvU8 st[PKT_LEN];
    int rc = 1;

    if (argc == 2 && !strcmp(argv[1], "--build-info")) {
        printf("source_commit=%s\ndriver_version=%s\n", SPECTRA_SOURCE_COMMIT,
               SPECTRA_DRIVER_VERSION);
        return 0;
    }
    if (argc < 3 || (strcmp(argv[2], "set") && argc != 3) ||
        (!strcmp(argv[2], "set") && (argc < 7 || argc > 9)) ||
        (strcmp(argv[2], "status") && strcmp(argv[2], "test") &&
         strcmp(argv[2], "restore") && strcmp(argv[2], "set"))) {
        fprintf(stderr, "usage: %s <driver-version> status|test|restore\n", argv[0]);
        fprintf(stderr, "       %s <driver-version> set MODE R G B [speed 0..255] [brightness 0..255]\n", argv[0]);
        return 2;
    }
    if (check_card() || open_rm(argv[1]))
        goto out;
    if (read_status(st))
        goto out;

    if (!strcmp(argv[2], "status"))
        rc = 0;
    else if (!strcmp(argv[2], "test"))
        rc = write_packet(PKT_TEST) ? 1 : 0;
    else if (!strcmp(argv[2], "restore"))
        rc = write_packet(PKT_RESTORE) ? 1 : 0;
    else
        rc = set_color(argv[3], argv[4], argv[5], argv[6],
                       argc > 7 ? argv[7] : NULL, argc > 8 ? argv[8] : NULL) ? 1 : 0;
out:
    close_rm();
    return rc;
}
