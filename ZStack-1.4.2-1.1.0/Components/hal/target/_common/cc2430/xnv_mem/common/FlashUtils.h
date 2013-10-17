#ifndef FLASHUTILS_H
#define FLASHUTILS_H

#define SIZEOF_IEEE_ADDRESS    8
#define SIZEOF_DLIMAGE_INFO    6


#define CRC32_POLYNOMIAL   ((uint32)0xEDB88320)  // reveresed version or 802.3 polynomial 0x04C11DB7
#define FCS_LENGTH         4

// structure to hold OAD persistent memory information. this format is visible only to boot code
// others must use access API defined in mbox.
typedef struct  {
    uint8    OADPM_ServerIEEEAddress[SIZEOF_IEEE_ADDRESS];  // valid only during download session
    uint16   OADPM_ServerNWKAddress;                        // valid only during download session
    uint8    OADPM_ServerEndpoint;                          // valid only during download session
    uint8    OADPM_DLImageInfo[SIZEOF_DLIMAGE_INFO];        // valid only during download session
    uint8    OADPM_DLImagePreambleOffset;                   // valid only if base address and 1st page offset valid
    uint32   OADPM_DLFirstPageOffset;                       // 0xFFFFFFFF if no image present
    uint32   OADPM_DLBaseAddress;                           // 0xFFFFFFFF if no image present
    uint16   OADPM_Status;                                  // bit map of status info
} oadpm_t;

#define OADPM_OS_IEEE_ADDR       (0)
#define OADPM_OS_NWK_ADDR        (OADPM_OS_IEEE_ADDR + SIZEOF_IEEE_ADDRESS)
#define OADPM_OS_ENDPT           (OADPM_OS_NWK_ADDR + sizeof(uint16))
#define OADPM_OS_DLIMG_INFO      (OADPM_OS_ENDPT + sizeof(uint8))
#define OADPM_OS_PREAMBLE_OFFSET (OADPM_OS_DLIMG_INFO + SIZEOF_DLIMAGE_INFO)
#define OADPM_OS_FIRSTPAGE_ADDR  (OADPM_OS_PREAMBLE_OFFSET + sizeof(uint8))
#define OADPM_OS_BADDR           (OADPM_OS_FIRSTPAGE_ADDR + sizeof(uint32))
#define OADPM_OS_STATUS          (OADPM_OS_BADDR + sizeof(uint32))

#define XMEM_WRITE     0
#define XMEM_READ      1

#define CHIP_PAGESIZE     (0x800)
#define CHIP_SHIFTCOUNT   (11)

void FlashInit(void);
void GetFlashRWFunc(int8 (**)(uint8, uint32, uint8 *, uint16));

#endif
