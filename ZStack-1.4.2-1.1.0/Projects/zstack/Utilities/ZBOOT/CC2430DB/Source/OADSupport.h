#ifndef OADSUPPORT_H
#define OADSUPPORT_H

// select the persistent memory field to set or get
typedef enum  {
    SFIELD_DLFIRSTPAGE_ADDR,  // download image first page offset in flashed area
    SFIELD_DLBASE,            // download image base address
    SFIELD_SERVER_IEEE_ADDR,  // IEEE address of Server device
    SFIELD_SERVER_NWK_ADDR,   // netowrk address of Server device
    SFIELD_SERVER_ENDPOINT,   // endpoint on Server device
    SFIELD_IMAGE_ID,          // image id (3 elements from premable) to be downloaded from Server
    SFIELD_STATUS,            // various status bits
    SFIELD_BOOTSIZE,          // size (in bytes) of boot sector (read-only)
    SFIELD_PREAMBLE_OFFSET,   // offset of premable from beginning of image
    SFIELD_EXTERNAL_MEM_OS    // external memory offset
} sfield_t;

// persistent memory field action choices
typedef enum  {
    ACTION_SET,
    ACTION_GET
} action_t;

typedef enum  {
    IMAGE_ACTIVE,
    IMAGE_DL
} image_t;

// bit locations for ZLOAD status word. the status word should be handled as a read-modify-write
// transaction. in addtion the supporting API is NOT thread-safe. be careful
#define ZLOADSTATUS_CHECKIMAGE     0x0001    // 1 == check operational image sanity; 0 == don't check

// PREAMBLE STRUCTURE DEFINITION
typedef struct  {
    char   pre_Magic[2];
    uint32 pre_Length;
    uint16 pre_ImgVersion;
    uint16 pre_ManuID;
    uint16 pre_ProductID;
} preamble_t;

typedef struct  {
    char   pre_Magic[2];
    uint32 pre_Length;
} briefpreamble_t;

// RAM MAILBOX STRUCTURE DEFINITION
typedef struct  {
     uint8      (*ReadFlash)(image_t, uint32, uint8 __xdata *, uint16);
     uint8      (*WriteFlash)(image_t, uint32, uint8 __xdata *, uint16);
     uint8      (*CheckCodeSanity)(image_t, uint32, uint32);
     uint8      (*GetPreamble)(image_t, uint32, preamble_t *);
     uint8      (*AccessZLOADStatus)(action_t, sfield_t, void *);
} mbox_t;


#define INTVEC_SIZE            0x93
#define PREAMBLE_BASE          INTVEC_SIZE
#define OP_IMAGE_BASE_ADDRESS  (0x800)

#define PREAMBLE_MAGIC1  ((char)'F')
#define PREAMBLE_MAGIC2  ((char)'8')

#define NO_DL_IMAGE_ADDRESS  (0xFFFFFFFFL)

#endif
