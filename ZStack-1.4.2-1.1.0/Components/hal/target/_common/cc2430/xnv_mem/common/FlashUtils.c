#define ENABLE_BIT_DEFINITIONS 1
#include "zcomdef.h"
#include "FlashUtils.h"
#include <ioCC2430.h>
#include "OADSupport.h"
#include "OSAL_Memory.h"

#ifdef NV_IS_I2C
#include "i2cSupport.h"
#endif

#ifdef NV_IS_SPI
#include "serialize.h"
#endif

#define STATIC static


#define FERASE  0x01  // Page erase: erase=1
#define FWRITE  0x02  // Page write: write=1
#define FCONRD  0x10  // Continuous read: enable=1
#define FSWBSY  0x40  // Single write: busy=1
#define FBUSY   0x80  // Write/erase: busy=1
#define FWBUSY  0xC0  // Flash write: busy=1

/******************************************************************************
 ** ANY VARIABLES DECLARED 'STATIC' MUST BE USED _ONLY_ IN BOOT STARTUP CODE
 ** memory addresses assigned to static variables will not be valid when
 ** application code is running. they are valid only when the boot startup
 ** code is running.
*******************************************************************************/

/*********************************************************************
 * MACROS
 */
//Macro for erasing a given flash page
#define ERASE           0x01
#define FLASH_ERASE_PAGE(page) \
   do{                         \
      FADDRH = (page) << 1;    \
      FCTL = ERASE;            \
      asm("NOP");              \
      while(FCTL == 0x80);     \
   }while (0)

// DMA Sturcture
typedef struct
{
  uint8  SRC_HI;
  uint8  SRC_LO;
  uint8  DST_HI;
  uint8  DST_LO;
  uint8  VLEN;
  uint8  LEN;
  uint8  TRIG;
  uint8  INCMODE;
} DMA_t;

static DMA_t  __xdata FlashDMA;

extern __near_func  uint8  GetCodeByte(uint32 address);

// set up the mailbox in RAM
extern mbox_t mbox;


// LOCAL FUNCTIONS
STATIC  uint8  CheckCode(image_t, uint32, uint32);
STATIC  uint8  GetPreamble(image_t, uint32, preamble_t *);
STATIC  uint32 CRC32Value(uint16);
STATIC  uint8  imageCRC(image_t, uint32, uint32, preamble_t *);
STATIC  uint8  AccessStatus(action_t, sfield_t, void *);
STATIC  uint8  ReadFlash(image_t, uint32 page, uint8 __xdata *buf, uint16 len);
STATIC  uint8  WriteFlash(image_t, uint32 page, uint8 __xdata *buf, uint16 len);

__near_func void   flashErasePage(uint8, uint8 __xdata *);

// EXTERNAL FUNCTIONS
__near_func void  halFlashDmaTrigger(void);

static int8 (*rw_Xmem)(uint8, uint32, uint8 *, uint16);

void FlashInit()
{
#ifdef NV_IS_SPI
    // initialize SPI-dataflash interface
    DF_spiInit(&rw_Xmem);
#endif

#ifdef NV_IS_I2C
    // initialize I2C-dataflash interface
    DF_i2cInit(&rw_Xmem);
#endif

     // populate mailbox function pointers
    mbox.ReadFlash         = ReadFlash;
    mbox.WriteFlash        = WriteFlash;
    mbox.CheckCodeSanity   = CheckCode;
    mbox.GetPreamble       = GetPreamble;
    mbox.AccessZLOADStatus = AccessStatus;

    return;
}

void GetFlashRWFunc(int8 (**func)(uint8, uint32, uint8 *, uint16))
{
#ifdef NV_IS_SPI
    // initialize SPI-dataflash interface
    DF_spiInit(&rw_Xmem);
#endif

#ifdef NV_IS_I2C
    // initialize I2C-dataflash interface
    DF_i2cInit(&rw_Xmem);
#endif

  *func = rw_Xmem;
}

/************************************************************************************
 *   uint8 CheckCode
 *      Purpose: Do all code sanity checks. Currently, this is to check the magic
 *               and check the FCS.
 *
 *        This routine is avaliable to application code. It is re-entrant but not
 *        thread-safe.
 *
 *      Arguments:
 *        input:
 *          baseAddr: base address of image
 *          firstPageAddr: address of first page
 *        output:
 *          none.
 *      Returns:
 *        0: Code at spefied address is sane
 *        1: Code at spefied address is NOT sane
 ************************************************************************************/
STATIC  uint8 CheckCode(image_t itype, uint32 baseAddr, uint32 firstPageAddress)
{
    uint8      rc;
    preamble_t preamble;

    // get preamble
    mbox.GetPreamble(itype, firstPageAddress, &preamble);

    // check for magic
    if ((preamble.pre_Magic[0] != PREAMBLE_MAGIC1) || (preamble.pre_Magic[1] != PREAMBLE_MAGIC2))  {
        return 1;
    }

    // only thing left to check for is CRC
    rc = imageCRC(itype, baseAddr, firstPageAddress, &preamble);

    return rc;
}

/************************************************************************************
 *   uint8 imageCRC
 *      Purpose: compute image CRC. Taken as an amalgam from various sources. There
 *      are a number of slightly varying implementations. It was hard to find test
 *      data for this specific implementation. The test used is the very weak test
 *      in which the ASCII string "123456789" is checked.
 *
 *      In this space it is calculating the CRC on downloaded image only. The
 *      application will never request the CRC calculated on the active image.
 *
 *      Note: the routine that follows this one also supports the CRC calculation.
 *
 *      Arguments:
 *        input:
 *          baseAddr: base address of image
 *          firstPageOffset: offset from baseAddr of actual first page of image
 *          premable: pointer to preamble info
 *        output:
 *          none
 *      Returns:
 *        0: CRC calculation matches the CRC in image
 *        1: CRC calculation does NOT match the CRC in image
 ************************************************************************************/
#define  DFBUFSIZE  32
STATIC  uint8 imageCRC(image_t itype, uint32 baseAddr, uint32 firstPageAddress, preamble_t *preamble)
{
    uint8  ch, scnt, dfIdx, dfSize;
    uint32 imgCRC, imglen, ulCRC, ulTemp1, ulTemp2, lastAddress, baseOrig;
#ifdef CC2430_BOOT_CODE
    uint8 cptr[DFBUFSIZE];
#else
    uint8 *cptr;
#endif

    imglen = preamble->pre_Length;

    // figure out wrap address in case image wraps within download area
    // the number is too big if the image doesn't wrap but that won't
    // matter becuase the loop(s) below will terminate since the image
    // length count is reached. this value matters only if the image
    // stored actually wraps.
    lastAddress = (baseAddr + imglen - 1) | ((itype == IMAGE_DL) ? (DF_PAGESIZE-1) : (CHIP_PAGESIZE-1));
    baseOrig    = baseAddr;
    baseAddr    = firstPageAddress;

    // start with all bits on. common practice to guard against initial sequence of
    // 0's (even though we don't have this case)
    ulCRC  = 0xFFFFFFFF;
    imgCRC = 0;
    scnt   = 0;

    dfIdx = 0;
    if (IMAGE_DL == itype)  {
      dfIdx = dfSize = DFBUFSIZE;
#ifndef CC2430_BOOT_CODE
      if (!(cptr = osal_mem_alloc(DFBUFSIZE)))  {
      // Plan B if the malloc fails...
        dfIdx = dfSize = 1;
        cptr           = &ch;
      }
#endif
    }

    // the same logic is used to both calculate the checksum and to retrieve stored
    // CRC. this is because the method below already deals with wrapping so we
    // might as well use the same logic to get the CRC. Otherwise the wrapping
    // logic has to be repeated: though unlikely, the CRC _could_ span the wrap.
    do  {
        for (; baseAddr <= lastAddress && imglen; baseAddr++, imglen--)  {
            // next byte...
          if (IMAGE_ACTIVE == itype)  {
            ch = GetCodeByte(baseAddr);
          }
          else  {
            if (dfIdx >= dfSize)  {
              rw_Xmem(XMEM_READ, baseAddr, cptr, dfSize);
              dfIdx = 0;
            }
            ch = cptr[dfIdx++];
          }
          if (imglen > FCS_LENGTH)  {
              ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
              ulTemp2 = CRC32Value(((uint16)ulCRC ^ ch) & 0xFF);
              ulCRC   = ulTemp1 ^ ulTemp2;
          }
          else  {
              // we've reached the stored CRC. create the actual
              // value by shifting in the bytes.
              imgCRC = imgCRC | (uint32)ch<<scnt;
              scnt  += 8;
          }
        }
        baseAddr = baseOrig;
    } while (imglen);

#ifndef CC2430_BOOT_CODE
    if (dfSize > 1)  {
      osal_mem_free(cptr);
    }
#endif

    // XOR with all bits on. this is a common practice
    ulCRC ^= 0xFFFFFFFF;

    // match?
    if (ulCRC != imgCRC)  {
        // "Close, but no match and the board goes back" (remember that?)
        return 1;
    }

    return 0;
}

STATIC  uint32 CRC32Value(uint16 inCRC)
{
    int16  j;
    uint32 ulCRC = inCRC;

    // for this byte (inCRC)...
    for (j=8; j; j--) {
        // lsb on? yes -- shift right and XOR with poly. else just shift
        if (ulCRC & 1) {
            ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
        }
        else  {
            ulCRC >>= 1;
        }
    }

    return ulCRC;
}

/************************************************************************************
 *   uint8 GetPreamble
 *      Purpose: get image preamble
 *
 *        This routine is avaliable to application code. It is re-entrant but not
 *        thread-safe.
 *
 *      Arguments:
 *        input:
 *          baseAddr: base address of image
 *        output:
 *          info: void pointer to destination memory
 *      Returns:
 *        0: OK
 *        1: error
 *
 *   NOTE: this works only because the preamble is always on the first flash page.
 *         This is important given that the downloaded image can wrap. It won't wrap
 *         within a page.
 ************************************************************************************/
STATIC uint8 GetPreamble(image_t itype, uint32 baseAddr, preamble_t *info)
{
    uint8 preOffset;

    if (IMAGE_DL == itype)  {

        mbox.AccessZLOADStatus(ACTION_GET, SFIELD_PREAMBLE_OFFSET, &preOffset);
        baseAddr += preOffset;
        rw_Xmem(XMEM_READ, baseAddr, (uint8 *)info, sizeof(preamble_t));
    }
    else if (IMAGE_ACTIVE == itype) {
        uint8 i;

        baseAddr += PREAMBLE_BASE;

        // need > 16-bit pointer arithmetic
        for (i=0; i<sizeof(preamble_t); ++i, ++baseAddr)  {
            *((uint8 *)info+i) = GetCodeByte(baseAddr);
        }
    }
    else  {
      return 1;
    }

    return 0;
}


/************************************************************************************
 *   uint8 AccessStatus
 *      Purpose: get and set fields in OAD persistent memory
 *
 *        This routine is avaliable to application code. It neither re-entrant nor
 *        thread-safe.
 *
 *      Arguments:
 *        input:
 *          action: get or set
 *          sfield: which field to operate on
 *        output:
 *          info: void pointer to source (set) or destination (get) memory
 *      Returns:
 *        0: OK
 *        1: bad action or field
 ************************************************************************************/
STATIC  uint8 AccessStatus(action_t action, sfield_t sfield, void *info)
{
    uint8   bcnt, os, rc = 0;

    // all results for this function (except the boot sector size) are byte copies
    // the offset and byte counts are set based on boot code knowledge of how the
    // persistent memory is arranged. the offsets and sizes are set here. they are
    // defined in a header. they could be calculated but it takes a lot more code.
    switch (sfield)  {
#if 0   // base and first page always the same for the 8051 with external memory
        case SFIELD_DLFIRSTPAGE_ADDR:
            bcnt = sizeof(uint32);
            os   = OADPM_OS_FIRSTPAGE_ADDR;
            break;
#else
        case SFIELD_DLFIRSTPAGE_ADDR:
#endif
        case SFIELD_DLBASE:
            bcnt = sizeof(uint32);
            os   = OADPM_OS_BADDR;
            break;

        case SFIELD_PREAMBLE_OFFSET:
            bcnt = sizeof(uint8);
            os   = OADPM_OS_PREAMBLE_OFFSET;
            break;

            // we don't need to support the next few accesses now. they were intended
            // to support the case where the entire image could not be downloaded and
            // we needed to have server context in case of failure. they have been
            // tested but are removed for now.
#if 0
        case SFIELD_SERVER_IEEE_ADDR:
            bcnt = SIZEOF_IEEE_ADDRESS;
            os   = OADPM_OS_IEEE_ADDR;
            break;

        case SFIELD_SERVER_NWK_ADDR:
            bcnt = sizeof(uint16);
            os   = OADPM_OS_NWK_ADDR;
            break;

        case SFIELD_SERVER_ENDPOINT:
            bcnt = sizeof(uint8);
            os   = OADPM_OS_ENDPT;
            break;

        case SFIELD_IMAGE_ID:
            bcnt = SIZEOF_DLIMAGE_INFO;
            os   = OADPM_OS_DLIMG_INFO;
            break;
#endif
        case SFIELD_STATUS:
            bcnt = sizeof(uint16);
            os   = OADPM_OS_STATUS;
            break;

#if 0  // not used for CC2430
        case SFIELD_BOOTSIZE:
            // doesn't really belong here because it isn't kept in PM but
            // it is conventient to support it with this API
            if (action == ACTION_SET)  {
                // not legal
                return 1;
            }
            *(uint16 *)info = bootSize;
            return 0;
#endif
        case SFIELD_EXTERNAL_MEM_OS:
            // doesn't really belong here because it isn't kept in PM but
            // it is conventient to support it with this API
            if (action == ACTION_SET)  {
                // not legal
                return 1;
            }
            *(uint16 *)info = DL_BASE_ADDRESS;
            return 0;

        default:
            rc = 1;
            break;
    }

    if (!rc)  {
        if (action == ACTION_SET)  {
          rw_Xmem(XMEM_WRITE, SYSNV_BASE_ADDRESS + os, info, bcnt);
        }
        else if (action == ACTION_GET)  {
          rw_Xmem(XMEM_READ, SYSNV_BASE_ADDRESS + os, info, bcnt);
        }
        else  {
            rc = 1;
        }
    }

    return rc;
}

STATIC  uint8 ReadFlash(image_t itype, uint32 address, uint8 __xdata *buf, uint16 len)
{
  uint8 rc = 0;

  if (IMAGE_ACTIVE == itype)  {
    uint16 i = len;

    while (i--)  {
      *buf++ = GetCodeByte(address++);
    }
  }
  else if (IMAGE_DL == itype)  {
    rw_Xmem(XMEM_READ, address, buf, len);
  }
  else  {
    rc = 1;
  }

  return rc;
}

STATIC  uint8 WriteFlash(image_t itype, uint32 address, uint8 __xdata *buf, uint16 len)
{
  uint8 rc = 1;  // 1 == write was OK.

  if (IMAGE_ACTIVE == itype)  {
    uint16  fbase, i;
    uint8   page = address >> CHIP_SHIFTCOUNT;

    // if we're on a page boundary for the target (chip) flash, erase the page
    // this implies that we either write a page sequentially or minimally
    // write the first address on the first call so the page gets eraseed.
    if (!(address & 0x000007FF))  {
      FLASH_ERASE_PAGE(page);
    }


    // OK. do the rest. use DMA Channel 0
    // Set up Flash DMA

    FlashDMA.DST_HI  = 0xdf;
    FlashDMA.DST_LO  = 0xaf;
    FlashDMA.VLEN    = 0x00;
    FlashDMA.LEN     = 0x04;
    FlashDMA.TRIG    = 0x12;
    FlashDMA.INCMODE = 0x42;

    DMA0CFGH = (uint16)&FlashDMA >> 8;
    DMA0CFGL = (uint16)&FlashDMA;

    fbase = (page << 9) + ((address & 0x000007FF)>>2);

    // 'len' must be 0 (mod 4)
    for (i=0; i<(len/4); ++i)  {
      // set buffer pointer in DMA config info
      FlashDMA.SRC_HI  = (uint16)buf >> 8;
      FlashDMA.SRC_LO  = (uint16)buf;
      // Set Flash write address based on fbase
      FADDRH = fbase >> 8;
      FADDRL = fbase;

      DMAARM = 0x01;
      halFlashDmaTrigger();
      while (!(DMAIRQ & 0x01));   // wait for DMA transfer
      while ( FCTL & FWBUSY );  // wait until Flash controller not busy
      DMAIRQ &= ~0x01;
      fbase++;
      buf += 4;
    }
  }
  else if (IMAGE_DL == itype)  {
    rw_Xmem(XMEM_WRITE, address, buf, len);
  }
  else  {
    rc = 0;
  }

  return rc;
}


