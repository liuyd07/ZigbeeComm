#ifndef I2C_SUPPORT_H
#define I2C_SUPPORT_H


// support for NV in external NV memory
#define FLASH_WRITE_BUFFER_SIZE (0x100)
#define NUM_NV_PAGES            (0x08)
#define NUM_SYS_PAGES           (0x01)
#define NV_BASE_ADDRESS         (0x00)
#define NUM_DL_FALLOW_PAGES     (0x08)
#define DL_BASE_ADDRESS         ((NUM_NV_PAGES+NUM_SYS_PAGES+NUM_DL_FALLOW_PAGES)*FLASH_WRITE_BUFFER_SIZE)
#define SYSNV_BASE_ADDRESS      (NUM_NV_PAGES*FLASH_WRITE_BUFFER_SIZE)

#define DF_PAGESIZE             FLASH_WRITE_BUFFER_SIZE
#define DF_SHIFTCOUNT           8

void  DF_i2cInit( int8 (**readWrite)(uint8, uint32, uint8 *, uint16));

#endif
