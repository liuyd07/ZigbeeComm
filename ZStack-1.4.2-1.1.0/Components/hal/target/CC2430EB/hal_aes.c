/******************************************************************************
    Filename:       aes.c
    Revised:        $Date: 2007-04-06 12:10:18 -0700 (Fri, 06 Apr 2007) $
    Revision:       $Revision: 13980 $

    Description:    Support for HW/SW AES encryption.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
******************************************************************************/
/******************************************************************************
 * INCLUDES
 */
#include "osal.h"
#include "hal_aes.h"
#include "hal_dma.h"

#ifdef __GNUC__
  #include "avr/include/avr/pgmspace.h"
#endif

/******************************************************************************
 * MACROS
 */

// Support for constant tables in flash
#ifdef __IAR_SYSTEMS_ICC__
#ifdef CC2420DB
  #define  PRGM     __flash
#else
  #define  PRGM     __code
#endif
  #define  IBOX(i)  InvSbox[i]
  #define  MUL2(i)  FFMult2[i]
  #define  MUL3(i)  FFMult3[i]
  #define  POLY(i)  Poly2Power[i]
  #define  POWR(i)  Power2Poly[i]
  #define  RCON(i)  RCon[i]
  #define  SBOX(i)  Sbox[i]
#elif defined __GNUC__
  #define  PRGM     PROGMEM
  #define  IBOX(i)  pgm_read_byte_near(&InvSbox[i])
  #define  MUL2(i)  pgm_read_byte_near(&FFMult2[i])
  #define  MUL3(i)  pgm_read_byte_near(&FFMult3[i])
  #define  POLY(i)  pgm_read_byte_near(&Poly2Power[i])
  #define  POWR(i)  pgm_read_byte_near(&Power2Poly[i])
  #define  RCON(i)  pgm_read_byte_near(&RCon[i])
  #define  SBOX(i)  pgm_read_byte_near(&Sbox[i])
#endif

/******************************************************************************
 * CONSTANTS
 */

#define  BLOCK_LENGTH  128   // Defined by AES
#define  KEY_ZLENGTH   128   // ZigBee only uses 128 bit keys

#define  STATE_BLENGTH  16   // Number of bytes in State
#define  KEY_BLENGTH    16   // Number of bytes in Key

#define  Nb  4   // (BLOCK_LENGTH / 32) = Number of columns in State (also known as Nc)
#define  Nk  4   // (KEY_ZLENGTH / 32) = Number of columns in Key
#define  Nr  10  // Number of Rounds

#define  KEY_EXP_LENGTH  176   // Nb * (Nr+1) * 4


/******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 * LOCAL VARIABLES
 */

const uint8 PRGM FFMult2[256] =  // Multiply by 0x02 Table
{
    0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x0e,0x10,0x12,0x14,0x16,0x18,0x1a,0x1c,0x1e,
    0x20,0x22,0x24,0x26,0x28,0x2a,0x2c,0x2e,0x30,0x32,0x34,0x36,0x38,0x3a,0x3c,0x3e,
    0x40,0x42,0x44,0x46,0x48,0x4a,0x4c,0x4e,0x50,0x52,0x54,0x56,0x58,0x5a,0x5c,0x5e,
    0x60,0x62,0x64,0x66,0x68,0x6a,0x6c,0x6e,0x70,0x72,0x74,0x76,0x78,0x7a,0x7c,0x7e,
    0x80,0x82,0x84,0x86,0x88,0x8a,0x8c,0x8e,0x90,0x92,0x94,0x96,0x98,0x9a,0x9c,0x9e,
    0xa0,0xa2,0xa4,0xa6,0xa8,0xaa,0xac,0xae,0xb0,0xb2,0xb4,0xb6,0xb8,0xba,0xbc,0xbe,
    0xc0,0xc2,0xc4,0xc6,0xc8,0xca,0xcc,0xce,0xd0,0xd2,0xd4,0xd6,0xd8,0xda,0xdc,0xde,
    0xe0,0xe2,0xe4,0xe6,0xe8,0xea,0xec,0xee,0xf0,0xf2,0xf4,0xf6,0xf8,0xfa,0xfc,0xfe,
    0x1b,0x19,0x1f,0x1d,0x13,0x11,0x17,0x15,0x0b,0x09,0x0f,0x0d,0x03,0x01,0x07,0x05,
    0x3b,0x39,0x3f,0x3d,0x33,0x31,0x37,0x35,0x2b,0x29,0x2f,0x2d,0x23,0x21,0x27,0x25,
    0x5b,0x59,0x5f,0x5d,0x53,0x51,0x57,0x55,0x4b,0x49,0x4f,0x4d,0x43,0x41,0x47,0x45,
    0x7b,0x79,0x7f,0x7d,0x73,0x71,0x77,0x75,0x6b,0x69,0x6f,0x6d,0x63,0x61,0x67,0x65,
    0x9b,0x99,0x9f,0x9d,0x93,0x91,0x97,0x95,0x8b,0x89,0x8f,0x8d,0x83,0x81,0x87,0x85,
    0xbb,0xb9,0xbf,0xbd,0xb3,0xb1,0xb7,0xb5,0xab,0xa9,0xaf,0xad,0xa3,0xa1,0xa7,0xa5,
    0xdb,0xd9,0xdf,0xdd,0xd3,0xd1,0xd7,0xd5,0xcb,0xc9,0xcf,0xcd,0xc3,0xc1,0xc7,0xc5,
    0xfb,0xf9,0xff,0xfd,0xf3,0xf1,0xf7,0xf5,0xeb,0xe9,0xef,0xed,0xe3,0xe1,0xe7,0xe5
};

const uint8 PRGM FFMult3[256] =  // Multiply by 0x03 Table
{
    0x00,0x03,0x06,0x05,0x0c,0x0f,0x0a,0x09,0x18,0x1b,0x1e,0x1d,0x14,0x17,0x12,0x11,
    0x30,0x33,0x36,0x35,0x3c,0x3f,0x3a,0x39,0x28,0x2b,0x2e,0x2d,0x24,0x27,0x22,0x21,
    0x60,0x63,0x66,0x65,0x6c,0x6f,0x6a,0x69,0x78,0x7b,0x7e,0x7d,0x74,0x77,0x72,0x71,
    0x50,0x53,0x56,0x55,0x5c,0x5f,0x5a,0x59,0x48,0x4b,0x4e,0x4d,0x44,0x47,0x42,0x41,
    0xc0,0xc3,0xc6,0xc5,0xcc,0xcf,0xca,0xc9,0xd8,0xdb,0xde,0xdd,0xd4,0xd7,0xd2,0xd1,
    0xf0,0xf3,0xf6,0xf5,0xfc,0xff,0xfa,0xf9,0xe8,0xeb,0xee,0xed,0xe4,0xe7,0xe2,0xe1,
    0xa0,0xa3,0xa6,0xa5,0xac,0xaf,0xaa,0xa9,0xb8,0xbb,0xbe,0xbd,0xb4,0xb7,0xb2,0xb1,
    0x90,0x93,0x96,0x95,0x9c,0x9f,0x9a,0x99,0x88,0x8b,0x8e,0x8d,0x84,0x87,0x82,0x81,
    0x9b,0x98,0x9d,0x9e,0x97,0x94,0x91,0x92,0x83,0x80,0x85,0x86,0x8f,0x8c,0x89,0x8a,
    0xab,0xa8,0xad,0xae,0xa7,0xa4,0xa1,0xa2,0xb3,0xb0,0xb5,0xb6,0xbf,0xbc,0xb9,0xba,
    0xfb,0xf8,0xfd,0xfe,0xf7,0xf4,0xf1,0xf2,0xe3,0xe0,0xe5,0xe6,0xef,0xec,0xe9,0xea,
    0xcb,0xc8,0xcd,0xce,0xc7,0xc4,0xc1,0xc2,0xd3,0xd0,0xd5,0xd6,0xdf,0xdc,0xd9,0xda,
    0x5b,0x58,0x5d,0x5e,0x57,0x54,0x51,0x52,0x43,0x40,0x45,0x46,0x4f,0x4c,0x49,0x4a,
    0x6b,0x68,0x6d,0x6e,0x67,0x64,0x61,0x62,0x73,0x70,0x75,0x76,0x7f,0x7c,0x79,0x7a,
    0x3b,0x38,0x3d,0x3e,0x37,0x34,0x31,0x32,0x23,0x20,0x25,0x26,0x2f,0x2c,0x29,0x2a,
    0x0b,0x08,0x0d,0x0e,0x07,0x04,0x01,0x02,0x13,0x10,0x15,0x16,0x1f,0x1c,0x19,0x1a
};

const uint8 PRGM RCon[10] =  // Rcon Table used in Key Schedule generation
{
    0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36
};

const uint8 PRGM Sbox[256] =  // SubBytes Transformation Table
{
    0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76,
    0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0,
    0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15,
    0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75,
    0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84,
    0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF,
    0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8,
    0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2,
    0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73,
    0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB,
    0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79,
    0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08,
    0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A,
    0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E,
    0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF,
    0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16
};


/******************************************************************************
 * GLOBAL VARIABLES
 */

void (*pSspAesEncrypt)( uint8 *, uint8 * ) = (void*)NULL;

/******************************************************************************
 * FUNCTION PROTOTYPES
 */

//
// Internal Functions
//
void aesDmaInit( void );
void RoundKey( uint8 *, uint8 );

//
// Encryption Functions
//
void sspAesEncryptKeyExp( uint8 *, uint8 * );
void sspAesEncryptBasic( uint8 *, uint8 * );

void AddRoundKeySubBytes( uint8 *, uint8 * );
void ShiftRows( uint8 * );
void MixColumns( uint8 * );

/******************************************************************************
 * @fn      aesDmaInit
 *
 * @brief   Initilize DMA for AES engine
 *
 * input parameters
 *
 * @param   None
 *
 * @return  None
 */
void aesDmaInit( void )
{
  halDMADesc_t *ch;

  /* Fill in DMA channel 1 descriptor and define it as input */
  ch = HAL_DMA_GET_DESC1234( HAL_DMA_AES_IN );
  HAL_DMA_SET_DEST( ch, HAL_AES_IN_ADDR );              /* Input of the AES module */
  HAL_DMA_SET_VLEN( ch, HAL_DMA_VLEN_USE_LEN );         /* Using the length field */
  HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_BYTE );   /* One byte is transferred each time */
  HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE );    /* A single byte is transferred each time */
  HAL_DMA_SET_TRIG_SRC( ch, HAL_DMA_TRIG_ENC_DW );      /* Setting the AES module to generate the DMA trigger */
  HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_1 );          /* The address for data fetch is incremented by 1 byte */
  HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_0 );          /* The destination address is constant */
  HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_DISABLE );       /* The DMA complete interrupt flag is not set at completion */
  HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS );          /* Transferring all 8 bits in each byte */
  HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_GUARANTEED );   /* DMA at least every second try. */

  /* Fill in DMA channel 2 descriptor and define it as output */
  ch = HAL_DMA_GET_DESC1234( HAL_DMA_AES_OUT );
  HAL_DMA_SET_SOURCE( ch, HAL_AES_OUT_ADDR );           /* Start address of the segment */
  HAL_DMA_SET_VLEN( ch, HAL_DMA_VLEN_USE_LEN );         /* Using the length field */
  HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_BYTE );   /* One byte is transferred each time */
  HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE );    /* A single byte is transferred each time */
  HAL_DMA_SET_TRIG_SRC( ch, HAL_DMA_TRIG_ENC_UP );      /* Setting the AES module to generate the DMA trigger */
  HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_0 );          /* The address for data fetch is constant */
  HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_1 );          /* The destination address is incremented by 1 byte */
  HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_DISABLE );       /* The DMA complete interrupt flag is not set at completion */
  HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS );          /* Transferring all 8 bits in each byte */
  HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_GUARANTEED );   /* DMA at least every second try. */
}

/******************************************************************************
 * @fn      AesLoadIV
 *
 * @brief   Writes IV into the CC2430
 *
 * input parameters
 *
 * @param   IV  - Pointer to IV.
 *
 * @return  None
 */
void AesLoadIV( uint8 *IV )
{
  halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_AES_IN );;

  /* Modify descriptors for channel 1 */
  HAL_DMA_SET_SOURCE( ch, IV );
  HAL_DMA_SET_LEN( ch, STATE_BLENGTH );

  /* Arm DMA channel 1 */
  HAL_DMA_ARM_CH( HAL_DMA_AES_IN );

  /* Set AES mode */
  AES_SET_ENCR_DECR_KEY_IV( AES_LOAD_IV );

  /* Kick it off, block until AES is ready */
  AES_START();
  while( !(ENCCS & 0x08) );
}

/******************************************************************************
 * @fn      AesLoadKey
 *
 * @brief   Writes the key into the CC2430
 *
 * input parameters
 *
 * @param   AesKey  - Pointer to AES Key.
 *
 * @return  None
 */
void AesLoadKey( uint8 *AesKey )
{
  halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_AES_IN );

  /* Modify descriptors for channel 1 */
  HAL_DMA_SET_SOURCE( ch, AesKey );
  HAL_DMA_SET_LEN( ch, KEY_BLENGTH );

  /* Arm DMA channel 1 */
  HAL_DMA_ARM_CH( HAL_DMA_AES_IN );

  /* Set AES mode */
  AES_SET_ENCR_DECR_KEY_IV( AES_LOAD_KEY );

  /* Kick it off, block until AES is ready */
  AES_START();
  while( !(ENCCS & 0x08) );
}

/******************************************************************************
 * @fn      AesDmaSetup
 *
 * @brief   Sets up DMA of 16 byte block using CC2430 HW aes encryption engine
 *
 * input parameters
 *
 * @param   Cstate  - Pointer to output data.
 * @param   msg_out_len - message out length
 * @param   msg_in  - pointer to input data.
 * @param   msg_in_len - message in length
 *
 * output parameters
 *
 * @param   Cstate  - Pointer to encrypted data.
 *
 * @return  None
 *
 */
void AesDmaSetup( uint8 *Cstate, uint16 msg_out_len, uint8 *msg_in, uint16 msg_in_len )
{
  halDMADesc_t *ch;

  /* Modify descriptors for channel 1 */
  ch = HAL_DMA_GET_DESC1234( HAL_DMA_AES_IN );
  HAL_DMA_SET_SOURCE( ch, msg_in );
  HAL_DMA_SET_LEN( ch, msg_in_len );

  /* Modify descriptors for channel 2 */
  ch = HAL_DMA_GET_DESC1234( HAL_DMA_AES_OUT );
  HAL_DMA_SET_DEST( ch, Cstate );
  HAL_DMA_SET_LEN( ch, msg_out_len );

  /* Arm DMA channels 1 and 2 */
  HAL_DMA_ARM_CH( HAL_DMA_AES_IN );
  HAL_DMA_ARM_CH( HAL_DMA_AES_OUT );
}

/******************************************************************************
 * @fn      HalAesInit
 *
 * @brief   Initilize AES engine
 *
 * input parameters
 *
 * @param   None
 *
 * @return  None
 */
void HalAesInit( void )
{
#if !((defined SOFTWARE_AES) && (SOFTWARE_AES == TRUE)) || ((defined SW_AES_AND_KEY_EXP) && (SW_AES_AND_KEY_EXP == TRUE))
   /* Init DMA channels 1 and 2 */
   aesDmaInit();
#endif
}

/******************************************************************************
 * @fn      ssp_HW_KeyInit
 *
 * @brief   Writes the key into AES engine
 *
 * input parameters
 *
 * @param   AesKey  - Pointer to AES Key.
 *
 * @return  None
 */
void ssp_HW_KeyInit( uint8 *AesKey )
{
  AES_SETMODE(ECB);
  AesLoadKey( AesKey );
}

/******************************************************************************
 * @fn      sspAesEncryptHW
 *
 * @brief   Encrypts 16 byte block using AES encryption engine
 *
 * input parameters
 *
 * @param   AesKey  - Pointer to AES Key.
 * @param   Cstate  - Pointer to input data.
 *
 * output parameters
 *
 * @param   Cstate  - Pointer to encrypted data.
 *
 * @return  None
 *
 */
void sspAesEncryptHW( uint8 *AesKey, uint8 *Cstate )
{
  /* Setup DMA for AES encryption */
  AesDmaSetup( Cstate, STATE_BLENGTH, Cstate, STATE_BLENGTH );
  AES_SET_ENCR_DECR_KEY_IV( AES_ENCRYPT );

  /* Kick it off, block until DMA is done */
  HAL_DMA_CLEAR_IRQ( HAL_DMA_AES_OUT );
  AES_START();
  while( !HAL_DMA_CHECK_IRQ( HAL_DMA_AES_OUT ) );
}

#if (defined SW_AES_AND_KEY_EXP) && (SW_AES_AND_KEY_EXP == TRUE)
/******************************************************************************
 * @fn      sspKeyExpansion
 *
 * @brief   Performs key expansion to create the entire Key Schedule.
 *
 * input parameters
 *
 * @param   AesKey  - Pointer to AES Key.
 * @param   KeyExp  - Pointer Key Expansion buffer.  Size = 176 bytes
 *
 * output parameters
 *
 * @param   KeyExp[]    - Key Schedule
 *
 * @return  None
 *
 */
void sspKeyExpansion( uint8 *AesKey, uint8 *KeyExp )
{
  uint8 temp[4], i, t, rc;

  osal_memcpy( KeyExp, AesKey, 16 );

  rc = 0;     // index to RCon[] table

  for (i=16; i < KEY_EXP_LENGTH; i+=16)
  {
    //  RotByte, SubByte, Rcon
    t= SBOX(KeyExp[i-4]);
    temp[0] = SBOX(KeyExp[i-3]) ^ RCON(rc++);
    temp[1] = SBOX(KeyExp[i-2]);
    temp[2] = SBOX(KeyExp[i-1]);
    temp[3] = t;

    KeyExp[i]   = KeyExp[i-16] ^ temp[0];
    KeyExp[i+1] = KeyExp[i-15] ^ temp[1];
    KeyExp[i+2] = KeyExp[i-14] ^ temp[2];
    KeyExp[i+3] = KeyExp[i-13] ^ temp[3];

    KeyExp[i+4] = KeyExp[i-12] ^ KeyExp[i];
    KeyExp[i+5] = KeyExp[i-11] ^ KeyExp[i+1];
    KeyExp[i+6] = KeyExp[i-10] ^ KeyExp[i+2];
    KeyExp[i+7] = KeyExp[i-9] ^ KeyExp[i+3];

    KeyExp[i+8] = KeyExp[i-8] ^ KeyExp[i+4];
    KeyExp[i+9] = KeyExp[i-7] ^ KeyExp[i+5];
    KeyExp[i+10] = KeyExp[i-6] ^ KeyExp[i+6];
    KeyExp[i+11] = KeyExp[i-5] ^ KeyExp[i+7];

    KeyExp[i+12] = KeyExp[i-4] ^ KeyExp[i+8];
    KeyExp[i+13] = KeyExp[i-3] ^ KeyExp[i+9];
    KeyExp[i+14] = KeyExp[i-2] ^ KeyExp[i+10];
    KeyExp[i+15] = KeyExp[i-1] ^ KeyExp[i+11];
  }
}

/******************************************************************************
 * @fn      sspAesEncryptKeyExp
 *
 * @brief   Performs AES-128 encryption using Key Expansion buffer.  The
 *          plaintext input will be overwritten with the ciphertext output.
 *
 * input parameters
 *
 * @param   KeyExp  - Pointer to Key Expansion buffer.
 * @param   Cstate  - Pointer to plaintext input.
 *
 * output parameters
 *
 * @param   Cstate[]    - Ciphertext output
 *
 * @return  None
 *
 */
void sspAesEncryptKeyExp( uint8 *KeyExp, uint8 *Cstate )
{
  uint8 i, round;

  for (round=0; round < 9; round++)
  {
    AddRoundKeySubBytes( KeyExp, Cstate );
    KeyExp += 16;
    ShiftRows( Cstate );
    MixColumns( Cstate );
  }
  AddRoundKeySubBytes( KeyExp, Cstate );
  KeyExp += 16;
  ShiftRows( Cstate );
  for (i=0; i < 16; i++)  Cstate[i] ^= KeyExp[i];
}
#endif

#if (defined SOFTWARE_AES) && (SOFTWARE_AES == TRUE)
/******************************************************************************
 * @fn      sspAesEncryptBasic
 *
 * @brief   Performs AES-128 encryption without using Key Expansion.  The
 *          plaintext input will be overwritten with the ciphertext output.
 *
 * input parameters
 *
 * @param   AesKey  - Pointer to AES Key.
 * @param   Cstate  - Pointer to plaintext input.
 *
 * output parameters
 *
 * @param   Cstate[]    - Ciphertext output
 *
 * @return  None
 *
 */
void sspAesEncryptBasic( uint8 *AesKey, uint8 *Cstate )
{
  uint8 RKBuff[KEY_BLENGTH];
  uint8 i, round;

  osal_memcpy( RKBuff, AesKey, 16 );

  for (round=0; round < 9; round++)
  {
    AddRoundKeySubBytes( RKBuff, Cstate );
    ShiftRows( Cstate );
    MixColumns( Cstate );
    RoundKey( RKBuff, round );
  }
  AddRoundKeySubBytes( RKBuff, Cstate );
  ShiftRows( Cstate );
  RoundKey( RKBuff, round );
  for (i=0; i < 16; i++)  Cstate[i] ^= RKBuff[i];
}

/******************************************************************************
 * @fn      RoundKey
 *
 * @brief   Generates the next Key Schedule based on the current Key Schedule.
 *
 * input parameters
 *
 * @param   W    - Pointer to the current Key Schedule.
 * @param   rc   - Round counter.
 *
 * output parameters
 *
 * @param   W[]     - Next Key Schedule
 *
 * @return  None
 *
 */
void RoundKey( uint8 *W, uint8 rc )
{
  uint8 temp[4], t;

  //  RotByte, SubByte, Rcon
  t = SBOX(W[12]);
  temp[0] = SBOX(W[13]) ^ RCON(rc);
  temp[1] = SBOX(W[14]);
  temp[2] = SBOX(W[15]);
  temp[3] = t;

  W[0] ^= temp[0];
  W[1] ^= temp[1];
  W[2] ^= temp[2];
  W[3] ^= temp[3];

  W[4] ^= W[0];
  W[5] ^= W[1];
  W[6] ^= W[2];
  W[7] ^= W[3];

  W[8] ^= W[4];
  W[9] ^= W[5];
  W[10] ^= W[6];
  W[11] ^= W[7];

  W[12] ^= W[8];
  W[13] ^= W[9];
  W[14] ^= W[10];
  W[15] ^= W[11];
}
#endif

#if ((defined SOFTWARE_AES) && (SOFTWARE_AES == TRUE)) || ((defined SW_AES_AND_KEY_EXP) && (SW_AES_AND_KEY_EXP == TRUE))
/******************************************************************************
 * @fn      AddRoundKeySubBytes
 *
 * @brief   Performs the AddRoundKey and SubBytes function.
 *
 * input parameters
 *
 * @param   KeySch  - Pointer to the Key Schedule.
 * @param   Cstate  - Pointer to cipher state.
 *
 * output parameters
 *
 * @param   Cstate[]    - updated cipher state
 *
 * @return  None
 *
 */
void AddRoundKeySubBytes( uint8 *KeySch, uint8 *Cstate )
{
  uint8 i;

  for (i=0; i < 16; i++)
  {
    Cstate[i] = SBOX(Cstate[i] ^ KeySch[i]);
  }
}

/******************************************************************************
 * @fn      ShiftRows
 *
 * @brief   Performs the ShiftRows function on the cipher state.
 *
 * input parameters
 *
 * @param   Cstate  - The current cipher state
 *
 * output parameters
 *
 * @param   Cstate[]    - Updated cipher state
 *
 * @return  None
 *
 */
void ShiftRows( uint8 *Cstate )
{
  uint8 temp;

  // Row 0 is not shifted

  // Row 1 is shifted down by 1
  temp = Cstate[1];
  Cstate[1] = Cstate[5];
  Cstate[5] = Cstate[9];
  Cstate[9] = Cstate[13];
  Cstate[13] = temp;

  // Row 2 is shifted down by 2
  temp = Cstate[2];
  Cstate[2] = Cstate[10];
  Cstate[10] = temp;
  temp = Cstate[6];
  Cstate[6] = Cstate[14];
  Cstate[14] = temp;

  // Row 3 is shifted down by 3
  temp = Cstate[3];
  Cstate[3] = Cstate[15];
  Cstate[15] = Cstate[11];
  Cstate[11] = Cstate[7];
  Cstate[7] = temp;
}

/******************************************************************************
 * @fn      MixColumns
 *
 * @brief   Performs the MixColumns function on the cipher state.
 *
 * input parameters
 *
 * @param   Cstate  - The current cipher state
 *
 * output parameters
 *
 * @param   Cstate[]    - Updated cipher state
 *
 * @return  None
 *
 */
void MixColumns( uint8 *Cstate )
{
  uint8 r, c, t[4];

  for (c=0; c < 16; c+=4)
  {
    for (r=0; r < 4; r++)  t[r] = Cstate[c+r];

    Cstate[c]   = MUL2(t[0]) ^ MUL3(t[1]) ^ t[2] ^ t[3];
    Cstate[c+1] = MUL2(t[1]) ^ MUL3(t[2]) ^ t[3] ^ t[0];
    Cstate[c+2] = MUL2(t[2]) ^ MUL3(t[3]) ^ t[0] ^ t[1];
    Cstate[c+3] = MUL2(t[3]) ^ MUL3(t[0]) ^ t[1] ^ t[2];
  }
}
#endif

