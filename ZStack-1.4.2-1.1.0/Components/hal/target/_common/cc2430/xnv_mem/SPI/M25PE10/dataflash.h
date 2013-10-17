/***************** Header File for STFL-I based Serial Flash Memory Driver *****

   Filename:    C2195.h
   Description: Header file for C2195.c
                Consult also the C file for more details.

                Please note that some necessary changes are made to account for the
                SPI-specific communication property. So this specification slightly differs from
                the STFL-I Specification designed for parallel NOR Flash memories.
		The major differences from the STFL-I SOFTWARE DRIVER INTERFACE
		SPECIFICATION (Specification-STFL-I-V2-1a) are listed below:
                - Flash Configuration Selection is not used.
                - BASE_ADDR is not used.
                - InstructionType enumerations are re-formulated to use SPI Flash instructions.
                - CONFIGURATION CONSTANTS are fixed, with #define INS(A) not used.
                ...

   Version:     1.1
   Date:        10-17-2005
   Authors:     Tan Zhi, Da Gang Zhou STMicroelectronics, Shanghai (China)
   Copyright (c) 2005 STMicroelectronics.

   THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH
   CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A
   RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
   CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT OF SUCH
   SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN
   IN CONNECTION WITH THEIR PRODUCTS.
********************************************************************************

   Version History.
   Ver.  Date         Comments

   1.0   07/07/2005   M25PE10, M25PE20 and M25PE40 support
   1.1   10/17/2005   Add M25PE80 support
*******************************************************************************/


/*************** User Change Area *******************************************

   The purpose of this section is to give all the support necessary to customize the
   SW Drivers according to the requirements of the hardware and Flash memory configuration.
   It is possible to choose the Flash memory start address, CPU Bit depth, number of Flash
   chips, hardware configuration and performance data (TimeOut Info).

   Note that data Bytes will be referred to as elements throughout the document unless otherwise specified.

   The options are listed and explained below:

   ********* Data Types *********
   The source code defines hardware independent datatypes assuming that the
   compiler implements the numerical types as follows:

   unsigned char    8 bits (defined as ST_uint8)
   char             8 bits (defined as ST_sint8)
   unsigned short   16 bits (defined as ST_uint16)
   short            16 bits (defined as ST_sint16)
   unsigned long    32 bits (defined as ST_uint32)
   long             32 bits (defined as ST_sint32)

   In case the compiler does not support the currently used numerical types,
   they can be easily changed once (only once) here in the user area of the header file.
   The data types are consequently referenced in the source code as (u)ST_sint8,
   (u)ST_sint16 and (u)ST_sint32. No other data types like 'CHAR','SHORT','INT','LONG'
   are directly used in the code.


   ********* Flash Type *********
   This driver supports the following Serial Flash memory Types

   M25PE10	1Mb Page-Erasable Serial Flash Memory	    #define USE_M25PE10
   M25PE20	2Mb Page-Erasable Serial Flash Memory	    #define USE_M25PE20
   M25PE40	4Mb Page-Erasable Serial Flash Memory	    #define USE_M25PE40
   M25PE80	8Mb Page-Erasable Serial Flash Memory	    #define USE_M25PE80

   ********* TimeOut *********
   There are timeouts implemented in the loops of the code, in order
   to enable a timeout detection for operations that would otherwise never terminate.
   There are two possibilities:

   1) The ANSI Library functions declared in 'time.h' exist

      If the current compiler supports 'time.h' the define statement
      TIME_H_EXISTS should be activated. This makes sure that
      the performance of the current evaluation HW does not change
      the timeout settings.

   2) or they are not available (COUNT_FOR_A_SECOND)

      If the current compiler does not support 'time.h', the define
      statement cannot be used. In this case the COUNT_FOR_A_SECOND
      value has to be defined so as to create a one-second delay.
      For example, if 100000 repetitions of a loop are
      needed to give a time delay of one second, then
      COUNT_FOR_A_SECOND should have the value 100000.

      Note: This delay is HW (Performance) dependent and needs,
      therefore, to be updated with every new HW.

      This driver has been tested with a certain configuration and other
      target platforms may have other performance data. The value may
      therefore have to be changed.

      It is up to the user to implement this value to prevent the code
      from timing out too early and allow correct completion of the device
      operations.


   ********* Additional Routines *********
   The drivers also provide a subroutine which displays the full
   error message instead of just an error number.

   The define statement VERBOSE activates additional Routines.
   Currently it activates the function FlashErrorStr()

   No further changes should be necessary.

*****************************************************************************/

#ifndef __DATAFLASH__H__
#define __DATAFLASH__H__

#define DRIVER_VERSION_MAJOR 1
#define DRIVER_VERSION_MINOR 0

#if 1
typedef unsigned char  ST_uint8; /* All HW dependent Basic Data Types */
typedef          char  ST_sint8;
typedef unsigned short ST_uint16;
typedef          short ST_sint16;
typedef unsigned long  ST_uint32;
typedef          long  ST_sint32;
#endif

/* With SYNCHRONOUS_IO defined, each function that sends an instruction (e.g. PE)
   will not return until the Flash memory finishes executing the instruction
   or a pre-set timeout occurs. the pre-set timeout value is in
   accordance with the datasheet of each memory.

   To achieve Send-n-Forget feature, comment out this #define*/
#define SYNCHRONOUS_IO

#define USE_M25PE10
/* Possible Values:	USE_M25PE10
					USE_M25PE20
					USE_M25PE40
					USE_M25PE80
                    */

/*#define TIME_H_EXISTS*/  /* set this macro if C-library "time.h" is supported */
/* Possible Values: TIME_H_EXISTS
                    - no define - TIME_H_EXISTS */

#ifndef TIME_H_EXISTS
  #define COUNT_FOR_A_SECOND 432666    /* Timer Usage */
#endif

#define xVERBOSE /* Activates additional Routines */
/* Currently the Error String Definition */

/********************** End of User Change Area *****************************/

/*******************************************************************************
     DERIVED DATATYPES
*******************************************************************************/
/******** InstructionCode ********/
enum
{
	//instruction set
	SPI_FLASH_INS_WREN      = 0x06,		// Write Enable
	SPI_FLASH_INS_WRDI      = 0x04,		// Write Disable
	SPI_FLASH_INS_RDSR	    = 0x05,		// Read Status Register
	SPI_FLASH_INS_WRSR      = 0x01,		// write status register
	SPI_FLASH_INS_READ	    = 0x03,		// Read Data Bytes
	SPI_FLASH_INS_FAST_READ = 0x0B,		// Read Data Bytes at Higher Speed
	SPI_FLASH_INS_PP	      = 0x02,		// Page Program
	SPI_FLASH_INS_SE	      = 0xD8,		// Sector Erase
	SPI_FLASH_INS_BE        = 0xC7,     // Bulk Erase
	SPI_FLASH_INS_DP	      = 0xB9,		// Deep Power-down
	SPI_FLASH_INS_RDID	    = 0x9F,		// Read Identification
	SPI_FLASH_INS_PW	      = 0x0A,		// Page Write
	SPI_FLASH_INS_PE	      = 0xDB,		// Page Erase
	SPI_FLASH_INS_WRLR      = 0xE5,     // Write to Lock Register	
	SPI_FLASH_INS_RDLR      = 0xE8,     // Read Lock Register
	SPI_FLASH_INS_RDP	      = 0xAB,		// Release from Deep Power-down
	SPI_FLASH_INS_DUMMY	    = 0xAA		// dummy byte to check Deep Power-down.
};


/******** InstructionType ********/

typedef enum {
    WriteEnable,
    WriteDisable,
    ReadDeviceIdentification,
    ReadManufacturerIdentification,
    ReadStatusRegister,
    Read,
    FastRead,
    PageWrite,
    PageProgram,
    PageErase,
    SectorErase,
    BulkErase,
    DeepPowerDown,
    ReleaseFromDeepPowerDown,
    Write,
    Program,
    ReadLockRegister,
    WriteLockRegister,
} InstructionType;

/******** ReturnType ********/

typedef enum {
    Flash_AddressInvalid,
    Flash_MemoryOverflow,
    Flash_PageEraseFailed,
    Flash_PageNrInvalid,
    Flash_SectorNrInvalid,
    Flash_FunctionNotSupported,
    Flash_NoInformationAvailable,
    Flash_OperationOngoing,
    Flash_OperationTimeOut,
    Flash_ProgramFailed,
    Flash_WrongType,
    Flash_Success
} ReturnType;

/******** SectorType ********/

typedef ST_uint16 uSectorType;

/******** PageType ********/

typedef ST_uint16 uPageType;

/******** AddrType ********/

typedef ST_uint32 uAddrType;

/******** ParameterType ********/

typedef union {

    /**** WriteEnable has no parameters ****/

    /**** WriteDisable has no parameters ****/

    /**** FlashDeepPowerDown has no parameters ****/

    /**** FlashReleaseFromDeepPowerDown has no parameters ****/

    /**** FlashBulkErase has no parameters ****/

    /**** ReadDeviceIdentification Parameters ****/
    struct {
      ST_uint16 ucDeviceIdentification;
    } ReadDeviceIdentification;

    /**** ReadManufacturerIdentification Parameters ****/
    struct {
      ST_uint8 ucManufacturerIdentification;
    } ReadManufacturerIdentification;

    /**** ReadStatusRegister Parameters ****/
    struct {
      ST_uint8 ucStatusRegister;
    } ReadStatusRegister;

    /**** Read Parameters ****/
    struct {
      uAddrType udAddr;
      ST_uint32 udNrOfElementsToRead;
      void *pArray;
    } Read;

    /**** FastRead Parameters ****/
    struct {
      uAddrType udAddr;
      ST_uint32 udNrOfElementsToRead;
      void *pArray;
    } FastRead;

    /**** PageWrite Parameters ****/
    struct {
      uAddrType udAddr;
      ST_uint32 udNrOfElementsInArray;
      void *pArray;
    } PageWrite;

    /**** PageProgram Parameters ****/
    struct {
      uAddrType udAddr;
      ST_uint32 udNrOfElementsInArray;
      void *pArray;
    } PageProgram;

    /**** PageErase Parameters ****/
    struct {
      uPageType upgPageNr;
    } PageErase;

    /**** SectorErase Parameters ****/
    struct {
      uSectorType ustSectorNr;
    } SectorErase;

    /**** Write Parameters ****/
    struct {
      uAddrType udAddr;
      ST_uint32 udNrOfElementsInArray;
      void *pArray;
    } Write;

    /**** Program Parameters ****/
    struct {
      uAddrType udAddr;
      ST_uint32 udNrOfElementsInArray;
      void *pArray;
    } Program;

    /**** ReadLockRegister Parameters ****/
    struct {
        uAddrType udAddr;
        ST_uint8 ucpLockRegister;
    } ReadLockRegister;

    /**** WriteLockRegister Parameters ****/
    struct {
        uAddrType udAddr;
        ST_uint8 ucLockRegister;
    } WriteLockRegister;

} ParameterType;


/******************************************************************************
    Standard functions
*******************************************************************************/
  ReturnType  Flash( InstructionType insInstruction, ParameterType *fp );
  ReturnType  FlashWriteEnable( void );
  ReturnType  FlashWriteDisable( void );
  ReturnType  FlashReadDeviceIdentification( ST_uint16 *uwpDeviceIdentification);
  ReturnType  FlashReadManufacturerIdentification( ST_uint8 *ucpManufacturerIdentification);
  ReturnType  FlashReadStatusRegister( ST_uint8 *ucpStatusRegister);
  ReturnType  FlashRead( uAddrType udAddr, ST_uint8 *ucpElements, ST_uint32 udNrOfElementsToRead);
  ReturnType  FlashFastRead( uAddrType udAddr, ST_uint8 *ucpElements, ST_uint32 udNrOfElementsToRead);
  ReturnType  FlashPageWrite( uAddrType udAddr, ST_uint8 *pArray , ST_uint16 udNrOfElementsInArray);
  ReturnType  FlashPageProgram( uAddrType udAddr, ST_uint8 *pArray, ST_uint16 udNrOfElementsInArray );
  ReturnType  FlashPageErase( uPageType upgPageNr );
  ReturnType  FlashSectorErase( uSectorType uscSectorNr );
  ReturnType  FlashBulkErase( void );
  ReturnType  FlashDeepPowerDown( void );
  ReturnType  FlashReleaseFromDeepPowerDown( void );
  ReturnType  FlashWrite( ST_uint32 udAddr, ST_uint8 *pArray, ST_uint32 udNrOfElementsInArray );
  ReturnType  FlashProgram( ST_uint32 udAddr, ST_uint8 *pArray , ST_uint32 udNrOfElementsInArray);
  ReturnType  FlashReadLockRegister( ST_uint32 udAddr, ST_uint8 *ucLockRegister );
  ReturnType  FlashWriteLockRegister( ST_uint32 udAddr, ST_uint8 ucLockRegister );

/******************************************************************************
    Utility functions
*******************************************************************************/
#ifdef VERBOSE
   ST_sint8 *FlashErrorStr( ReturnType rErrNum );
#endif

  ReturnType  FlashTimeOut( ST_uint32 udSeconds );

/*******************************************************************************
List of Errors and Return values, Explanations and Help.
********************************************************************************

Error Name:   Flash_AddressInvalid
Description:  The address given is out of the Flash memory address range.
Solution:     Check whether the address given is in the valid Flash memory address range.
********************************************************************************

Error Name:   Flash_PageEraseFailed
Description:  The Page Erase instruction did not complete successfully.
Solution:     Try to erase the Page again. If the operation fails again, the device
              may be faulty and need to be replaced.
********************************************************************************

Error Name:   Flash_PageNrInvalid
Note:         The Flash memory is not at fault.
Description:  An attempt to select a Page (Parameter) that is not
              within the valid range has been made. Valid Page numbers are from 0 to
              FLASH_PAGE_COUNT - 1.
Solution:     Check that the Page number given is in the valid range.
********************************************************************************

Error Name:   Flash_SectorNrInvalid
Note:         The Flash memory is not at fault.
Description:  An attempt to select a Sector(Parameter) that is not
              within the valid range has been made. Valid Sector numbers are from 0 to
              FLASH_SECTOR_COUNT - 1.
Solution:     Check that the Sector number given is in the valid range.
********************************************************************************

Return Name:  Flash_FunctionNotSupported
Description:  The user has attempted to make use of a functionality not
              available on this Flash device (and thus not provided by the
              software drivers).
Solution:     This may happen after changing Flash SW Drivers in existing
              environments. For example an application tries to use a
              functionality that is no longer provided with the new device.
********************************************************************************

Return Name:  Flash_NoInformationAvailable
Description:  The system cannot give any additional information about the error.
Solution:     None
********************************************************************************

Error Name:   Flash_OperationOngoing
Description:  This message is one of two messages which are given by the TimeOut
              subroutine. That means that the current Flash memory operation is still
	      within the defined time frame.
********************************************************************************

Error Name:   Flash_OperationTimeOut
Description:  The Program/Erase Controller algorithm could not complete an
              operation successfully. It should have set bit 7 of the Status
              Register from 0 to 1, but this did not happen within the predefined
              time limit. The operation was therefore cancelled by a timeout.
	      This may be due to the device that is damaged.
Solution:     Try the previous instruction again. If it fails a second time then it
              is likely that the device needs to be replaced.
********************************************************************************

Error Name:   Flash_ProgramFailed
Description:  The value that should be programmed has not been written correctly
              to the Flash memory.
Solutions:    Make sure that the Page which is supposed to receive the value
              was erased successfully before programming. Try to erase the Page and
              to program the value again. If it fails again then the device may
              be faulty.
********************************************************************************

Error Name:   Flash_WrongType
Description:  This message appears if the Manufacturer and Device Identifications read from
              the Flash device in use do not match the expected Identification
              values. This means that the source code is not explicitly written for
              the currently used Flash chip. It may work, but correct operation is not
              guaranteed.
Solutions:    Use a different Flash chip with the target hardware or contact
              STMicroelectronics for a different source code library.
********************************************************************************

Return Name:  Flash_Success
Description:  This value indicates that the Flash memory instruction was executed
              correctly.
********************************************************************************/

/*******************************************************************************
Device Constants
*******************************************************************************/


#define MANUFACTURER_ST (0x20)    /* ST Manufacturer Code is 0x20 */
#define ANY_ADDR (0x0)            /* Any address offset within the Flash Memory will do */

#ifdef USE_M25PE10                       /* The M25PE10 device */
   #define EXPECTED_DEVICE (0x8011)      /* Device code for the M25PE10 */
   #define FLASH_SIZE (0x020000)		 /* Total device size in Bytes */
   #define FLASH_PAGE_COUNT (0x0200)	 /* Total device size in Pages */
   #define FLASH_SECTOR_COUNT (0x02)     /* Total device size in Sectors */
   #define FLASH_WRITE_BUFFER_SIZE 0x100 /* Write Buffer = 256 bytes */
   #define FLASH_MWA 1                   /* Minimum Write Access */

   #define PW_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Write Operation*/
   #define PP_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Program Operation*/
   #define PE_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Erase Operation*/
   #define SE_TIMEOUT (0x06)		 /* Timeout in seconds suggested for Sector Erase Operation*/

   #define NUM_NV_PAGES          (0x08)
   #define NUM_SYS_PAGES         (0x01)
   #define NV_BASE_ADDRESS       (0x00)
   #define NUM_DL_FALLOW_PAGES   (0x08)
   #define DL_BASE_ADDRESS       ((NUM_NV_PAGES+NUM_SYS_PAGES+NUM_DL_FALLOW_PAGES)*FLASH_WRITE_BUFFER_SIZE)
   #define SYSNV_BASE_ADDRESS    (NUM_NV_PAGES*FLASH_WRITE_BUFFER_SIZE)

   #define DF_PAGESIZE             FLASH_WRITE_BUFFER_SIZE
   #define DF_SHIFTCOUNT           8

#endif /* USE_M25PE10 */


#ifdef USE_M25PE20                       /* The M25PE20 device */
   #define EXPECTED_DEVICE (0x8012)      /* Device code for the M25PE20 */
   #define FLASH_SIZE (0x040000)		 /* Total device size in Bytes */
   #define FLASH_PAGE_COUNT (0x0400)	 /* Total device size in Pages */
   #define FLASH_SECTOR_COUNT (0x04)     /* Total device size in Sectors */
   #define FLASH_WRITE_BUFFER_SIZE 0x100 /* Write Buffer = 256 bytes */
   #define FLASH_MWA 1                   /* Minimum Write Access */

   #define PW_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Write Operation*/
   #define PP_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Program Operation*/
   #define PE_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Erase Operation*/
   #define SE_TIMEOUT (0x06)		 /* Timeout in seconds suggested for Sector Erase Operation*/

#endif /* USE_M25PE20 */

#ifdef USE_M25PE40                       /* The M25PE40 device */
   #define EXPECTED_DEVICE (0x8013)      /* Device code for the M25PE40 */
   #define FLASH_SIZE (0x080000)		 /* Total device size in Bytes */
   #define FLASH_PAGE_COUNT (0x0800)	 /* Total device size in Pages */
   #define FLASH_SECTOR_COUNT (0x08)     /* Total device size in Sectors */
   #define FLASH_WRITE_BUFFER_SIZE 0x100 /* Write Buffer = 256 bytes */
   #define FLASH_MWA 1                   /* Minimum Write Access */

   #define PW_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Write Operation*/
   #define PP_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Program Operation*/
   #define PE_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Erase Operation*/
   #define SE_TIMEOUT (0x06)		 /* Timeout in seconds suggested for Sector Erase Operation*/
#endif /* USE_M25PE40 */

#ifdef USE_M25PE80                       /* The M25PE80 device */
   #define EXPECTED_DEVICE (0x8014)      /* Device code for the M25PE80 */
   #define FLASH_SIZE (0x100000)		 /* Total device size in Bytes */
   #define FLASH_PAGE_COUNT (0x1000)	 /* Total device size in Pages */
   #define FLASH_SECTOR_COUNT (0x10)     /* Total device size in Sectors */
   #define FLASH_SUBSECTOR_COUNT (0x10)  /* Total sub-Sector size in Sectors */
   #define FLASH_WRITE_BUFFER_SIZE 0x100 /* Write Buffer = 256 bytes */
   #define FLASH_MWA 1                   /* Minimum Write Access */

   #define PW_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Write Operation*/
   #define PP_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Program Operation*/
   #define PE_TIMEOUT (0x01)		 /* Timeout in seconds suggested for Page Erase Operation*/
   #define SE_TIMEOUT (0x06)		 /* Timeout in seconds suggested for Sector Erase Operation*/
   #define BE_TIMEOUT (0x3D)		 /* Timeout in seconds suggested for Bulk Erase Operation*/


#endif /* USE_M25PE80 */

/******************************************************************************
    External variable declaration
*******************************************************************************/

// none in this version of the release

/*******************************************************************************
Flash Status Register Definitions (see Datasheet)
*******************************************************************************/
enum
{
	SPI_FLASH_WIP	= 0x01,				// write/program/erase in progress indicator
	SPI_FLASH_WEL	= 0x02				// write enable latch
};

/*******************************************************************************
Specific Function Prototypes
*******************************************************************************/
//typedef unsigned char BOOL;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

uint8 IsFlashBusy(void);

/*******************************************************************************
List of Specific Errors and Return values, Explanations and Help.
*******************************************************************************

// none in this version of the release
********************************************************************************/

#endif /* __C2195__H__  */
/* In order to avoid a repeated usage of the header file */

/*******************************************************************************
     End of C2195.h
********************************************************************************/

