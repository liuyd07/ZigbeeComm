/**************************************************************************************************
    Filename:       i2csupport.c
    Revised:        $Date: 2006-10-19 13:43:54 -0700 (Thu, 19 Oct 2006) $
    Revision:       $Revision: 12357 $

    Description:

    This file contains the I2C interface to off-chip serial EEPROM. While
    much of the driver is generic, certain portions are specific to the
    Atmel part AT24C1024. For example, the 17th address bit occurs in the
    device address byte where one of the address pins should be. Different
    1 Mb parts use a different bit location. This could be abstracted at
    a later time if this is the only differentce among these parts.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/
#include "ioCC2430.h"
#include "zcomdef.h"
#include "i2cSupport.h"
#include "flashutils.h"

#define STATIC  static

// the default cofiguration below usses P1.3 for SDA and P1.2 for SCL.
// change these as needed.
#ifndef OCM_CLK_PORT
  #define OCM_CLK_PORT  1
#endif

#ifndef OCM_DATA_PORT
  #define OCM_DATA_PORT   1
#endif

#ifndef OCM_CLK_PIN
  #define OCM_CLK_PIN   2
#endif

#ifndef OCM_DATA_PIN
  #define OCM_DATA_PIN    3
#endif

// General I/O definitions
#define IO_GIO  0  // General purpose I/O
#define IO_PER  1  // Peripheral function
#define IO_IN   0  // Input pin
#define IO_OUT  1  // Output pin
#define IO_PUD  0  // Pullup/pulldn input
#define IO_TRI  1  // Tri-state input
#define IO_PUP  0  // Pull-up input pin
#define IO_PDN  1  // Pull-down input pin

#define OCM_ADDRESS  (0xA0)
#define OCM_READ     (0x01)
#define OCM_WRITE    (0x00)
#define SMB_ACK      (0)
#define SMB_NAK      (1)
#define SEND_STOP    (0)
#define NOSEND_STOP  (1)
#define SEND_START   (0)
#define NOSEND_START (1)

// device specific as to where the 17th address bit goes...
#define OCM_ADDRESS_BYTE(target,rw)  (OCM_ADDRESS | rw | ((target > 0xFFFF) ? 0x02 : 0x00))

// *************************   MACROS   ************************************
#undef P

  /* I/O PORT CONFIGURATION */
#define CAT1(x,y) x##y  // Concatenates 2 strings
#define CAT2(x,y) CAT1(x,y)  // Forces evaluation of CAT1

// OCM port I/O defintions
// Builds I/O port name: PNAME(1,INP) ==> P1INP
#define PNAME(y,z) CAT2(P,CAT2(y,z))
// Builds I/O bit name: BNAME(1,2) ==> P1_2
#define BNAME(port,pin) CAT2(CAT2(P,port),CAT2(_,pin))

// OCM port I/O defintions
#define OCM_SCL BNAME(OCM_CLK_PORT, OCM_CLK_PIN)
#define OCM_SDA BNAME(OCM_DATA_PORT, OCM_DATA_PIN)

#define IO_DIR_PORT_PIN(port, pin, dir) \
{\
  if ( dir == IO_OUT ) \
    PNAME(port,DIR) |= (1<<(pin)); \
  else \
    PNAME(port,DIR) &= ~(1<<(pin)); \
}

#define OCM_DATA_HIGH()\
{ \
  IO_DIR_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_IN); \
}

#define OCM_DATA_LOW() \
{ \
  IO_DIR_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_OUT); \
  OCM_SDA = 0;\
}

#define IO_FUNC_PORT_PIN(port, pin, func) \
{ \
  if( port < 2 ) \
  { \
    if ( func == IO_PER ) \
      PNAME(port,SEL) |= (1<<(pin)); \
    else \
      PNAME(port,SEL) &= ~(1<<(pin)); \
  } \
  else \
  { \
    if ( func == IO_PER ) \
      P2SEL |= (1<<(pin>>1)); \
    else \
      P2SEL &= ~(1<<(pin>>1)); \
  } \
}

#define IO_IMODE_PORT_PIN(port, pin, mode) \
{ \
  if ( mode == IO_TRI ) \
    PNAME(port,INP) |= (1<<(pin)); \
  else \
    PNAME(port,INP) &= ~(1<<(pin)); \
}

#define IO_PUD_PORT(port, dir) \
{ \
  if ( dir == IO_PDN ) \
    P2INP |= (1<<(port+5)); \
  else \
    P2INP &= ~(1<<(port+5)); \
}

STATIC void   smbSend( uint8 *buffer, uint16 len, uint8 sendStart, uint8 sendStop );
STATIC _Bool  smbSendByte( uint8 dByte );
STATIC void   smbWrite( bool dBit );
STATIC void   smbClock( bool dir );
STATIC void   smbStart( void );
STATIC void   smbStop( void );
STATIC void   smbReceive( uint32 address, uint8 *buffer, uint16 len );
STATIC uint8  smbReceiveByte( void );
STATIC _Bool  smbRead( void );
STATIC void   smbSendDeviceAddress(uint32 address);
STATIC int8   SMBReceive(uint32 address, uint8 *buf, uint16 len);
STATIC int8   SMBSend(uint32 address, uint8 *buf, uint16 len);
STATIC int8   SMBReadWrite(uint8 which, uint32 address, uint8 *buf, uint16 len);


STATIC __near_func void   smbWait( uint8 );

STATIC uint8 s_xmemIsInit;

/*********************************************************************
 * @fn      DF_i2cInit
 * @brief   Initializes two-wire serial I/O bus
 * @param   void
 * @return  void
 */
void DF_i2cInit( int8 (**readWrite)(uint8, uint32, uint8 *, uint16))
{
  if (!s_xmemIsInit)  {
    s_xmemIsInit = 1;

    // Set port pins as inputs
    IO_DIR_PORT_PIN( OCM_CLK_PORT, OCM_CLK_PIN, IO_IN );
    IO_DIR_PORT_PIN( OCM_DATA_PORT, OCM_DATA_PIN, IO_IN );

    // Set for general I/O operation
    IO_FUNC_PORT_PIN( OCM_CLK_PORT, OCM_CLK_PIN, IO_GIO );
    IO_FUNC_PORT_PIN( OCM_DATA_PORT, OCM_DATA_PIN, IO_GIO );

    // Set I/O mode for pull-up/pull-down
    IO_IMODE_PORT_PIN( OCM_CLK_PORT, OCM_CLK_PIN, IO_PUD );
    IO_IMODE_PORT_PIN( OCM_DATA_PORT, OCM_DATA_PIN, IO_PUD );

    // Set pins to pull-up
    IO_PUD_PORT( OCM_CLK_PORT, IO_PUP );
    IO_PUD_PORT( OCM_DATA_PORT, IO_PUP );
  }

  *readWrite = SMBReadWrite;
}

int8 SMBReadWrite(uint8 which, uint32 address, uint8 *buf, uint16 len)
{
  if (which == XMEM_READ)  {
    return SMBReceive(address, buf, len);
  }
  else  {
    return SMBSend(address, buf, len);
  }
}

int8 SMBReceive(uint32 address, uint8 *buf, uint16 len)
{
  smbReceive(address, buf, len);

  return 0;
}

int8 SMBSend(uint32 address, uint8 *buf, uint16 len)
{
  uint16 rollover = (uint16)address;
  uint16  locLen;

  // The AT24C1024 writes 256 byte 'pages'. If the starting address
  // plus the number of bytes to write exceed the 256 byte page boundary
  // the address will wrap and overwrite from the beginning of the page.
  // Presumably this is not what is intended under this scenario. We
  // have to deal with rollover explicitly. If it occurs we need to
  // re-address the target byte so we need to stop and re-start.
  do  {
    if (((rollover+len-1)&0xFF00) != (rollover&0xFF00))  {
      // We're going to exceed the current page. Write only
      // enough bytes to fill the current page.
      locLen = 0x100 - (address & 0xFF);
    }
    else  {
      locLen = len;
    }
    // begin the write sequence with the address byte
    smbSendDeviceAddress(address);
    smbSend(buf, locLen, NOSEND_START, SEND_STOP);
    len      -= locLen;
    address  += locLen;
    rollover += locLen;
    buf      += locLen;
  } while (len);

  return 0;
}
/*********************************************************************
 * @fn      smbSend
 * @brief   Sends buffer contents to SM-Bus device
 * @param   buffer - ptr to buffered data to send
 * @param   len - number of bytes in buffer
 * @param   sendStart - whether or not to send start condition.
 * @param   sendStop - whether or not to send stop condition.
 * @return  void
 */
STATIC void smbSend( uint8 *buffer, uint16 len, uint8 sendStart, uint8 sendStop )
{
  uint16 i;

  if (!len)  {
    return;
  }

  if (sendStart == SEND_START)  {
    smbStart();
  }

  for ( i = 0; i < len; i++ )
  {
    while ( !smbSendByte( buffer[i] ) );  // takes care of ack polling
  }

  if (sendStop == SEND_STOP) {
    smbStop();
  }
}

/*********************************************************************
 * @fn      smbSendByte
 * @brief   Serialize and send one byte to SM-Bus device
 * @param   dByte - data byte to send
 * @return  ACK status - 0=none, 1=received
 */
STATIC _Bool smbSendByte( uint8 dByte )
{
  uint8 i;

  for ( i = 0; i < 8; i++ )
  {
    // Send the MSB
    smbWrite( dByte & 0x80 );
    // Next bit into MSB
    dByte <<= 1;
  }
  // need clock low so if the SDA transitions on the next statement the
  // slave doesn't stop. Also give opportunity for slave to set SDA
  smbClock( 0 );
  OCM_DATA_HIGH(); // set to input to receive ack...
  smbClock( 1 );
  smbWait(1);

  return ( !OCM_SDA );  // Return ACK status
}

/*********************************************************************
 * @fn      smbWrite
 * @brief   Send one bit to SM-Bus device
 * @param   dBit - data bit to clock onto SM-Bus
 * @return  void
 */
STATIC void smbWrite( bool dBit )
{
  smbClock( 0 );
  smbWait(1);
  if ( dBit )
  {
    OCM_DATA_HIGH();
  }
  else
  {
    OCM_DATA_LOW();
  }

  smbClock( 1 );
  smbWait(1);
}

/*********************************************************************
 * @fn      smbClock
 * @brief   Clocks the SM-Bus. If a negative edge is going out, the
 *          I/O pin is set as an output and driven low. If a positive
 *          edge is going out, the pin is set as an input and the pin
 *          pull-up drives the line high. This way, the slave device
 *          can hold the node low if longer setup time is desired.
 * @param   dir - clock line direction
 * @return  void
 */
STATIC void smbClock( bool dir )
{
  if ( dir )
  {
    IO_DIR_PORT_PIN( OCM_CLK_PORT, OCM_CLK_PIN, IO_IN );
  }
  else
  {
    IO_DIR_PORT_PIN( OCM_CLK_PORT, OCM_CLK_PIN, IO_OUT );
    OCM_SCL = 0;
  }
  smbWait(1);
}

/*********************************************************************
 * @fn      smbStart
 * @brief   Initiates SM-Bus communication. Makes sure that both the
 *          clock and data lines of the SM-Bus are high. Then the data
 *          line is set high and clock line is set low to start I/O.
 * @param   void
 * @return  void
 */
STATIC void smbStart( void )
{
  // wait for slave to release clock line...
  // set SCL to input but with pull-up. if slave is pulling down it will stay down.
  smbClock(1);
  while (!OCM_SCL) ; // wait until the line is high...

  // SCL low to set SDA high so the transition will be correct.
  smbClock(0);
  OCM_DATA_HIGH();  // SDA high
  smbClock(1);      // set up for transition
  smbWait(1);
  OCM_DATA_LOW();   // start

  smbWait(1);
  smbClock( 0 );
}

/*********************************************************************
 * @fn      smbStop
 * @brief   Terminates SM-Bus communication. Waits unitl the data line
 *          is low and the clock line is high. Then sets the data line
 *          high, keeping the clock line high to stop I/O.
 * @param   void
 * @return  void
 */
STATIC void smbStop( void )
{
  // Wait for clock high and data low
  smbClock(0);
  OCM_DATA_LOW();  // force low with SCL low
  smbWait(1);

  smbClock( 1 );
  OCM_DATA_HIGH(); // stop condition
  smbWait(1);

}

/*********************************************************************
 * @fn      smbWait
 * @brief   Wastes a an amount of time.
 * @param   count: down count in busy-wait
 * @return  void
 */
STATIC __near_func void smbWait( uint8 count )
{
  while ( count-- );
}

/*********************************************************************
 * @fn      smbReceiveByte
 * @brief   Read the 8 data bits.
 * @param   void
 * @return  character read
 */
STATIC uint8 smbReceiveByte()
{
  int8 i, rval = 0;

  for (i=7; i>=0; --i)  {
    if (smbRead())  {
      rval |= 1<<i;
    }
  }

  return rval;
}
/**************************************************************************************************
**************************************************************************************************/
/*********************************************************************
 * @fn      smbReceive
 * @brief   reads data into a buffer
 * @param   address: linear address on part from which to read
 * @param   buffer: target array for read characters
 * @param   len: max number of characters to read
 * @return  void
 */
STATIC void smbReceive( uint32 address, uint8 *buffer, uint16 len )
{
  uint8  ch;
  uint16 i;

  if (!len)  {
    return;
  }

  smbSendDeviceAddress(address);

  ch = OCM_ADDRESS_BYTE(0, OCM_READ);
  smbSend(&ch, 1, SEND_START, NOSEND_STOP);

  for ( i = 0; i < len-1; i++ )
  {
    // SCL may be high. set SCL low. If SDA goes high when input
    // mode is set the slave won't see a STOP
    smbClock(0);
    OCM_DATA_HIGH();

    buffer[i] = smbReceiveByte();
    smbWrite(SMB_ACK);           // write leaves SCL high
  }

  // condition SDA one more time...
  smbClock(0);
  OCM_DATA_HIGH();
  buffer[i] = smbReceiveByte();
  smbWrite(SMB_NAK);

  smbStop();
}

/*********************************************************************
 * @fn      smbRead
 * @brief   Toggle the clock line to let the slave set the data line.
 *          Then read the data line.
 * @param   void
 * @return  TRUE if bit read is 1 else FALSE
 */
STATIC _Bool smbRead( void )
{
  // SCL low to let slave set SDA. SCL high for SDA
  // valid and then get bit
  smbClock( 0 );
  smbWait(1);
  smbClock( 1 );
  smbWait(1);

  return OCM_SDA;
}

/*********************************************************************
 * @fn      smbSendDeviceAddress
 * @brief   Send onlythe device address. Do ack polling
 *
 * @param   void
 * @return  none
 */
STATIC void smbSendDeviceAddress(uint32 address)
{
  uint8 addr[2];

  addr[0] = (address>>8) & 0xFF;
  addr[1] = address & 0xFF;

  // do ack polling...
  do  {
    smbStart();
  } while (!smbSendByte(OCM_ADDRESS_BYTE(address, OCM_WRITE)));

  // OK. send memory address
  smbSend(addr, 2, NOSEND_START, NOSEND_STOP);
}
