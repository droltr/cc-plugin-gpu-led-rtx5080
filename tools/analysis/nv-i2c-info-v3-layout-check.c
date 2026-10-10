/* Layout check of NV_I2C_INFO_V3 for i386 (FireStorm is a 32-bit binary).
 * The struct below repeats the field list of NV_I2C_INFO_V3 from NVIDIA NVAPI SDK
 * nvapi.h (github.com/NVIDIA/nvapi, commit 70d337d, MIT). Compile with
 * gcc -m32 -fsyntax-only; any wrong offset fails a _Static_assert.
 */
typedef unsigned int NvU32; typedef unsigned char NvU8;
typedef enum { S0,S1,S2,S3,S4,S5,S6 } NV_I2C_SPEED;
typedef struct { NvU32 version; NvU32 displayMask; NvU8 bIsDDCPort; NvU8 i2cDevAddress;
 NvU8* pbI2cRegAddress; NvU32 regAddrSize; NvU8* pbData; NvU32 cbSize; NvU32 i2cSpeed;
 NV_I2C_SPEED i2cSpeedKhz; NvU8 portId; NvU32 bIsPortIdSet; } NV_I2C_INFO_V3;
#define O(f,v) _Static_assert(__builtin_offsetof(NV_I2C_INFO_V3,f)==v, #f);
O(version,0x00) O(displayMask,0x04) O(bIsDDCPort,0x08) O(i2cDevAddress,0x09)
O(pbI2cRegAddress,0x0c) O(regAddrSize,0x10) O(pbData,0x14) O(cbSize,0x18)
O(i2cSpeed,0x1c) O(i2cSpeedKhz,0x20) O(portId,0x24) O(bIsPortIdSet,0x28)
_Static_assert(sizeof(NV_I2C_INFO_V3)==0x2c, "size");
_Static_assert((sizeof(NV_I2C_INFO_V3)|(3<<16))==0x3002c, "VER3");
