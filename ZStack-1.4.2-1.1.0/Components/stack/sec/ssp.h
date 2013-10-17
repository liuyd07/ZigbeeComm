#ifndef SSP_H
#define SSP_H

/*********************************************************************
    Filename:       ssp.h
    Revised:        $Date: 2007-02-16 19:11:51 -0800 (Fri, 16 Feb 2007) $
    Revision:       $Revision: 13492 $

    Description:

        Security Service Provider (SSP) interface

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define SSP_APPLY  0
#define SSP_REMOVE 1

// Auxiliary header field lengths
#define FRAME_COUNTER_LEN 4

#define SEC_KEY_LEN  16  // 128/8 octets (128-bit key is standard for ZigBee)

// Security Key Indentifiers
#define SEC_KEYID_LINK      0x00
#define SEC_KEYID_NWK       0x01
#define SEC_KEYID_TRANSPORT 0x02
#define SEC_KEYID_LOAD      0x03

// Security Levels
#define SEC_MASK        0x07
#define SEC_NONE        0x00
#define SEC_MIC_32      0x01
#define SEC_MIC_64      0x02

#define SEC_MIC_128     0x03
#define SEC_ENC         0x04
#define SEC_ENC_MIC_32  0x05
#define SEC_ENC_MIC_64  0x06
#define SEC_ENC_MIC_128 0x07

// Key types
#define KEY_TYPE_TC_MASTER  0
#define KEY_TYPE_NWK        1
#define KEY_TYPE_APP_MASTER 2
#define KEY_TYPE_APP_LINK   3

#define SSP_AUXHDR_CTRL      0
#define SSP_AUXHDR_FRAMECNTR 1

#define SSP_AUXHDR_KEYID_MASK     0x03
#define SSP_AUXHDR_KEYID_SHIFT    3
#define SSP_AUXHDR_EXTNONCE_SHIFT 5
#define SSP_AUXHDR_EXTNONCE_BIT   0x01
#define SSP_AUXHDR_LEVEL_MASK     0x07

#define SSP_AUXHDR_MIN_LEN    5
#define SSP_AUXHDR_SEQNUM_LEN 1
#define SSP_AUXHDR_NWK_LEN ( SSP_AUXHDR_MIN_LEN +  \
                             SSP_AUXHDR_SEQNUM_LEN  )
#define SSP_AUXHDR_EXT_LEN ( SSP_AUXHDR_NWK_LEN + Z_EXTADDR_LEN )

#define SSP_MIC_LEN_MAX 16

#define SSP_NONCE_LEN 13

#define SSP_MACDATA_SIZE (uint8)           \
                         ( 1             + \
                           Z_EXTADDR_LEN + \
                           Z_EXTADDR_LEN + \
                           SEC_KEY_LEN   + \
                           SEC_KEY_LEN     )


/*********************************************************************
 * TYPEDEFS
 */

typedef struct
{
  byte keySeqNum;
  byte key[SEC_KEY_LEN];
} nwkKeyDesc;

typedef struct
{
  nwkKeyDesc  active;
  uint32      frameCounter;
} nwkActiveKeyItems;

typedef struct
{
  uint32 inFrmCntr;
  uint32 outFrmCntr;
  byte   masterKey[SEC_KEY_LEN];     // optional!!
  byte   linkKey[SEC_KEY_LEN];
  byte   partnerDevice[Z_EXTADDR_LEN];
} linkKeyDesc;

typedef struct
{
  byte hdrLen;
  byte auxLen;
  byte msgLen;
  byte secLevel;
  byte keyId;
  uint32 frameCtr;
  byte *key;
} ssp_ctx;

typedef struct
{
  uint8* initExtAddr;
  uint8* qeu;
  uint8* rspExtAddr;
  uint8* qev;
  uint8* masterKey;
  uint8* tag1;
  uint8* tag2;
  uint8* key;
} SSP_MacTagData_t;

typedef struct
{
  uint8  dir;
  uint8  secLevel;
  uint8  hdrLen;
  uint8  sduLen;   //service data unit length
  uint8* pdu;      //protocol data unit
  uint8  extAddr[Z_EXTADDR_LEN];
  uint8  keyID;
  uint8* key;
  uint8  keySeqNum;
  uint32 frmCntr;
  uint8  auxLen;
  uint8  micLen;
} SSP_Info_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
//extern uint8 nwkKeyLoaded;
//extern nwkKeyDesc nwkActiveKey;
extern uint32 nwkFrameCounter;
extern byte zgPreConfigKey[SEC_KEY_LEN];

/*********************************************************************
 * FUNCTIONS
 */

/*
 * SSP Initialization
 */
extern void SSP_Init( void );

/*
 * Parse Auxillary Header
 */
extern void SSP_ParseAuxHdr( SSP_Info_t* si );

/*
 * Process Security Information
 */
extern ZStatus_t SSP_Process( SSP_Info_t* si );

/*
 * Process MAC TAG Data - Generate Tags
 */
extern ZStatus_t SSP_GetMacTags( SSP_MacTagData_t* data );

/*
 * Returns Random Bits
 */
extern void SSP_GetTrueRand( byte len, byte *rand );

/*
 * Read the network active key information
 */
extern void SSP_ReadNwkActiveKey( nwkActiveKeyItems *items );

/*
 * Write the network active key information
 */
extern void SSP_WriteNwkActiveKey( nwkActiveKeyItems *items );

/*
 * Get the selected network key
 */
extern byte *SSP_GetNwkKey( byte seqNum );

/*
 * Secure/Unsecure a network PDU
 */
extern ZStatus_t SSP_NwkSecurity(byte ed_flag, byte *msg, byte hdrLen, byte nsduLen);

/*
 * Set the alternate network key
 */
extern void SSP_UpdateNwkKey( byte *key, byte keySeqNum );

/*
 * Make the alternate network key as active
 */
extern void SSP_SwitchNwkKey( byte seqNum );

extern void SSP_BuildNonce( byte *addr, uint32 frameCntr, byte secCtrl, byte *nonce );

extern byte SSP_GetMicLen( byte securityLevel );

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* SSP_H */
