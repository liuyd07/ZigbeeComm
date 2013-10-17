/*********************************************************************
    Filename:       OSAL_Nv.c
    Revised:        $Date: 2007-04-11 14:41:47 -0700 (Wed, 11 Apr 2007) $
    Revision:       $Revision: 13998 $

    Description: This module contains the OSAL non-volatile memory functions.

    Notes: A trick buried deep in initPage() requires that the MSB of the NV
           Item Id be reserved for use by this module.

    Copyright (c) 2007 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "hal_adc.h"
#include "hal_dma.h"
#include "osal.h"
#include "OSAL_Nv.h"
#include <ioCC2430.h>

#if !defined ( OSAL_NV_CLEANUP )
  #define OSAL_NV_CLEANUP  FALSE
#endif

/*********************************************************************
 * CONSTANTS
 */

#define OSAL_NV_DMA_CH         (&dmaCh0)

#define OSAL_NV_ACTIVE          0x00
#define OSAL_NV_ERASED          0xFF
#define OSAL_NV_ERASED_ID       0xFFFF
#define OSAL_NV_ZEROED_ID       0x0000

#define OSAL_NV_PAGE_FREE      (OSAL_NV_PAGE_SIZE - Z_EXTADDR_LEN)

/* The last Flash page will reserve an IEEE addr block at the end of the page where the tools know
 * to program the IEEE.
 */
#define OSAL_NV_IEEE_OFFSET    (OSAL_NV_PAGE_SIZE - Z_EXTADDR_LEN)
#define OSAL_NV_IEEE_PAGE       63

// In case pages 0-1 are ever used, define a null page value.
#define OSAL_NV_PAGE_NULL       0

// In case item Id 0 is ever used, define a null item value.
#define OSAL_NV_ITEM_NULL       0

#define OSAL_NV_WORD_SIZE       4

#define OSAL_NV_PAGE_HDR_OFFSET 0

/*********************************************************************
 * MACROS
 */

#define OSAL_NV_PAGE_ERASE( pg ) \
  st( \
    FADDRH = (pg) << 1; \
    FCTL = 0x01; \
    asm("NOP");              \
    while(FCTL == 0x80);     \
  )

#define OSAL_NV_PAGE_TO_ADDR( pg )    ((uint32)pg << 11)
#define OSAL_NV_ADDR_TO_PAGE( addr )  ((uint8)(addr >> 11))

#define  OSAL_NV_CHECK_BUS_VOLTAGE  (HalAdcCheckVdd( HAL_ADC_VDD_LIMIT_4 ))

/*********************************************************************
 * TYPEDEFS
 */

typedef struct
{
  uint16 id;
  uint16 len;   // Enforce Flash-WORD size on len.
  uint16 chk;   // Byte-wise checksum of the 'len' data bytes of the item.
  uint16 stat;  // Item status.
} osalNvHdr_t;
// Struct member offsets.
#define OSAL_NV_HDR_ID    0
#define OSAL_NV_HDR_LEN   2
#define OSAL_NV_HDR_CHK   4
#define OSAL_NV_HDR_STAT  6

#define OSAL_NV_HDR_ITEM  2  // Length of any item of a header struct.
#define OSAL_NV_HDR_SIZE  8
#define OSAL_NV_HDR_HALF (OSAL_NV_HDR_SIZE / 2)

typedef struct
{
  uint16 active;
  uint16 inUse;
  uint16 xfer;
  uint16 spare;
} osalNvPgHdr_t;
// Struct member offsets.
#define OSAL_NV_PG_ACTIVE 0
#define OSAL_NV_PG_INUSE  2
#define OSAL_NV_PG_XFER   4
#define OSAL_NV_PG_SPARE  6

#define OSAL_NV_PAGE_HDR_SIZE  8
#define OSAL_NV_PAGE_HDR_HALF (OSAL_NV_PAGE_HDR_SIZE / 2)

typedef enum
{
  eNvXfer,
  eNvZero
} eNvHdrEnum;

typedef enum
{
  ePgActive,
  ePgInUse,
  ePgXfer,
  ePgSpare
} ePgHdrEnum;

/*********************************************************************
 * GLOBAL VARIABLES
 */

uint8 __xdata FBuff[4];  // Flash buffer for DMA transfer.

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

extern __near_func uint8 GetCodeByte(uint32);
extern __near_func void halFlashDmaTrigger(void);

extern bool HalAdcCheckVdd(uint8 limit);

/*********************************************************************
 * LOCAL VARIABLES
 */

// Offset into the page of the first available erased space.
static uint16 pgOff[OSAL_NV_PAGES_USED];

// Count of the bytes lost for the zeroed-out items.
static uint16 pgLost[OSAL_NV_PAGES_USED];

static uint8 pgRes;  // Page reserved for item compacting transfer.

/* It saves ~100 code bytes to move a uint8* parameter/return value from findItem()
 * to this local global.
 */
static uint8 findPg;

/* Immediately before the voltage critical operations of a page erase or
 * a word write, check bus voltage. If less than min, set global flag & abort.
 * Since this is to be done at the lowest level, many void functions would have to be changed to
 * return a value and code added to check that value before proceeding, resulting in a very
 * expensive code size hit for implementing this properly. Therefore, use this global as follows:
 * at the start of osal_nv_item_init/osal_nv_write, set to FALSE, and at the end, before returning,
 * check the value. Thus, the global is an accumulator of any error that occurred in any of the
 * attempts to modify Flash with a low bus voltage during the complicated sequence of events that
 * may occur on any item init or write. This is much more expedient and code saving than adding
 * return values and checking return values to early out. No matter which method is used, an NV
 * data record may end up mangled due to the low VCC conditions. The strategy is that the headers
 * and checksums will detect and allow recovery from such a condition.
 *
 * One unfortunate side-effect of using the global fail flag vice adding and checking return
 * values, is that setItem(), unaware that setting an item Id to zero has failed due to the low VCC
 * check, will still update the page Lost bytes counter. Having an artificially high lost byte
 * count makes it look like there are more bytes to recover from compacting a page than there may
 * actually be. An easy work-around it to invoke initNV() from osal_nv_item_init or osal_nv_write
 * anytime that the failF gets set - this will re-walk all of the pages and set the page offset
 * count and page lost bytes count to their actual values.
 */
static uint8 failF;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void   initDMA( void );
static void   execDMA( void );

static uint8  initNV( void );

static void   setPageUse( uint8 pg, uint8 inUse );
static uint16 initPage( uint8 pg, uint16 id );
static void   erasePage( uint8 pg );
static void   compactPage( uint8 pg );

static uint16 findItem( uint16 id );
static uint8  initItem( uint16 id, uint16 len, void *buf );
static uint8  initItem2( uint16 id, uint16 len, uint8 *comPg );
static void   setItem( uint8 pg, uint16 offset, eNvHdrEnum stat );

static uint16 calcChkB( uint16 len, uint8 *buf );
static uint16 calcChkF( byte pg, uint16 offset, uint16 len );

static void   readHdr( uint8 pg, uint16 offset, uint8 *buf );
static void   readWord( uint8 pg, uint16 offset, uint8 *buf );

static void   writeWord( uint8 pg, uint16 offset, uint8 *buf );
static void   writeWordD( uint8 pg, uint16 offset, uint8 *buf );
static void   writeWordH( uint8 pg, uint16 offset, uint8 *buf );
static void   writeBuf( uint8 pg, uint16 offset, uint16 len, uint8 *buf );
static void   xferBuf( uint8 srcPg, uint16 srcOff, uint8 dstPg, uint16 dstOff, uint16 len );

static uint8  writeItem( uint8 pg, uint16 id, uint16 len, void *buf );

/*********************************************************************
 * @fn      initDMA
 *
 * @brief   Initialize the DMA Channel for NV flash operations.
 *
 * @param   none
 *
 * @return  None
 */
static void initDMA( void )
{
  // Start address of the destination is the Flash Write Data register.
  OSAL_NV_DMA_CH->dstAddrH = 0xdf;
  OSAL_NV_DMA_CH->dstAddrL = 0xaf;
  OSAL_NV_DMA_CH->srcAddrH = (uint16)FBuff >> 8;
  OSAL_NV_DMA_CH->srcAddrL = (uint16)FBuff;

  // Using the length field to determine how many bytes to transfer.
  HAL_DMA_SET_VLEN( OSAL_NV_DMA_CH, HAL_DMA_VLEN_USE_LEN );

  // Transfer 4 bytes at a time.
  HAL_DMA_SET_LEN( OSAL_NV_DMA_CH, OSAL_NV_WORD_SIZE );

  // Transfer size is 1 byte.
  HAL_DMA_SET_WORD_SIZE( OSAL_NV_DMA_CH, HAL_DMA_WORDSIZE_BYTE );

  // After every 4-byte transfer, must await Flash write done indication.
  HAL_DMA_SET_TRIG_MODE( OSAL_NV_DMA_CH, HAL_DMA_TMODE_SINGLE );
  HAL_DMA_SET_TRIG_SRC( OSAL_NV_DMA_CH, HAL_DMA_TRIG_FLASH );

  // The source address is incremented by 1 byte after each transfer.
  HAL_DMA_SET_SRC_INC( OSAL_NV_DMA_CH, HAL_DMA_SRCINC_1 );

  // The destination address is constant - the Flash Write Data register.
  HAL_DMA_SET_DST_INC( OSAL_NV_DMA_CH, HAL_DMA_DSTINC_0 );

  // The DMA is to be polled and shall not issue an IRQ upon completion.
  HAL_DMA_SET_IRQ( OSAL_NV_DMA_CH, HAL_DMA_IRQMASK_DISABLE );

  // Xfer all 8 bits of a byte xfer.
  HAL_DMA_SET_M8( OSAL_NV_DMA_CH, HAL_DMA_M8_USE_8_BITS );

  // DMA memory access has highest priority.
  HAL_DMA_SET_PRIORITY( OSAL_NV_DMA_CH, HAL_DMA_PRI_HIGH );
}

/*********************************************************************
 * @fn      execDMA
 *
 * @brief   Arms and triggers a DMA write to Flash memory.
 *
 * @param   none
 *
 * @return  none
 */
static void execDMA( void )
{
  if ( !OSAL_NV_CHECK_BUS_VOLTAGE )
  {
    failF = TRUE;
    return;
  }

  HAL_DMA_CLEAR_IRQ( 0 );

  HAL_DMA_ARM_CH( 0 );

  halFlashDmaTrigger();

  while ( !(HAL_DMA_CHECK_IRQ( 0 )) );

  while ( FCTL & FWBUSY );
}

/*********************************************************************
 * @fn      initNV
 *
 * @brief   Initialize the NV flash pages.
 *
 * @param   none
 *
 * @return  TRUE if NV pages seem ok; FALSE otherwise.
 */
static uint8 initNV( void )
{
  osalNvPgHdr_t pgHdr, ieee;
  uint8 oldPg = OSAL_NV_PAGE_NULL;
  uint8 newPg = OSAL_NV_PAGE_NULL;
  uint8 xBad;
  uint8 pg;

  readHdr( OSAL_NV_IEEE_PAGE, OSAL_NV_IEEE_OFFSET, (uint8 *)(&ieee) );
  if ( (ieee.active == OSAL_NV_ERASED_ID) &&
       (ieee.inUse == OSAL_NV_ERASED_ID) &&
       (ieee.xfer == OSAL_NV_ERASED_ID) &&
       (ieee.spare == OSAL_NV_ERASED_ID) )
  {
    xBad = TRUE;
  }
  else
  {
    xBad = FALSE;
  }

  pgRes = OSAL_NV_PAGE_NULL;

  for ( pg = OSAL_NV_PAGE_BEG; pg <= OSAL_NV_PAGE_END; pg++ )
  {
#if OSAL_NV_CLEANUP
    OSAL_NV_PAGE_ERASE( pg );
    asm( "NOP" );
#endif

    readHdr( pg, OSAL_NV_PAGE_HDR_OFFSET, (uint8 *)(&pgHdr) );

    if ( pgHdr.active == OSAL_NV_ERASED_ID )
    {
      if ( pgRes == OSAL_NV_PAGE_NULL )
      {
        pgRes = pg;
      }
      else
      {
        setPageUse( pg, TRUE );
      }
    }
    else  // Page is active.
    {
      // If the page is not yet in use, it is the tgt of items from an xfer.
      if ( pgHdr.inUse == OSAL_NV_ERASED_ID )
      {
        newPg = pg;
      }
      // An Xfer from this page was in progress.
      else if ( pgHdr.xfer != OSAL_NV_ERASED_ID )
      {
        oldPg = pg;
      }
    }

    // Calculate page offset and lost bytes.
    initPage( pg, OSAL_NV_ITEM_NULL );

    readHdr( pg, OSAL_NV_IEEE_OFFSET, (uint8 *)(&pgHdr) );
    if ( xBad )
    {
      /* TBD - For the cost of more code space, the IEEE could be checksummed & then tested here
       * before installing to the erased IEEE on page 63.
       */
      if ( (pgHdr.active != OSAL_NV_ERASED_ID) ||
           (pgHdr.inUse != OSAL_NV_ERASED_ID) ||
           (pgHdr.xfer != OSAL_NV_ERASED_ID) ||
           (pgHdr.spare != OSAL_NV_ERASED_ID) )
      {
        writeWordD( OSAL_NV_IEEE_PAGE, OSAL_NV_IEEE_OFFSET, (uint8 *)(&pgHdr) );
        xBad = FALSE;
      }
    }
    else
    {
      if ( (pgHdr.active == OSAL_NV_ERASED_ID) &&
           (pgHdr.inUse == OSAL_NV_ERASED_ID) &&
           (pgHdr.xfer == OSAL_NV_ERASED_ID) &&
           (pgHdr.spare == OSAL_NV_ERASED_ID) )
      {
        writeWordD( pg, OSAL_NV_IEEE_OFFSET, (uint8 *)(&ieee) );
      }
    }
  }  // for ( pg = OSAL_NV_PAGE_BEG; pg <= OSAL_NV_PAGE_END; pg++ )

  /* First the old page is erased, and then the new page is put into use.
   * So if a transfer was in progress, the new page will always not yet be
   * marked as in use, since that is the last step to ending a transfer.
   */
  if ( newPg != OSAL_NV_PAGE_NULL )
  {
    /* If there is already a fallow page reserved, keep it and put the newPg in use.
     * An unfinished compaction will finish to the new reserve page and the old page
     * will be erased and reserved.
     */
    if ( pgRes != OSAL_NV_PAGE_NULL )
    {
      setPageUse( newPg, TRUE );
    }
    else
    {
      pgRes = newPg;
    }

    /* If a page compaction was interrupted and the page being compacted is not
     * yet erased, then there may be items remaining to xfer before erasing.
     */
    if ( oldPg != OSAL_NV_PAGE_NULL )
    {
      compactPage( oldPg );
    }
  }

  return (pgRes != OSAL_NV_PAGE_NULL);
}

/*********************************************************************
 * @fn      setPageUse
 *
 * @brief   Set page header active/inUse state according to 'inUse'.
 *
 * @param   pg - Valid NV page to verify and init.
 * @param   inUse - Boolean TRUE if inUse, FALSE if only active.
 *
 * @return  none
 */
static void setPageUse( uint8 pg, uint8 inUse )
{
  osalNvPgHdr_t pgHdr;

  pgHdr.active = OSAL_NV_ZEROED_ID;

  if ( inUse )
  {
    pgHdr.inUse = OSAL_NV_ZEROED_ID;
  }
  else
  {
    pgHdr.inUse = OSAL_NV_ERASED_ID;
  }

  writeWord( pg, OSAL_NV_PAGE_HDR_OFFSET, (uint8*)(&pgHdr) );
}

/*********************************************************************
 * @fn      initPage
 *
 * @brief   Walk the page items; calculate checksums, lost bytes & page offset.
 *
 * @param   pg - Valid NV page to verify and init.
 * @param   id - Valid NV item Id to use function as a "findItem".
 *               If set to NULL then just perform the page initialization.
 *
 * @return  If 'id' is non-NULL and good checksums are found, return the offset
 *          of the data corresponding to item Id; else OSAL_NV_ITEM_NULL.
 */
static uint16 initPage( uint8 pg, uint16 id )
{
  uint16 offset = OSAL_NV_PAGE_HDR_SIZE;
  uint16 sz, lost = 0;
  osalNvHdr_t hdr;

  do
  {
    readHdr( pg, offset, (uint8 *)(&hdr) );

    if ( hdr.id == OSAL_NV_ERASED_ID )
    {
      break;
    }
    offset += OSAL_NV_HDR_SIZE;
    sz = ((hdr.len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE;

    // A bad 'len' write has blown away the rest of the page.
    if ( (offset + sz) > OSAL_NV_PAGE_FREE )
    {
      lost += (OSAL_NV_PAGE_FREE - offset + OSAL_NV_HDR_SIZE);
      offset = OSAL_NV_PAGE_FREE;
      break;
    }

    if ( hdr.id != OSAL_NV_ZEROED_ID )
    {
      if ( hdr.chk == calcChkF( pg, offset, hdr.len ) )
      {
        /* This trick allows function to do double duty for findItem() without
         * compromising its essential functionality at powerup initialization.
         */
        if ( id != OSAL_NV_ITEM_NULL )
        {
          /* This trick allows asking to find the old/transferred item in case
           * of a successful new item write that gets interrupted before the
           * old item can be zeroed out.
           */
          if ( (id & 0x7fff) == hdr.id )
          {
            if ( (((id & 0x8000) == 0) && (hdr.stat == OSAL_NV_ERASED_ID)) ||
                 (((id & 0x8000) != 0) && (hdr.stat != OSAL_NV_ERASED_ID)) )
            {
              return offset;
            }
          }
        }
        // When invoked from the osal_nv_init(), find and zero any duplicates.
        else if ( hdr.stat == OSAL_NV_ERASED_ID )
        {
          /* The trick of setting the MSB of the item Id causes the logic
           * immediately above to return a valid page only if the header 'stat'
           * indicates that it was the older item being transferred.
           */
          uint16 off = findItem( (hdr.id | 0x8000) );

          if ( off != OSAL_NV_ITEM_NULL )
          {
            setItem( findPg, off, eNvZero );  // Mark old duplicate as invalid.
          }
        }
      }
      else
      {
        setItem( pg, offset, eNvZero );  // Mark bad checksum as invalid.
      }
    }

    if ( hdr.id == OSAL_NV_ZEROED_ID )
    {
      lost += (OSAL_NV_HDR_SIZE + sz);
    }
    offset += sz;

  } while ( TRUE );

  pgOff[pg - OSAL_NV_PAGE_BEG] = offset;
  pgLost[pg - OSAL_NV_PAGE_BEG] = lost;

  return OSAL_NV_ITEM_NULL;
}

/*********************************************************************
 * @fn      erasePage
 *
 * @brief   Erases a page in Flash.
 *
 * @param   pg - Valid NV page to erase.
 *
 * @return  none
 */
static void erasePage( uint8 pg )
{
  osalNvHdr_t ieee;

  if ( !OSAL_NV_CHECK_BUS_VOLTAGE )
  {
    failF = TRUE;
    return;
  }

  OSAL_NV_PAGE_ERASE( pg );

  pgOff[pg - OSAL_NV_PAGE_BEG] = OSAL_NV_PAGE_HDR_SIZE;
  pgLost[pg - OSAL_NV_PAGE_BEG] = 0;

  readHdr( OSAL_NV_IEEE_PAGE, OSAL_NV_IEEE_OFFSET, (uint8 *)(&ieee) );
  if ( (ieee.id != OSAL_NV_ERASED_ID) ||
       (ieee.len != OSAL_NV_ERASED_ID) ||
       (ieee.chk != OSAL_NV_ERASED_ID) ||
       (ieee.stat != OSAL_NV_ERASED_ID) )
  {
    writeWordD( pg, OSAL_NV_IEEE_OFFSET, (uint8 *)(&ieee) );
  }
}

/*********************************************************************
 * @fn      compactPage
 *
 * @brief   Compacts the page specified.
 *
 * @param   srcPg - Valid NV page to erase.
 *
 * @return  none
 */
static void compactPage( uint8 srcPg )
{
  uint16 dstOff = pgOff[pgRes-OSAL_NV_PAGE_BEG];
  uint16 srcOff = OSAL_NV_ZEROED_ID;
  osalNvHdr_t hdr;

  // Mark page as being in process of compaction.
  writeWordH( srcPg, OSAL_NV_PG_XFER, (uint8*)(&srcOff) );

  srcOff = OSAL_NV_PAGE_HDR_SIZE;

  do
  {
    uint16 sz;
    readHdr( srcPg, srcOff, (uint8 *)(&hdr) );

    if ( hdr.id == OSAL_NV_ERASED_ID )
    {
      break;
    }

    srcOff += OSAL_NV_HDR_SIZE;

    if ( (srcOff + hdr.len) > OSAL_NV_PAGE_FREE )
    {
      break;
    }

    sz = ((hdr.len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE;

    if ( hdr.id != OSAL_NV_ZEROED_ID )
    {
      if ( hdr.chk == calcChkF( srcPg, srcOff, hdr.len ) )
      {
        setItem( srcPg, srcOff, eNvXfer );
        writeBuf( pgRes, dstOff, OSAL_NV_HDR_SIZE, (byte *)(&hdr) );
        dstOff += OSAL_NV_HDR_SIZE;
        xferBuf( srcPg, srcOff, pgRes, dstOff, sz );
        dstOff += sz;
      }

      setItem( srcPg, srcOff, eNvZero );  // Mark old location as invalid.
    }

    srcOff += sz;

  } while ( TRUE );

  pgOff[pgRes-OSAL_NV_PAGE_BEG] = dstOff;

  /* In order to recover from a page compaction that is interrupted,
   * the logic in osal_nv_init() depends upon the following order:
   * 1. Compacted page is erased.
   * 2. State of the target of compaction is changed ePgActive to ePgInUse.
   */
  erasePage( srcPg );

  // Mark the reserve page as being in use.
  setPageUse( pgRes, TRUE );

  // Mark newly erased page as the new reserve page.
  pgRes = srcPg;
}

/*********************************************************************
 * @fn      findItem
 *
 * @brief   Find an item Id in NV and return the page and offset to its data.
 *
 * @param   id - Valid NV item Id.
 *
 * @return  Offset of data corresponding to item Id, if found;
 *          otherwise OSAL_NV_ITEM_NULL.
 *
 *          The page containing the item, if found;
 *          otherwise no valid assignment made - left equal to item Id.
 *
 */
static uint16 findItem( uint16 id )
{
  uint16 off;
  uint8 pg;

  for ( pg = OSAL_NV_PAGE_BEG; pg <= OSAL_NV_PAGE_END; pg++ )
  {
    if ( (off = initPage( pg, id )) != OSAL_NV_ITEM_NULL )
    {
      findPg = pg;
      return off;
    }
  }

  // Now attempt to find the item as the "old" item of a failed/interrupted NV write.
  for ( pg = OSAL_NV_PAGE_BEG; pg <= OSAL_NV_PAGE_END; pg++ )
  {
    if ( (off = initPage( pg, (id | 0x8000) )) != OSAL_NV_ITEM_NULL )
    {
      findPg = pg;
      return off;
    }
  }

  findPg = OSAL_NV_PAGE_NULL;
  return OSAL_NV_ITEM_NULL;
}

/*********************************************************************
 * @fn      initItem
 *
 * @brief   An NV item is created and initialized with the data passed to the function, if any.
 *
 * @param   id  - Valid NV item Id.
 * @param   len - Item data length.
 * @param  *buf - Pointer to item initalization data. Set to NULL if none.
 *
 * @return  TRUE if item write and read back checksums ok; FALSE otherwise.
 */
static uint8 initItem( uint16 id, uint16 len, void *buf )
{
  uint16 sz = ((len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE +
                                                                    OSAL_NV_HDR_SIZE;
  uint8 pg = OSAL_NV_PAGE_BEG;
  uint8 rtrn = FALSE;
  uint8 idx;

  for ( idx = 0; idx < OSAL_NV_PAGES_USED; idx++, pg++ )
  {
    if ( pg == pgRes )
    {
      continue;
    }
    if ( (pgOff[idx] - pgLost[idx] + sz) <= OSAL_NV_PAGE_FREE )
    {
      break;
    }
  }

  if ( idx != OSAL_NV_PAGES_USED )
  {
    // Item fits if an old page is compacted.
    if ( (pgOff[idx] + sz) > OSAL_NV_PAGE_FREE )
    {
      pg = pgRes;
    }

    // New item is the first one written to the reserved page, then the old page is compacted.
    if ( writeItem( pg, id, len, buf ) )
    {
      rtrn = TRUE;
    }

    if ( pg == pgRes )
    {
      compactPage( OSAL_NV_PAGE_BEG+idx );
    }
  }

  return rtrn;
}

/*********************************************************************
 * @fn      initItem2
 *
 * @brief   An NV item is created.
 *
 * @param   id  - Valid NV item Id.
 * @param   len - Item data length.
 *
 * @return  TRUE if item write and read back checksums ok; FALSE otherwise.
 *          If return it TRUE, then findPg is set to OSAL_NV_PAGE_NULL if a page compaction is not
 *          required; otherwise it is set to the non-NULL page that must be compacted.
 */
static uint8 initItem2( uint16 id, uint16 len, uint8 *comPg )
{
  uint16 sz = ((len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE +
                                                                    OSAL_NV_HDR_SIZE;
  uint8 pg = OSAL_NV_PAGE_BEG;
  uint8 idx;

  for ( idx = 0; idx < OSAL_NV_PAGES_USED; idx++, pg++ )
  {
    if ( pg == pgRes )
    {
      continue;
    }
    if ( (pgOff[idx] - pgLost[idx] + sz) <= OSAL_NV_PAGE_FREE )
    {
      break;
    }
  }

  // Item fits if an old page is compacted.
  if ( (idx == OSAL_NV_PAGES_USED) || ((pgOff[idx] + sz) > OSAL_NV_PAGE_FREE) )
  {
    pg = pgRes;

    if ( idx != OSAL_NV_PAGES_USED )
    {
      *comPg = OSAL_NV_PAGE_BEG+idx;
    }
    else
    {
      // comPg has already been set to the page containing the item, so compact that one.
    }
  }

  if ( writeItem( pg, id, len, NULL ) )
  {
    return pg;
  }
  else
  {
    return OSAL_NV_PAGE_NULL;
  }
}

/*********************************************************************
 * @fn      setItem
 *
 * @brief   Set an item Id or status to mark its state.
 *
 * @param   pg - Valid NV page.
 * @param   offset - Valid offset into the page of the item data - the header
 *                   offset is calculated from this.
 * @param   stat - Valid enum value for the item status.
 *
 * @return  none
 */
static void setItem( uint8 pg, uint16 offset, eNvHdrEnum stat )
{
  osalNvHdr_t hdr;

  offset -= OSAL_NV_HDR_SIZE;
  readHdr( pg, offset, (uint8 *)(&hdr) );

  if ( stat == eNvXfer )
  {
    hdr.stat = OSAL_NV_ACTIVE;
    writeWord( pg, offset+OSAL_NV_HDR_CHK, (uint8*)(&(hdr.chk)) );
  }
  else // if ( stat == eNvZero )
  {
    uint16 sz = ((hdr.len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE +
                                                                          OSAL_NV_HDR_SIZE;
    hdr.id = 0;
    writeWord( pg, offset, (uint8 *)(&hdr) );
    pgLost[pg-OSAL_NV_PAGE_BEG] += sz;
  }
}

/*********************************************************************
 * @fn      calcChkB
 *
 * @brief   Calculates the data checksum over the 'buf' parameter.
 *
 * @param   pg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   len - Byte count of the data to be checksummed.
 *
 * @return  Calculated checksum of the data bytes.
 */
static uint16 calcChkB( uint16 len, uint8 *buf )
{
  uint16 chk = 0;

  while ( len-- )
  {
    chk += *buf++;
  }

  return chk;
}

/*********************************************************************
 * @fn      calcChkF
 *
 * @brief   Calculates the data checksum by reading the data bytes from NV.
 *
 * @param   pg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   len - Byte count of the data to be checksummed.
 *
 * @return  Calculated checksum of the data bytes.
 */
static uint16 calcChkF( byte pg, uint16 offset, uint16 len )
{
  uint32 addr = OSAL_NV_PAGE_TO_ADDR( pg ) + offset;
  uint16 chk = 0;
  uint8 eFlag = TRUE;

  while ( len-- )
  {
    uint8 ch = GetCodeByte( addr++ );

    if ( ch != OSAL_NV_ERASED )
    {
      eFlag = FALSE;
    }

    chk += ch;
  }

  if ( eFlag )
  {
    return OSAL_NV_ERASED_ID;
  }
  else
  {
    return chk;
  }
}

/*********************************************************************
 * @fn      readHdr
 *
 * @brief   Reads "sizeof( osalNvHdr_t )" bytes from NV.
 *
 * @param   pg - Valid NV page.
 * @param   offset - Valid offset into the page.
 * @param   buf - Valid buffer space of at least sizeof( osalNvHdr_t ) bytes.
 *
 * @return  none
 */
static void readHdr( uint8 pg, uint16 offset, uint8 *buf )
{
  uint32 addr = OSAL_NV_PAGE_TO_ADDR( pg ) + offset;
  uint8 len = OSAL_NV_HDR_SIZE;

  do
  {
    *buf++ = GetCodeByte( addr++ );
  } while ( --len );
}

/*********************************************************************
 * @fn      readWord
 *
 * @brief   Reads "sizeof( osalNvHdr_t )" bytes from NV.
 *
 * @param   pg - Valid NV page.
 * @param   offset - Valid offset into the page.
 * @param   buf - Valid buffer space of at least sizeof( osalNvHdr_t ) bytes.
 *
 * @return  none
 */
static void readWord( uint8 pg, uint16 offset, uint8 *buf )
{
  uint32 addr = OSAL_NV_PAGE_TO_ADDR( pg ) + offset;
  uint8 len = OSAL_NV_WORD_SIZE;

  do
  {
    *buf++ = GetCodeByte( addr++ );
  } while ( --len );
}


/*********************************************************************
 * @fn      writeWord
 *
 * @brief   Writes a Flash-WORD to NV.
 *
 * @param   pg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   buf - Pointer to source buffer.
 *
 * @return  none
 */
static void writeWord( uint8 pg, uint16 offset, uint8 *buf )
{
  if ( (buf[0] != OSAL_NV_ERASED) || (buf[1] != OSAL_NV_ERASED) ||
       (buf[2] != OSAL_NV_ERASED) || (buf[3] != OSAL_NV_ERASED) )
  {
    offset = (offset >> 2) + ((uint16)pg << 9);

    FADDRL = (uint8)offset;
    FADDRH = (uint8)(offset >> 8);

    FBuff[0] = buf[0];
    FBuff[1] = buf[1];
    FBuff[2] = buf[2];
    FBuff[3] = buf[3];

    execDMA();
  }
}

/*********************************************************************
 * @fn      writeWordD
 *
 * @brief   Writes two Flash-WORDs to NV.
 *
 * @param   pg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   buf - Pointer to source buffer.
 *
 * @return  none
 */
static void writeWordD( uint8 pg, uint16 offset, uint8 *buf )
{
  writeWord( pg, offset, buf );
  writeWord( pg, offset+OSAL_NV_WORD_SIZE, buf+OSAL_NV_WORD_SIZE);
}

/*********************************************************************
 * @fn      writeWordH
 *
 * @brief   Writes the 1st half of a Flash-WORD to NV (filling 2nd half with 0xffff).
 *
 * @param   pg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   buf - Pointer to source buffer.
 *
 * @return  none
 */
static void writeWordH( uint8 pg, uint16 offset, uint8 *buf )
{
  uint8 tmp[4];

  tmp[0] = buf[0];
  tmp[1] = buf[1];
  tmp[2] = OSAL_NV_ERASED;
  tmp[3] = OSAL_NV_ERASED;

  writeWord( pg, offset, tmp );
}

/*********************************************************************
 * @fn      writeBuf
 *
 * @brief   Writes a data buffer to NV.
 *
 * @param   dstPg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   len  - Byte count of the data to write.
 * @param   buf  - The data to write.
 *
 * @return  TRUE if data buf checksum matches read back checksum, else FALSE.
 */
static void writeBuf( uint8 dstPg, uint16 dstOff, uint16 len, uint8 *buf )
{
  uint32 addr;
  uint8 idx, rem, tmp[OSAL_NV_WORD_SIZE];

  rem = dstOff % OSAL_NV_WORD_SIZE;
  if ( rem )
  {
    dstOff -= rem;
    addr = OSAL_NV_PAGE_TO_ADDR( dstPg ) + dstOff;

    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = GetCodeByte( addr++ );
    }

    while ( (idx < OSAL_NV_WORD_SIZE) && len )
    {
      tmp[idx++] = *buf++;
      len--;
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    writeWord( dstPg, dstOff, tmp );
    dstOff += OSAL_NV_WORD_SIZE;
  }

  rem = len % OSAL_NV_WORD_SIZE;
  len /= OSAL_NV_WORD_SIZE;

  while ( len-- )
  {
    writeWord( dstPg, dstOff, buf );
    dstOff += OSAL_NV_WORD_SIZE;
    buf += OSAL_NV_WORD_SIZE;
  }

  if ( rem )
  {
    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = *buf++;
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    writeWord( dstPg, dstOff, tmp );
  }
}

/*********************************************************************
 * @fn      xferBuf
 *
 * @brief   Xfers an NV buffer from one location to another, enforcing OSAL_NV_WORD_SIZE writes.
 *
 * @return  none
 */
static void xferBuf( uint8 srcPg, uint16 srcOff, uint8 dstPg, uint16 dstOff, uint16 len )
{
  uint32 addr;
  uint8 idx, rem, tmp[OSAL_NV_WORD_SIZE];

  rem = dstOff % OSAL_NV_WORD_SIZE;
  if ( rem )
  {
    dstOff -= rem;
    addr = OSAL_NV_PAGE_TO_ADDR( dstPg ) + dstOff;

    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = GetCodeByte( addr++ );
    }

    addr = OSAL_NV_PAGE_TO_ADDR( srcPg ) + srcOff;

    while ( (idx < OSAL_NV_WORD_SIZE) && len )
    {
      tmp[idx++] = GetCodeByte( addr++ );
      srcOff++;
      len--;
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    writeWord( dstPg, dstOff, tmp );
    dstOff += OSAL_NV_WORD_SIZE;
  }

  rem = len % OSAL_NV_WORD_SIZE;
  len = len / OSAL_NV_WORD_SIZE;

  while ( len-- )
  {
    readWord( srcPg, srcOff, tmp );
    srcOff += OSAL_NV_WORD_SIZE;
    writeWord( dstPg, dstOff, tmp );
    dstOff += OSAL_NV_WORD_SIZE;
  }

  if ( rem )
  {
    addr = OSAL_NV_PAGE_TO_ADDR( srcPg ) + srcOff;

    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = GetCodeByte( addr++ );
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    writeWord( dstPg, dstOff, tmp );
  }
}

/*********************************************************************
 * @fn      writeItem
 *
 * @brief   Writes an item header/data combo to the specified NV page.
 *
 * @param   pg - Valid NV Flash page.
 * @param   id - Valid NV item Id.
 * @param   len  - Byte count of the data to write.
 * @param   buf  - The data to write. If NULL, no data/checksum write.
 *
 * @return  TRUE if header/data to write matches header/data read back, else FALSE.
 */
static uint8 writeItem( uint8 pg, uint16 id, uint16 len, void *buf )
{
  uint16 offset = pgOff[pg-OSAL_NV_PAGE_BEG];
  uint8 rtrn = FALSE;
  osalNvHdr_t hdr;
  uint16 sz;

  if ( pg == pgRes )
  {
    /* Mark reserve page as being active, in process of receiving items.
     * Invoking function must effect a page compaction.
     */
    setPageUse( pg, FALSE );
  }

  hdr.id = id;
  hdr.len = len;

  writeWord( pg, offset, (uint8 *)&hdr );
  readHdr( pg, offset, (uint8 *)(&hdr) );

  if ( (hdr.id == id) && (hdr.len == len) )
  {
    if ( buf != NULL )
    {
      uint16 chk = calcChkB( len, buf );

      offset += OSAL_NV_HDR_SIZE;
      writeBuf( pg, offset, len, buf );

      if ( chk == calcChkF( pg, offset, len ) )
      {
        writeWordH( pg, (offset-OSAL_NV_WORD_SIZE), (uint8 *)&chk );
        readHdr( pg, (offset-OSAL_NV_HDR_SIZE), (uint8 *)(&hdr) );

        if ( chk == hdr.chk )
        {
          rtrn = TRUE;
        }
      }
    }
    else
    {
      rtrn = TRUE;
    }
  }

  sz = ((len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE +
                                                             OSAL_NV_HDR_SIZE;
  pgOff[pg-OSAL_NV_PAGE_BEG] += sz;

  return rtrn;
}

/*********************************************************************
 * @fn      osal_nv_init
 *
 * @brief   Initialize NV service.
 *
 * @param   p - Not used.
 *
 * @return  none
 */
void osal_nv_init( void *p )
{
  (void)p;  // Suppress Lint warning.

  // Set Flash write timing based on CPU speed.
#ifdef CPU16MHZ
  FWT = 0x15;
#else
  FWT = 0x2A;
#endif

  initDMA();

  (void)initNV();  // Always returns TRUE after pages have been erased.
}

/*********************************************************************
 * @fn      osal_nv_item_init
 *
 * @brief   If the NV item does not already exist, it is created and
 *          initialized with the data passed to the function, if any.
 *          This function must be called before calling osal_nv_read() or
 *          osal_nv_write().
 *
 * @param   id  - Valid NV item Id.
 * @param   len - Item length.
 * @param  *buf - Pointer to item initalization data. Set to NULL if none.
 *
 * @return  NV_ITEM_UNINIT - Id did not exist and was created successfully.
 *          ZSUCCESS       - Id already existed, no action taken.
 *          NV_OPER_FAILED - Failure to find or create Id.
 */
uint8 osal_nv_item_init( uint16 id, uint16 len, void *buf )
{
  failF = FALSE;

  /* ZCD_NV_EXTADDR is the only item maintained without an osalNvHdr_t,
   * so it is always already initialized.
   */
  if ( (id == ZCD_NV_EXTADDR) || (findItem( id ) != OSAL_NV_ITEM_NULL) )
  {
    return ZSUCCESS;
  }
  else if ( initItem( id, len, buf ) != OSAL_NV_PAGE_NULL )
  {
    if ( failF )
    {
      (void)initNV();  // See comment at the declaration of failF.
      return NV_OPER_FAILED;
    }
    else
    {
      return NV_ITEM_UNINIT;
    }
  }
  else
  {
    return NV_OPER_FAILED;
  }
}

/*********************************************************************
 * @fn      osal_nv_item_len
 *
 * @brief   Get the data length of the item stored in NV memory.
 *
 * @param   id  - Valid NV item Id.
 *
 * @return  Item length, if found; zero otherwise.
 */
uint16 osal_nv_item_len( uint16 id )
{
  if ( id == ZCD_NV_EXTADDR )
  {
    return Z_EXTADDR_LEN;
  }
  else
  {
    uint16 offset = findItem( id );

    if ( offset == OSAL_NV_ITEM_NULL )
    {
      return 0;
    }
    else
    {
      osalNvHdr_t hdr;
      readHdr( findPg, (offset - OSAL_NV_HDR_SIZE), (uint8 *)(&hdr) );
      return hdr.len;
    }
  }
}

/*********************************************************************
 * @fn      osal_nv_write
 *
 * @brief   Write a data item to NV. Function can write an entire item to NV or
 *          an element of an item by indexing into the item with an offset.
 *
 * @param   id  - Valid NV item Id.
 * @param   ndx - Index offset into item
 * @param   len - Length of data to write.
 * @param  *buf - Data to write.
 *
 * @return  ZSUCCESS if successful, NV_ITEM_UNINIT if item did not
 *          exist in NV and offset is non-zero, NV_OPER_FAILED if failure.
 */
uint8 osal_nv_write( uint16 id, uint16 ndx, uint16 len, void *buf )
{
  uint8 rtrn = ZSUCCESS;

  /* Global fail flag for fail due to low bus voltage has less impact on code
   * size than passing back a return value all the way from the lowest level.
   */
  failF = FALSE;

  if ( id == ZCD_NV_EXTADDR )
  {
    osalNvHdr_t hdr;
    readHdr( OSAL_NV_IEEE_PAGE, OSAL_NV_IEEE_OFFSET, (uint8 *)(&hdr) );

    if ( (hdr.id == OSAL_NV_ERASED_ID) &&
         (hdr.len == OSAL_NV_ERASED_ID) &&
         (hdr.chk == OSAL_NV_ERASED_ID) &&
         (hdr.stat == OSAL_NV_ERASED_ID) )
    {
      writeWordD( OSAL_NV_IEEE_PAGE, OSAL_NV_IEEE_OFFSET, buf );
      return ((failF) ? NV_OPER_FAILED : ZSUCCESS);
    }
    else
    {
      return NV_OPER_FAILED;
    }
  }

  if ( len != 0 )
  {
    osalNvHdr_t hdr;
    uint32 addr;
    uint16 srcOff;
    uint16 cnt;
    uint8 *ptr;

    srcOff = findItem( id );
    if ( srcOff == OSAL_NV_ITEM_NULL )
    {
      return NV_ITEM_UNINIT;
    }

    readHdr( findPg, (srcOff - OSAL_NV_HDR_SIZE), (uint8 *)(&hdr) );
    if ( hdr.len < (ndx + len) )
    {
      return NV_OPER_FAILED;
    }

    addr = OSAL_NV_PAGE_TO_ADDR( findPg ) + srcOff + ndx;
    ptr = buf;
    cnt = len;
    do
    {
      if ( GetCodeByte( addr++ ) != *ptr++ )
      {
        break;
      }
    } while ( --cnt );

    if ( cnt != 0 )  // If the buffer to write is different in one or more bytes.
    {
      uint8 comPg, srcPg;

      comPg = srcPg = findPg;
      // initItem2() can change the findPg and it advances pgOff[] of the dstPg.
      uint8 dstPg = initItem2( id, hdr.len, &comPg );

      if ( dstPg != OSAL_NV_PAGE_NULL )
      {
        uint16 tmp = ((hdr.len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE;
        uint16 dstOff = pgOff[dstPg-OSAL_NV_PAGE_BEG] - tmp;
        uint16 origOff = srcOff;

        setItem( srcPg, srcOff, eNvXfer );

        xferBuf( srcPg, srcOff, dstPg, dstOff, ndx );
        srcOff += ndx;
        dstOff += ndx;

        writeBuf( dstPg, dstOff, len, buf );
        srcOff += len;
        dstOff += len;

        xferBuf( srcPg, srcOff, dstPg, dstOff, (hdr.len-ndx-len) );

        // Calculate and write the new checksum.
        dstOff = pgOff[dstPg-OSAL_NV_PAGE_BEG] - tmp;
        tmp = calcChkF( dstPg, dstOff, hdr.len );
        dstOff -= OSAL_NV_HDR_SIZE;
        writeWordH( dstPg, (dstOff+OSAL_NV_HDR_CHK), (uint8 *)&tmp );
        readHdr( dstPg, dstOff, (uint8 *)(&hdr) );

        if ( tmp == hdr.chk )
        {
          setItem( srcPg, origOff, eNvZero );
        }
        else
        {
          rtrn = NV_OPER_FAILED;
        }

        if ( dstPg == pgRes )
        {
          compactPage( comPg );
        }
      }
      else
      {
        rtrn = NV_OPER_FAILED;
      }
    }
  }

  if ( failF )
  {
    (void)initNV();  // See comment at the declaration of failF.
    rtrn = NV_OPER_FAILED;
  }

  return rtrn;
}

/*********************************************************************
 * @fn      osal_nv_read
 *
 * @brief   Read data from NV.  This function can be used to read an
 *          entire item from NV or an element of an item by indexing
 *          into the item with an offset.  Read data is copied into
 *          *buf.
 *
 * @param   id     - Valid NV item Id.
 *
 * @param   ndx - Index offset into item
 *
 * @param   len    - Length of data to read.
 *
 * @param   *buf  - Data is read into this buffer.
 *
 * @return  ZSUCCESS if NV data was copied to the parameter 'buf'.
 *          Otherwise, NV_OPER_FAILED for failure.
 */
uint8 osal_nv_read( uint16 id, uint16 ndx, uint16 len, void *buf )
{
  uint32 addr;
  uint16 offset;
  uint8 *ptr = (uint8 *)buf;

  if ( id == ZCD_NV_EXTADDR )
  {
    offset = OSAL_NV_IEEE_OFFSET;
    findPg = OSAL_NV_IEEE_PAGE;
  }
  else
  {
    offset = findItem( id );
  }

  if ( offset == OSAL_NV_ITEM_NULL )
  {
    return NV_OPER_FAILED;
  }

  addr = OSAL_NV_PAGE_TO_ADDR( findPg ) + offset + ndx;
  while ( len-- )
  {
    *ptr++ = GetCodeByte( addr++ );
  }

  return ZSUCCESS;
}

/*********************************************************************
*********************************************************************/
