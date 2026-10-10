#include <stdio.h>
#include "nvtypes.h"
#include "nvmisc.h"
#include "ctrl/ctrl402c.h"
int main(void){
 printf("10KHZ flags=0x%x  100KHZ flags=0x%x  field shift=%d mask=0x%x\n",
  DRF_NUM(402C_CTRL,_I2C_FLAGS,_SPEED_MODE,NV402C_CTRL_I2C_FLAGS_SPEED_MODE_10KHZ),
  DRF_NUM(402C_CTRL,_I2C_FLAGS,_SPEED_MODE,NV402C_CTRL_I2C_FLAGS_SPEED_MODE_100KHZ),
  DRF_SHIFT(NV402C_CTRL_I2C_FLAGS_SPEED_MODE), DRF_SHIFTMASK(NV402C_CTRL_I2C_FLAGS_SPEED_MODE));
 printf("addr 0x4A -> deviceAddress 0x%x\n", 0x4A<<1); return 0; }
