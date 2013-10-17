/**************** STFL-I based Serial Flash Memory Driver **********************

   Filename:    c2195.c
   Description: Library routines for the M25PE10, M25PE20, M25PE40, M25PE80
               Serial Flash Devices.

   Version:     V1.1
   Date:        10-17-2005
   Authors:     Zhi Tan, Da Gang Zhou, STMicroelectronics, Shanghai (China)
   Copyright (c) 2005 STMicroelectronics.

   THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH
   CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A
   RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
   CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT OF SUCH
   SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN
   IN CONNECTION WITH THEIR PRODUCTS.
********************************************************************************

   Version History.
   Ver.  Date        Comments

   1.0   07/07/2005   M25PE10, M25PE20 and M25PE40 support
   1.1   10/17/2005   Add M25PE80 support by Da Gang Zhou
********************************************************************************

   This source file provides library C code for the  M25PE10, M25PE20, M25PE40,
   and M25PE80 Serial Flash devices.

   The following functions are available in this library:

      Flash(WriteEnable, 0)                         to disable write protect of the Flash memory
      Flash(WriteDisable, 0)                        to enable write protect of the Flash memory
      Flash(ReadDeviceIdentification, ParameterType)          to get the Device Identification from the device
      Flash(ReadManufacturerIdentification, ParameterType)    to get the manufacturer Identification from the device
      Flash(ReadStatusRegister, ParameterType)      to get the value in the Status Register from the device
      Flash(Read, ParameterType)                    to read from the Flash device
      Flash(FastRead, ParameterType)                to read from the Flash device in a faster way
      Flash(PageWrite, ParameterType)               to write an array of elements within one page
      Flash(PageProgram, ParameterType)             to program an array of elements within one page
      Flash(PageErase, ParameterType)               to erase a whole page
      Flash(SectorErase, ParameterType)             to erase a whole sector
      Flash(BulkErase, 0)                           to erase the whole chip
      Flash(DeepPowerDown, 0)                       to put the memory in a low power consumption mode
      Flash(ReleaseFromDeepPowerDown, 0)            to wake up the memory from the low power consumption mode
      Flash(Write, ParameterType)                   to write an array of elements
      Flash(Program, ParameterType)                 to program an array of elements
      Flash(ReadLockRegister, ParameterType)        to get the lock register value
      Flash(WriteLockRegister, ParameterType)       to set the lock register value
      FlashErrorStr()                               to return an error description (define VERBOSE)

   Note that data Bytes will be referred to as elements throughout the document unless otherwise specified.

   For further information consult the related Datasheets (M25PE10, M25PE20, M25PE40 and M25PE80)
   and Application Note (ANXXXX).
   The Application Note gives information about how to modify this code for
   a specific application.

   The hardware specific functions which may need to be modified by the user are:

      FlashWrite() for writing an element (uCPUBusType) to the Flash memory
      FlashRead()  for reading an element (uCPUBusType) from the Flash memory
      FlashTimeOut()                              to return after the function has timed out

   A list of the error conditions can be found at the end of the header file.

*******************************************************************************/
//#include "iocc2430.h"
//#include "ZcomDef.h"
//#include "OnBoard.h"
#include "hal_types.h"
#include "Serialize.h" /* Header file with SPI master abstract prototypes */

#ifdef TIME_H_EXISTS
  #include <time.h>
#endif

#ifdef SYNCHRONOUS_IO
    #define WAIT_TILL_INSTRUCTION_EXECUTION_COMPLETE(x)  \
    FlashTimeOut(0); \
    while(IsFlashBusy()) \
    { \
        if(Flash_OperationTimeOut == FlashTimeOut(x)) return  Flash_OperationTimeOut; \
    };
#else
    // do nothing
#endif


/******************************************************************************
    Global variables: none
*******************************************************************************/


/*******************************************************************************
Function:     ReturnType Flash( InstructionType insInstruction, ParameterType *fp )
Arguments:    insInstruction is an enum which contains all the available function
    instructions of the SW driver.
              fp is a (union) parameter struct for all Flash memory instruction parameters
Return Value: The function returns the following conditions:

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
   Flash_Success,
   Flash_WrongType

Description:  This function is used to access all functions provided with the
   current Flash device.

Pseudo Code:
   Step 1: Select the right action using the insInstruction parameter
   Step 2: Execute the Flash memory Function
   Step 3: Return the Error Code
*******************************************************************************/
ReturnType Flash( InstructionType insInstruction, ParameterType *fp ) {
    ReturnType  rRetVal;
#ifdef FULL_DATAFLASH_FUNCTION
    ST_uint8  ucManufacturerIdentification, ucStatusRegister;
    ST_uint16 ucDeviceIdentification;
#elif defined USE_M25PE80
    ST_uint8  ucStatusRegister;
#endif
   switch (insInstruction) {
#ifdef FULL_DATAFLASH_FUNCTION
      case WriteEnable:
           rRetVal = FlashWriteEnable( );
           break;

      case WriteDisable:
           rRetVal = FlashWriteDisable( );
           break;

      case ReadDeviceIdentification:
           rRetVal = FlashReadDeviceIdentification(&ucDeviceIdentification);
           (*fp).ReadDeviceIdentification.ucDeviceIdentification = ucDeviceIdentification;
           break;

      case ReadManufacturerIdentification:
           rRetVal = FlashReadManufacturerIdentification(&ucManufacturerIdentification);
           (*fp).ReadManufacturerIdentification.ucManufacturerIdentification = ucManufacturerIdentification;
           break;

      case ReadStatusRegister:
           rRetVal = FlashReadStatusRegister(&ucStatusRegister);
           (*fp).ReadStatusRegister.ucStatusRegister = ucStatusRegister;
           break;
#endif
      case Read:
           rRetVal = FlashRead( (*fp).Read.udAddr,
                                (*fp).Read.pArray,
                                (*fp).Read.udNrOfElementsToRead
                              );
           break;
#ifdef FULL_DATAFLASH_FUNCTION
      case FastRead:
           rRetVal = FlashFastRead( (*fp).Read.udAddr,
                                    (*fp).Read.pArray,
                                    (*fp).Read.udNrOfElementsToRead
                                  );
           break;
#endif
      case PageWrite:
           rRetVal = FlashWrite( (*fp).PageWrite.udAddr,
                                 (*fp).PageWrite.pArray,
                                 (*fp).PageWrite.udNrOfElementsInArray
                               );
           break;
#ifdef FULL_DATAFLASH_FUNCTION
      case PageProgram:
           rRetVal = FlashProgram( (*fp).PageProgram.udAddr,
                                 (*fp).PageProgram.pArray,
                                 (*fp).PageProgram.udNrOfElementsInArray
                               );
           break;

      case PageErase:
           rRetVal = FlashPageErase( (*fp).PageErase.upgPageNr );
           break;

      case SectorErase:
           rRetVal = FlashSectorErase((*fp).SectorErase.ustSectorNr );
           break;
#endif

#if defined( USE_M25PE80 )
      case BulkErase:
           rRetVal = FlashBulkErase( );
           break;
#endif

#ifdef FULL_DATAFLASH_FUNCTION
      case DeepPowerDown:
           rRetVal = FlashDeepPowerDown( );
           break;
	
      case ReleaseFromDeepPowerDown:
           rRetVal = FlashReleaseFromDeepPowerDown( );
           break;
#endif
      case Write:
           rRetVal = FlashWrite( (*fp).Write.udAddr,
                                   (*fp).Write.pArray,
								   (*fp).Write.udNrOfElementsInArray);
           break;
#ifdef FULL_DATAFLASH_FUNCTION
      case Program:
           rRetVal = FlashProgram( (*fp).Program.udAddr,
                                   (*fp).Program.pArray,
                                   (*fp).Program.udNrOfElementsInArray);
           break;
#endif
#if defined( USE_M25PE80 )
      case ReadLockRegister:
           rRetVal = FlashReadLockRegister( (*fp).ReadLockRegister.udAddr,
                                            &ucStatusRegister);
           (*fp).ReadLockRegister.ucpLockRegister = ucStatusRegister;
           break;
      case WriteLockRegister:
           rRetVal = FlashWriteLockRegister( (*fp).WriteLockRegister.udAddr,
                                             (*fp).WriteLockRegister.ucLockRegister);
           break;
#endif
      default:
           rRetVal = Flash_FunctionNotSupported;
           break;

   } /* EndSwitch */
   return rRetVal;
} /* EndFunction Flash */

/*******************************************************************************
Function:     FlashWriteEnable( void )
Arguments:    void

Return Value:
   Flash_Success

Description:  This function sets the Write Enable Latch(WEL)
              by sending a WREN instruction.

Pseudo Code:
   Step 1: Initialize the data (i.e. instruction) packet to be sent serially
   Step 2: Send the packet serially
*******************************************************************************/
ReturnType  FlashWriteEnable( void )
{
    CharStream char_stream_send;
    ST_uint8  cWREN = SPI_FLASH_INS_WREN;

    // Step 1: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length = 1;
    char_stream_send.pChar  = &cWREN;

    // Step 2: Send the packet serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumDisableTransOnly_DeSelectSlave
              );
    return Flash_Success;
} /* EndFunction FlashWriteEnable */

/*******************************************************************************
Function:     FlashWriteDisable( void )
Arguments:    void

Return Value:
   Flash_Success

Description:  This function resets the Write Enable Latch(WEL)
              by sending a WRDI instruction.

Pseudo Code:
   Step 1: Initialize the data (i.e. instruction) packet to be sent serially
   Step 2: Send the packet serially
*******************************************************************************/
ReturnType  FlashWriteDisable( void )
{
    CharStream char_stream_send;
    ST_uint8  cWRDI = SPI_FLASH_INS_WRDI;

    // Step 1: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length = 1;
    char_stream_send.pChar  = &cWRDI;

    // Step 2: Send the packet serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumDisableTransOnly_DeSelectSlave
              );
    return Flash_Success;
} /* EndFunction FlashWriteDisable */
#ifdef FULL_DATAFLASH_FUNCTION
/*******************************************************************************
Function:     FlashReadDeviceIdentification( ST_uint16 *uwpDeviceIdentification)
Arguments:    uwpDeviceIdentification, 16-bit buffer to hold the DeviceIdentification read from the
              memory, with the memory type residing in the higher 8-bit nibble, and the
              memory capacity in the lower 8-bit nibble

Return Value:
   Flash_Success
   Flash_WrongType

Description:  This function returns the Device Identification (memory type + memory capacity)
              by sending an SPI_FLASH_INS_RDID instruction.
              After retrieving the Device Identification, the routine checks if the device is
              the expected device (defined by EXPECTED_DEVICE). If not,
              Flash_WrongType is returned.

Pseudo Code:
   Step 1: Initialize the data (i.e. instruction) packet to be sent serially
   Step 2: Send the packet serially
   Step 3: get returned Device Identification (memory type + memory capacity)
*******************************************************************************/
ReturnType  FlashReadDeviceIdentification( ST_uint16 *uwpDeviceIdentification)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    ST_uint8  cRDID = SPI_FLASH_INS_RDID;
    ST_uint8  pID[3];

    // Step 1: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length  = 1;
    char_stream_send.pChar   = &cRDID;

    char_stream_recv.length  = 3;
    char_stream_recv.pChar   = &pID[0];

    // Step 2: Send the packet serially
    Serialize(&char_stream_send,
              &char_stream_recv,
              enumEnableTansRecv_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    // Step 3: get the returned Device Identification (memory type + memory capacity)
    *uwpDeviceIdentification = char_stream_recv.pChar[1];
    *uwpDeviceIdentification <<= 8;
    *uwpDeviceIdentification |= char_stream_recv.pChar[2];

    if(EXPECTED_DEVICE == *uwpDeviceIdentification)
        return Flash_Success;
    else
        return Flash_WrongType;
} /* EndFunction FlashReadDeviceIdentification */

/*******************************************************************************
Function:     FlashReadManufacturerIdentification( ST_uint8 *ucpManufacturerIdentification)
Arguments:    ucpManufacturerIdentification, 8-bit buffer to hold the Manufacturer Identification
              being read from the memory

Return Value:
   Flash_WrongType: if any value other than MANUFACTURER_ST (0x20) is returned
   Flash_Success : if MANUFACTURER_ST(0x20) is correctly returned

Description:  This function returns the Manufacturer Identification (0x20)
              by sending an SPI_FLASH_INS_RDID instruction.
              After retrieving the Manufacturer Identification, the routine checks if the device is
              an ST memory product. If not, Flash_WrongType is returned.

Pseudo Code:
   Step 1: Initialize the data (i.e. instruction) packet to be sent serially
   Step 2: Send the packet serially
   Step 3: get the Manufacturer Identification
*******************************************************************************/
ReturnType  FlashReadManufacturerIdentification( ST_uint8 *ucpManufacturerIdentification)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    ST_uint8  cRDID = SPI_FLASH_INS_RDID;
    ST_uint8  pID[3];

    // Step 1: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length  = 1;
    char_stream_send.pChar   = &cRDID;
    char_stream_recv.length  = 3;
    char_stream_recv.pChar   = &pID[0];

    // Step 2: Send the packet serially
    Serialize(&char_stream_send,
              &char_stream_recv,
              enumEnableTansRecv_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    // Step 3: get the returned Manufacturer Identification
    *ucpManufacturerIdentification = pID[0];
    if(MANUFACTURER_ST == *ucpManufacturerIdentification)
    {
        return Flash_Success;
    }
    else
    {
        return Flash_WrongType;
    }
} /* EndFunction FlashReadManufacturerIdentification */
#endif
/*******************************************************************************
Function:     FlashReadStatusRegister( ST_uint8 *ucpStatusRegister)
Arguments:    ucpStatusRegister, 8-bit buffer to hold the status(WEL + WIP) read
              from the memory

Return Value:
   Flash_Success

Description:  This function reads the Status Register by sending an
               SPI_FLASH_INS_RDSR instruction.

Pseudo Code:
   Step 1: Initialize the data (i.e. instruction) packet to be sent serially
   Step 2: Send the packet serially
   Step 3: get the returned status(WEL + WIP)
*******************************************************************************/
ReturnType  FlashReadStatusRegister( ST_uint8 *ucpStatusRegister)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    ST_uint8  cRDSR = SPI_FLASH_INS_RDSR;
    ST_uint8  cSR;

    // Step 1: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length  = 1;
    char_stream_send.pChar   = &cRDSR;
    char_stream_recv.length  = 1;
    char_stream_recv.pChar   = &cSR;

    // Step 2: Send the packet serially
    Serialize(&char_stream_send,
              &char_stream_recv,
              enumEnableTansRecv_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    // Step 3: get the returned status(WEL + WIP)
    *ucpStatusRegister = cSR;

    return Flash_Success;
} /* EndFunction FlashReadStatusRegister */


/*******************************************************************************
Function:     FlashRead( ST_uint32 udAddr, ST_uint8 *ucpElements, ST_uint32 udNrOfElementsToRead)
Arguments:    udAddr, start address to read from
              ucpElements, buffer to hold the elements to be returned
              udNrOfElementsToRead, number of elements to be returned, counted in bytes.

Return Value:
   Flash_AddressInvalid
   Flash_Success

Description:  This function reads the Flash memory by sending an
              SPI_FLASH_INS_READ instruction.
              by design, the whole Flash memory space can be read with one READ instruction
              by incrementing the start address and rolling to 0h automatically,
              that is, this function goes across pages and sectors.

Pseudo Code:
   Step 1: Validate address input
   Step 2: Initialize the data (i.e. instruction) packet to be sent serially
   Step 3: Send the packet serially, and fill the buffer with the data being returned
*******************************************************************************/
ReturnType  FlashRead( uAddrType udAddr, ST_uint8 *ucpElements, ST_uint32 udNrOfElementsToRead)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    ST_uint8  pIns_Addr[4];

    // Step 1: Validate address input
    if(!(udAddr <  FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length   = 4;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_READ;
    pIns_Addr[1]              = udAddr>>16;
    pIns_Addr[2]              = udAddr>>8;
    pIns_Addr[3]              = udAddr;

    char_stream_recv.length   = udNrOfElementsToRead;
    char_stream_recv.pChar    = ucpElements;

    // Step 3: Send the packet serially, and fill the buffer with the data being returned
    Serialize(&char_stream_send,
              &char_stream_recv,
              enumEnableTansRecv_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    return Flash_Success;
} /* EndFunction FlashRead */

#ifdef FULL_DATAFLASH_FUNCTION
/*******************************************************************************
Function:     FlashFastRead( ST_uint32 udAddr, ST_uint8 *ucpElements, ST_uint32 udNrOfElementsToRead)
Arguments:    udAddr, start address to read from
              ucpElements, buffer to hold the elements to be returned
              udNrOfElementsToRead, number of elements to be returned, counted in bytes.

Return Value:
   Flash_AddressInvalid
   Flash_Success

Description:  This function reads the Flash memory by sending an
              SPI_FLASH_INS_FAST_READ instruction.
              by design, the whole Flash memory space can be read with one READ instruction
              by incrementing the start address and rolling to 0h automatically,
              that is, this function goes across pages and sectors.

Pseudo Code:
   Step 1: Validate address input
   Step 2: Initialize the data (i.e. instruction) packet to be sent serially
   Step 3: Send the packet serially, and fill the buffer with data being returned
*******************************************************************************/
ReturnType  FlashFastRead( uAddrType udAddr, ST_uint8 *ucpElements, ST_uint32 udNrOfElementsToRead)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    ST_uint8  pIns_Addr[5];

    // Step 1: Validate address input
    if(!(udAddr <  FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length   = 5;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_FAST_READ;
    pIns_Addr[1]              = udAddr>>16;
    pIns_Addr[2]              = udAddr>>8;
    pIns_Addr[3]              = udAddr;
    pIns_Addr[4]              = SPI_FLASH_INS_DUMMY;

    char_stream_recv.length   = udNrOfElementsToRead;
    char_stream_recv.pChar    = ucpElements;

    // Step 3: Send the packet serially, and fill the buffer with the data being returned
    Serialize(&char_stream_send,
              &char_stream_recv,
              enumEnableTansRecv_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    return Flash_Success;
} /* EndFunction FlashFastRead */
#endif

/*******************************************************************************
Function:     FlashPageWrite( ST_uint32 udAddr, ST_uint8 *pArray, ST_uint32 udNrOfElementsInArray)
Arguments:    udAddr, start address to write to
              pArray, buffer to hold the elements to be written
              udNrOfElementsInArray, number of elements to be written, counted in bytes

Return Value:
   Flash_AddressInvalid
   Flash_OperationOngoing
   Flash_OperationTimeOut
   Flash_Success

Description:  This function writes 256 bytes or less of data into the memory by sending an
              SPI_FLASH_INS_PW instruction.
              by design, the PW instruction is effective WITHIN ONE page,i.e. 0xXX00 - 0xXXFF.
              when 0xXXFF is reached, the address rolls over to 0xXX00 automatically.

Pseudo Code:
   Step 1: Validate address input
   Step 2: Check whether any previous Write, Program or Erase cycle is on-going
   Step 3: Disable Write protection (the Flash memory will automatically enable it again after
           the execution of the instruction)
   Step 4: Initialize the data (instruction & address only) packet to be sent serially
   Step 5: Send the packet (instruction & address only) serially
   Step 6: Initialize the data (data to be written) packet to be sent serially
   Step 7: Send the packet (data to be written) serially
   Step 8: Wait until the operation completes or a timeout occurs.
*******************************************************************************/
ReturnType  FlashPageWrite( uAddrType udAddr, ST_uint8 *pArray , ST_uint16 udNrOfElementsInArray)
{
    CharStream char_stream_send;
    ST_uint8  pIns_Addr[4];

    // Step 1: Validate address input
    if(!(udAddr <  FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
    if (IsFlashBusy()) return Flash_OperationOngoing;

    // Step 3: Disable Write protection
    FlashWriteEnable();

    // Step 4: Initialize the data (instruction & address only) packet to be sent serially
    char_stream_send.length   = 4;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_PW;
    pIns_Addr[1]              = udAddr>>16;
    pIns_Addr[2]              = udAddr>>8;
    pIns_Addr[3]              = udAddr;

    // Step 5: Send the packet (instruction & address only) serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumNull
              );

    // Step 6: Initialize the data (data to be written) packet to be sent serially
    char_stream_send.length   = udNrOfElementsInArray;
    char_stream_send.pChar    = pArray;

    // Step 7: Send the packet (data to be written) serially
    Serialize(&char_stream_send,
              0,
              enumNull,
              enumDisableTransOnly_DeSelectSlave
              );

    // Step 8: Wait until the operation completes or a timeout occurs.
    WAIT_TILL_INSTRUCTION_EXECUTION_COMPLETE(PW_TIMEOUT)

    return Flash_Success;
} /* EndFunction FlashPageWrite */


/*******************************************************************************
Function:     FlashPageProgram( ST_uint32 udAddr, ST_uint8 *pArray, ST_uint32 udNrOfElementsInArray)
Arguments:    udAddr, start address to write to
              pArray, buffer to hold the elements to be programmed
              udNrOfElementsInArray, number of elements to be programmed, counted in bytes

Return Value:
   Flash_AddressInvalid
   Flash_OperationOngoing
   Flash_OperationTimeOut
   Flash_Success

Description:  This function writes 256 bytes or less of data into the memory by sending an
              SPI_FLASH_INS_PP instruction.
              by design, the PP instruction is effective WITHIN ONE page,i.e. 0xXX00 - 0xXXFF.
              when 0xXXFF is reached, the address rolls over to 0xXX00 automatically.

Pseudo Code:
   Step 1: Validate address input
   Step 2: Check whether any previous Write, Program or Erase cycle is on-going
   Step 3: Disable Write protection (the Flash memory will automatically enable it again after
           the execution of the instruction)
   Step 4: Initialize the data (instruction & address only) packet to be sent serially
   Step 5: Send the packet (instruction & address only) serially
   Step 6: Initialize the data (data to be programmed) packet to be sent serially
   Step 7: Send the packet (data to be programmed) serially
   Step 8: Wait until the operation completes or a timeout occurs.
*******************************************************************************/
ReturnType  FlashPageProgram( uAddrType udAddr, ST_uint8 *pArray , ST_uint16 udNrOfElementsInArray)
{
    CharStream char_stream_send;
    ST_uint8  pIns_Addr[4];

    // Step 1: Validate address input
    if(!(udAddr <  FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Check whether any previous a Write, Program or Erase cycle is still on-going
    if(IsFlashBusy()) return Flash_OperationOngoing;

    // Step 3: Disable Write protection
    FlashWriteEnable();

    // Step 4: Initialize the data (instruction & address only) packet to be sent serially
    char_stream_send.length   = 4;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_PP;
    pIns_Addr[1]              = udAddr>>16;
    pIns_Addr[2]              = udAddr>>8;
    pIns_Addr[3]              = udAddr;

    // Step 5: Send the packet (instruction & address only) serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumNull
              );

    // Step 6: Initialize the data (data to be programmed) packet to be sent serially
    char_stream_send.length   = udNrOfElementsInArray;
    char_stream_send.pChar    = pArray;

    // Step 7: Send the packet(data to be programmed) serially
    Serialize(&char_stream_send,
              0,
              enumNull,
              enumDisableTransOnly_DeSelectSlave
              );

    // Step 8: Wait until the operation completes or a timeout occurs.
    WAIT_TILL_INSTRUCTION_EXECUTION_COMPLETE(PP_TIMEOUT)

    return Flash_Success;
} /* EndFunction FlashPageProgram */
#ifdef FULL_DATAFLASH_FUNCTION
/*******************************************************************************
Function:     ReturnType FlashPageErase( uPageType upgPageNr )
Arguments:    upgPageNr is the number of the Page to be erased.

Return Values:
   Flash_PageNrInvalid
   Flash_OperationOngoing
   Flash_OperationTimeOut
   Flash_Success

Description:  This function erases the Page specified in upgPageNr
   by sending an SPI_FLASH_INS_PE instruction.
   The function checks that the Page number is within the valid range before issuing
   the erase instruction. Once erase has completed the Flash_Success value
   is returned.

Pseudo Code:
   Step 1: Validate page number input
   Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
   Step 3: Disable Write protection (the Flash memory will automatically enable it again after
           the execution of the instruction)
   Step 4: Initialize the data (instruction & address) packet to be sent serially
   Step 5: Send the packet (data to be programmed) serially
   Step 6: Wait until the operation completes or a timeout occurs.
*******************************************************************************/
ReturnType  FlashPageErase( uPageType upgPageNr )
{
    CharStream char_stream_send;
    ST_uint8 pIns_Addr[4];

    // Step 1: Validate page number input
    if(!(upgPageNr < FLASH_PAGE_COUNT)) return Flash_PageNrInvalid;

    // Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
    if(IsFlashBusy()) return Flash_OperationOngoing;

    // Step 3: Disable Write protection
    FlashWriteEnable();

    // Step 4: Initialize the data (instruction & address) packet to be sent serially
    char_stream_send.length   = 4;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_PE;
    pIns_Addr[1]              = upgPageNr>>8;
    pIns_Addr[2]              = upgPageNr;
    pIns_Addr[3]              = 0;

    // Step 5: Send the packet (instruction & address) serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    // Step 6: Wait until the operation completes or a timeout occurs.
    WAIT_TILL_INSTRUCTION_EXECUTION_COMPLETE(PE_TIMEOUT)

    return Flash_Success;
} /* EndFunction FlashPageErase */
#endif
#ifdef FULL_DATAFLASH_FUNCTION
/*******************************************************************************
Function:     ReturnType FlashSectorErase( uSectorType uscSectorNr )
Arguments:    uSectorType is the number of the Sector to be erased.

Return Values:
   Flash_SectorNrInvalid
   Flash_OperationOngoing
   Flash_OperationTimeOut
   Flash_Success

Description:  This function erases the Sector specified in uscSectorNr
   by sending an SPI_FLASH_INS_SE instruction.
   The function checks that the sector number is within the valid range before issuing
   the erase instruction. Once erase has completed the Flash_Success value
   is returned.

Pseudo Code:
   Step 1: Validate sector number input
   Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
   Step 3: Disable Write protection (the Flash memory will automatically enable it
           again after the execution of the instruction)
   Step 4: Initialize the data (instruction & address) packet to be sent serially
   Step 5: Send the packet (instruction & address) serially
   Step 6: Wait until the operation completes or a timeout occurs.
*******************************************************************************/
ReturnType  FlashSectorErase( uSectorType uscSectorNr )
{
    CharStream char_stream_send;
    ST_uint8  pIns_Addr[4];

    // Step 1: Validate sector number input
    if(!(uscSectorNr < FLASH_SECTOR_COUNT)) return Flash_SectorNrInvalid;

    // Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
    if(IsFlashBusy()) return Flash_OperationOngoing;

    // Step 3: Disable Write protection
    FlashWriteEnable();

    // Step 4: Initialize the data (instruction & address) packet to be sent serially
    char_stream_send.length   = 4;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_SE;
    pIns_Addr[1]              = uscSectorNr;
    pIns_Addr[2]              = 0;
    pIns_Addr[3]              = 0;

    // Step 5: Send the packet (instruction & address) serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    // Step 6: Wait until the operation completes or a timeout occurs.
    WAIT_TILL_INSTRUCTION_EXECUTION_COMPLETE(SE_TIMEOUT)

    return Flash_Success;
} /* EndFunction FlashSectorErase */
#endif

#ifdef FULL_DATAFLASH_FUNCTION
/*******************************************************************************
Function:     FlashDeepPowerDown( void )
Arguments:    void

Return Value:
   Flash_OperationOngoing
   Flash_Success

Description:  This function puts the device in the lowest power consumption
              mode (the Deep Power-down mode) by sending an SPI_FLASH_INS_DP.
              After calling this routine, the Flash memory will not respond to
              any instruction except for the RDP instruction.

Pseudo Code:
   Step 1: Initialize the data (i.e. instruction) packet to be sent serially
   Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
   Step 3: Send the packet serially
*******************************************************************************/
ReturnType  FlashDeepPowerDown( void )
{
    CharStream char_stream_send;
    ST_uint8  cDP = SPI_FLASH_INS_DP;

    // Step 1: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length = 1;
    char_stream_send.pChar  = &cDP;

    // Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
    if(IsFlashBusy()) return Flash_OperationOngoing;

    // Step 3: Send the packet serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumDisableTransOnly_DeSelectSlave
              );

    return Flash_Success;
}/* EndFunction FlashDeepPowerDown */

/*******************************************************************************
Function:     FlashReleaseFromDeepPowerDown( void )
Arguments:    void

Return Value:
   Flash_Success

Description:  This function takes the device out of the Deep Power-down
              mode by sending an SPI_FLASH_INS_RDP.

Pseudo Code:
   Step 1: Initialize the data (i.e. instruction) packet to be sent serially
   Step 2: Send the packet serially
*******************************************************************************/
ReturnType  FlashReleaseFromDeepPowerDown( void )
{
    CharStream char_stream_send;
    ST_uint8  cRDP = SPI_FLASH_INS_RDP;

    // Step 1: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length = 1;
    char_stream_send.pChar  = &cRDP;

    // Step 2: Send the packet serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumDisableTransOnly_DeSelectSlave
              );
    return Flash_Success;
} /* EndFunction FlashReleaseFromDeepPowerDown */
#endif

/*******************************************************************************
Function:     FlashWrite( ST_uint32 udAddr, ST_uint8 *pArray, ST_uint32 udNrOfElementsInArray)
Arguments:    udAddr, start address to write
              pArray, address of the  buffer that holds the elements to be written
              udNrOfElementsInArray, counter of elements to be written, counted in bytes

Return Value:
   Flash_OperationOngoing
   Flash_AddressInvalid
   Flash_MemoryOverflow
   Flash_OperationTimeOut
   Flash_Success

Description:  This function writes a chunk of data into the memory at one go.
              If verifying the start address and checking the available space are successful,
              this function writes data from the buffer(pArray) to the memory sequentially by
              invoking FlashPageWrite(). This function automatically handles page boundary
              crosses, if any.

Pseudo Code:
   Step 1: Validate address input
   Step 2: Check memory space available on the whole memory
   Step 3: calculate memory space available within the page containing the start address(udAddr)
   Step 3-1: if the page boundary is crossed, invoke FlashPageWrite() repeatedly
*******************************************************************************/
ReturnType  FlashWrite( ST_uint32 udAddr, ST_uint8 *pArray, ST_uint32 udNrOfElementsInArray )
{
    ST_uint16  ucMargin;
    ST_uint16  bytes_to_write;
    ST_uint32  ucRemainder;
    ReturnType typeReturn;


    // Step 1: Validate address input
    if(!(udAddr <  FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Check memory space available on the whole memory
    if(udAddr + udNrOfElementsInArray > FLASH_SIZE) return Flash_MemoryOverflow;

    // Step 3: calculte memory space available within the page containing the start address(udAddr)
    ucMargin = (ST_uint8)(~udAddr) + 1;

    // Step 3-1: if the page boundary is crossed, invoke FlashPageWrite() repeatedly
    ucRemainder = udNrOfElementsInArray;
    while(ucRemainder>0)
    {
        if(ucMargin != FLASH_WRITE_BUFFER_SIZE)
        {
            bytes_to_write = ucMargin;
            ucMargin = FLASH_WRITE_BUFFER_SIZE;
        }
        else
            bytes_to_write = FLASH_WRITE_BUFFER_SIZE;

        if(ucRemainder <= bytes_to_write)
            bytes_to_write = ucRemainder;

        typeReturn = FlashPageWrite(udAddr, pArray, bytes_to_write);
        if(Flash_Success != typeReturn)
            return typeReturn;            // return immediately if Not successful
        udAddr += bytes_to_write;
        pArray += bytes_to_write;
        ucRemainder -= bytes_to_write;
    }

    return typeReturn;
} /* EndFunction FlashWrite */

#ifdef FULL_DATAFLASH_FUNCTION
/*******************************************************************************
Function:     FlashProgram( ST_uint32 udAddr, ST_uint8 *pArray, ST_uint32 udNrOfElementsInArray )
Arguments:    udAddr, start address to program
              pArray, address of the  buffer that holds the elements to be programmed
              udNrOfElementsInArray, number of elements to be programmed, counted in bytes

Return Value:
   Flash_AddressInvalid
   Flash_MemoryOverflow
   Flash_OperationTimeOut
   Flash_Success

Description:  This function programs a chunk of data into the memory at one go.
              If verifying the start address and checking the available space is successful,
              this function programs data from the buffer(pArray) to the memory sequentially by
              invoking FlashPageProgram(). This function automatically handles page boundary
              crosses, if any.
              Like FlashPageProgram(), this function assumes that the memory to be programmed
              has previously been erased or that bits are only changed from 1 to 0.

Pseudo Code:
   Step 1: Validate address input
   Step 2: Check memory space available in the whole memory
   Step 3: calculte memory space available within the page containing the start address(udAddr)
   Step 3-1: if the page boundary is crossed, invoke FlashPageProgram() repeatedly
*******************************************************************************/
ReturnType  FlashProgram( ST_uint32 udAddr, ST_uint8 *pArray, ST_uint32 udNrOfElementsInArray )
{
    ST_uint16  ucMargin;
    ST_uint16  bytes_to_write;
    ST_uint32  ucRemainder;
    ReturnType typeReturn;

    // Step 1: Validate address input
    if(!(udAddr <  FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Check memory space available in the whole memory
    if(udAddr + udNrOfElementsInArray > FLASH_SIZE) return Flash_MemoryOverflow;

    // Step 3: calculte memory space available within the page containing the start address(udAddr)
    ucMargin = (ST_uint8)(~udAddr) + 1;

    // Step 3-1: if the page boundary is crossed, invoke FlashPageWrite() repeatedly
    ucRemainder = udNrOfElementsInArray;
    while(ucRemainder>0)
    {
        if(ucMargin != FLASH_WRITE_BUFFER_SIZE)
        {
            bytes_to_write = ucMargin;
            ucMargin = FLASH_WRITE_BUFFER_SIZE;
        }
        else
            bytes_to_write = FLASH_WRITE_BUFFER_SIZE;

        if(ucRemainder <= bytes_to_write)
            bytes_to_write = ucRemainder;

        typeReturn = FlashPageProgram(udAddr, pArray, bytes_to_write);
        if(Flash_Success != typeReturn)
            return typeReturn;       // return immediately if Not successful
        udAddr += bytes_to_write;
        pArray += bytes_to_write;
        ucRemainder -= bytes_to_write;
    }
    return typeReturn;
} /* End of FlashProgram*/
#endif

#if  defined(USE_M25PE80)
/*******************************************************************************
Function:     ReturnType FlashBulkErase( void )
Arguments:    None.

Return Values:
   Flash_OperationOngoing
   Flash_OperationTimeOut
   Flash_Success

Description:  This function erases the whole flash
   by sending an SPI_FLASH_INS_BE instruction.
   Once erase has completed the Flash_Success value
   is returned.

Pseudo Code:
   Step 1: Check whether any previous Write, Program or Erase cycle is still on-going
   Step 2: Disable Write protection (the Flash memory will automatically enable it
           again after the execution of the instruction)
   Step 3: Initialize the data (instruction & address) packet to be sent serially
   Step 4: Send the packet (instruction & address) serially
   Step 5: Wait until the operation completes or a timeout occurs.
*******************************************************************************/
ReturnType  FlashBulkErase( void )
{
    CharStream char_stream_send;
    ST_uint8  pIns_Addr[1];

    // Step 1: Check whether any previous Write, Program or Erase cycle is still on-going
    if(IsFlashBusy()) return Flash_OperationOngoing;

    // Step 2: Disable Write protection
    FlashWriteEnable();

    // Step 3: Initialize the data (instruction & address) packet to be sent serially
    char_stream_send.length   = 1;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_BE;

    // Step 3: Send the packet (instruction & address) serially
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTransOnly_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    // Step 5: Wait until the operation completes or a timeout occurs.
    WAIT_TILL_INSTRUCTION_EXECUTION_COMPLETE(BE_TIMEOUT)

    return Flash_Success;
} /* EndFunction FlashBulkErase */

/*******************************************************************************
Function:      ST_uint8 FlashReadLockRegister( ST_uint32 udAddr ,ST_uint8 *ucpLockRegister )
Arguments:     udAddr, address to read
               ucpLockRegister, 8-bit buffer to hold the lock register read from the memory
Return Values:
   Flash_AddressInvalid
   Flash_Success

Description:   This function is used to read the Sector/Sub-sector Lock register.
Pseudo Code:
    Step 1:  Check Range of address
    Step 2:  Initialize the data (i.e. instruction) packet to be sent serially
    Step 3:  Send the packet serially, and fill the buffer with the data being returned
*******************************************************************************/
ReturnType FlashReadLockRegister( ST_uint32 udAddr, ST_uint8 *ucpLockRegister )
{
   CharStream char_stream_send,char_stream_recv;
   ST_uint8   pIns_Addr[4];

    // Step 1: Validate address
    if(!(udAddr < FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length   = 4;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_RDLR;
    pIns_Addr[1]              = udAddr>>16;
    pIns_Addr[2]              = udAddr>>8;
    pIns_Addr[3]              = udAddr;

    char_stream_recv.length   = 1;
    char_stream_recv.pChar    = ucpLockRegister;

    // Step 3: Send the packet serially, and fill the buffer with the data being returned
    Serialize(&char_stream_send,
              &char_stream_recv,
              enumEnableTansRecv_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

    return Flash_Success;

} /* EndFunction FlashReadLockRegister */

/*******************************************************************************
Function:      ST_uint8 FlashWriteLockReg( ST_uint32 udAddr, ST_uint8 ucLockRegister )
Arguments:     udAddr , address to write
               ucLockRegister, 8-bit value to write into the lock register
Return Values:
   Flash_AddressInvalid
   Flash_OperationOngoing
   Flash_Success

Description:   This function is used to write the Sector/Sub-sector Lock register.
Pseudo Code:
    Step 1:  Check Range of address
    Step 2:  Check whether any previous Write, Program or Erase cycle is still on-going
    Step 3:  Disable Write protection
    Step 4:  Initialize the data (i.e. instruction) packet to be sent serially
    Step 5:  Send the packet serially, and fill the buffer with the data being returned
*******************************************************************************/
ReturnType FlashWriteLockRegister( ST_uint32 udAddr, ST_uint8 ucLockRegister )
{
   CharStream char_stream_send;
   ST_uint8   pIns_Addr[5];

    // Step 1: Validate address
    if(!(udAddr < FLASH_SIZE)) return Flash_AddressInvalid;

    // Step 2: Check whether any previous Write, Program or Erase cycle is still on-going
    if (IsFlashBusy()) return Flash_OperationOngoing;

    // Step 3: Disable Write protection
    FlashWriteEnable();

    // Step 4: Initialize the data (i.e. instruction) packet to be sent serially
    char_stream_send.length   = 5;
    char_stream_send.pChar    = pIns_Addr;
    pIns_Addr[0]              = SPI_FLASH_INS_WRLR;
    pIns_Addr[1]              = udAddr>>16;
    pIns_Addr[2]              = udAddr>>8;
    pIns_Addr[3]              = udAddr;
    pIns_Addr[4]              = ucLockRegister;

    // Step 5: Send the packet serially, and fill the buffer with the data being returned
    Serialize(&char_stream_send,
              ptrNull,
              enumEnableTansRecv_SelectSlave,
              enumDisableTansRecv_DeSelectSlave
              );

   return Flash_Success;

} /* EndFunction FlashWriteLockRegister */

#endif

/*******************************************************************************
Function:     IsFlashBusy( )
Arguments:    none

Return Value:
   TRUE
   FALSE

Description:  This function checks the Write In Progress (WIP) bit to determine whether
              the Flash memory is busy with a Write, Program or Erase cycle.

Pseudo Code:
   Step 1: Read the Status Register.
   Step 2: Check the WIP bit.
*******************************************************************************/
uint8 IsFlashBusy()
{
    ST_uint8 ucSR;

    // Step 1: Read the Status Register.
    FlashReadStatusRegister(&ucSR);
    // Step 2: Check the WIP bit.
    if(ucSR & SPI_FLASH_WIP)
        return TRUE;
    else
        return FALSE;
}

#ifdef VERBOSE
/*******************************************************************************
Function:     FlashErrorStr( ReturnType rErrNum );
Arguments:    rErrNum is the error number returned from other Flash memory Routines

Return Value: A pointer to a string with the error message

Description:  This function is used to generate a text string describing the
   error from the Flash memory. Call with the return value from other Flash memory routines.

Pseudo Code:
   Step 1: Return the correct string.
*******************************************************************************/
ST_sint8 *FlashErrorStr( ReturnType rErrNum )
{
   switch(rErrNum)
   {
   case Flash_AddressInvalid:
      return "Flash - Address is out of Range";
   case Flash_MemoryOverflow:
      return "Flash - Memory Overflows";
   case Flash_PageEraseFailed:
      return "Flash - Page Erase failed";
   case Flash_PageNrInvalid:
      return "Flash - Page Number is out of Range";
   case Flash_SectorNrInvalid:
      return "Flash - Sector Number is out of Range";
  case Flash_FunctionNotSupported:
      return "Flash - Function not supported";
   case Flash_NoInformationAvailable:
      return "Flash - No Additional Information Available";
   case Flash_OperationOngoing:
      return "Flash - Operation ongoing";
   case Flash_OperationTimeOut:
      return "Flash - Operation TimeOut";
   case Flash_ProgramFailed:
      return "Flash - Program failed";
   case Flash_Success:
      return "Flash - Success";
   case Flash_WrongType:
      return "Flash - Wrong Type";
   default:
      return "Flash - Undefined Error Value";
   } /* EndSwitch */
} /* EndFunction FlashErrorString */
#endif /* VERBOSE Definition */


/*******************************************************************************
Function:     FlashTimeOut(ST_uint32 udSeconds)
Arguments:    udSeconds holds the number of seconds before giving a TimeOut

Return Value:
   Flash_OperationTimeOut
   Flash_OperationOngoing

Example:   FlashTimeOut(0)  // Initializes the Timer

           While(1) {
              ...
              If (FlashTimeOut(5) == Flash_OperationTimeOut) break;
              // The loop is executed for 5 Seconds, then timeout occurs
           } EndWhile

*******************************************************************************/
#ifdef TIME_H_EXISTS
/*-----------------------------------------------------------------------------
Description:   This function provides a timeout for Flash memory polling actions or
   other operations that would otherwise never terminate.
   The Routine uses the clock() function inside the ANSI C library "time.h".
-----------------------------------------------------------------------------*/
ReturnType FlashTimeOut(ST_uint32 udSeconds){
   static clock_t clkReset,clkCount;

   if (udSeconds == 0) { /* Set Timeout to 0 */
      clkReset=clock();
   } /* EndIf */

   clkCount = clock() - clkReset;

   if (clkCount<(CLOCKS_PER_SEC*(clock_t)udSeconds))
      return Flash_OperationOngoing;
   else
	  return Flash_OperationTimeOut;
}/* EndFunction FlashTimeOut */

#else
/*-----------------------------------------------------------------------------
Description:   This function provides a timeout for Flash memory polling actions or
   other operations that would otherwise never terminate.
   The Routine uses COUNT_FOR_A_SECOND that gives the number of times a loop has to
   be repeated to generate a one second delay. This value needs to be adapted to the target Hardware.
-----------------------------------------------------------------------------*/
ReturnType FlashTimeOut(ST_uint32 udSeconds) {

   static ST_uint32 udCounter=0;

   if (udSeconds == 0) { /* Set Timeout to 0 */
      udCounter = 0;
   } /* EndIf */

   if (udCounter == (udSeconds * COUNT_FOR_A_SECOND))
   {
      return Flash_OperationTimeOut;

   } else {
      udCounter++;
      return Flash_OperationOngoing;
   } /* Endif */

} /* EndFunction FlashTimeOut */
#endif /* TIME_H_EXISTS */

/*******************************************************************************
 End of C2195.c
*******************************************************************************/
