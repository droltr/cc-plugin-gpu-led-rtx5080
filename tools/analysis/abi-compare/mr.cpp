#include <cstdio>
#include <cstddef>
#include "nvrm_api.h"
typedef nv_ioctl_rm_api_version_t VERSION_T;
int main(){
#include "dump.inc"
  printf("I2C_BUFFER regaddr %zu len %zu ptr %zu\n", offsetof(NV402C_DATA_I2C_BUFFER_RW,registerAddress), offsetof(NV402C_DATA_I2C_BUFFER_RW,messageLength), offsetof(NV402C_DATA_I2C_BUFFER_RW,pMessage));
  printf("TYPE I2C_BUFFER_RW %d\n", NV402C_TRANS_I2C_BUFFER_RW);
  return 0; }
