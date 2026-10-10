#include <stdio.h>
#include <stddef.h>
#include "nvtypes.h"
#include "nvos.h"
#include "class/cl0080.h"
#include "class/cl2080.h"
#include "ctrl/ctrl402c.h"
#include "nv-ioctl.h"
typedef nv_ioctl_rm_api_version_t VERSION_T;
int main(void){
#include "dump.inc"
  /* buffer member */
  printf("I2C_BUFFER regaddr %zu len %zu ptr %zu\n", offsetof(NV402C_CTRL_I2C_TRANSACTION_DATA_I2C_BUFFER_RW,registerAddress), offsetof(NV402C_CTRL_I2C_TRANSACTION_DATA_I2C_BUFFER_RW,messageLength), offsetof(NV402C_CTRL_I2C_TRANSACTION_DATA_I2C_BUFFER_RW,pMessage));
  printf("TYPE I2C_BUFFER_RW %d\n", NV402C_CTRL_I2C_TRANSACTION_TYPE_I2C_BUFFER_RW);
  return 0; }
