/********************************************************************************************************
    Filename:       zmac.c
    Revised:        $Date: 2006-11-28 13:47:33 -0800 (Tue, 28 Nov 2006) $
    Revision:       $Revision: 12837 $

    Description:

    This file contains the ZStack MAC Porting Layer

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
 ********************************************************************************************************/


/********************************************************************************************************
 *                                               INCLUDES
 ********************************************************************************************************/

#include "ZComDef.h"
#include "OSAL.h"
#include "ZMAC.h"

/********************************************************************************************************
 *                                                 MACROS
 ********************************************************************************************************/

/********************************************************************************************************
 *                                               CONSTANTS
 ********************************************************************************************************/
static CONST byte TxPowerSettings[] =
{
  OUTPUT_POWER_0DBM,
  OUTPUT_POWER_N1DBM,
  OUTPUT_POWER_N3DBM,
  OUTPUT_POWER_N5DBM,
  OUTPUT_POWER_N7DBM,
  OUTPUT_POWER_N10DBM,
  OUTPUT_POWER_N15DBM,
  OUTPUT_POWER_N25DBM
};

/********************************************************************************************************
 *                                               GLOBALS
 ********************************************************************************************************/
uint32 _ScanChannels;

extern uint8 aExtendedAddress[];

/********************************************************************************************************
 *                                               LOCALS
 ********************************************************************************************************/

/* Pointer to scan result buffer */
void *ZMac_ScanBuf = NULL;

/********************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************/

/********************************************************************************************************
 *                                                TYPEDEFS
 ********************************************************************************************************/


/********************************************************************************************************
 *                                                FUNCTIONS
 ********************************************************************************************************/

/********************************************************************************************************
 * @fn      ZMacInit
 *
 * @brief   Initialize MAC.
 *
 * @param   none.
 *
 * @return  status.
 ********************************************************************************************************/
ROOT uint8 ZMacInit( void )
{
  uint8 stat;

  MAC_Init();
  MAC_InitDevice();

#if defined ( RTR_NWK )
  MAC_InitCoord();
#endif

  // If OK, initialize the MAC
  stat = ZMacReset( TRUE );

  // Turn off interrupts
  osal_int_disable( INTS_ALL );

  return ( stat );
}

/********************************************************************************************************
 * @fn      ZMacUpdate
 *
 * @brief   Gives MAC (or others) some processing time.
 *
 * @param   none.
 *
 * @return  true if CPU needs to keep running (not sleep).
 ********************************************************************************************************/
ROOT byte ZMacUpdate( void )
{
  /* Put code here that needs to run each OSAL event loop */
  return ( false );
}

/********************************************************************************************************
 * @fn      ZMacReset
 *
 * @brief   Reset the MAC.
 *
 * @param   Default to PIB defaults.
 *
 * @return  status.
 ********************************************************************************************************/
ROOT uint8 ZMacReset( bool SetDefaultPIB )
{
  byte stat;
  byte value;

  stat = MAC_MlmeResetReq( SetDefaultPIB );

  // Don't send PAN ID conflict
  value = FALSE;
  MAC_MlmeSetReq( MAC_ASSOCIATED_PAN_COORD, &value );
  MAC_MlmeSetReq( MAC_EXTENDED_ADDRESS, &aExtendedAddress );

  if (ZMac_ScanBuf)
  {
    osal_mem_free(ZMac_ScanBuf);
    ZMac_ScanBuf = NULL;
  }

  return ( stat );
}


/********************************************************************************************************
 * @fn      ZMacGetReq
 *
 * @brief   Read a MAC PIB attribute.
 *
 * @param   attr - PIB attribute to get
 * @param   value - pointer to the buffer to store the attribute
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacGetReq( uint8 attr, uint8 *value )
{
  if ( attr == ZMacExtAddr )
  {
    osal_cpyExtAddr( value, &aExtendedAddress );
    return ZMacSuccess;
  }

  return (ZMacStatus_t) MAC_MlmeGetReq( attr, value );
}


/********************************************************************************************************
 * @fn      ZMacSetReq
 *
 * @brief   Write a MAC PIB attribute.
 *
 * @param   attr - PIB attribute to Set
 * @param   value - pointer to the data
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacSetReq( uint8 attr, byte *value )
{
  if ( attr == ZMacExtAddr )
  {
    osal_cpyExtAddr( &aExtendedAddress, value );
  }

  return (ZMacStatus_t) MAC_MlmeSetReq( attr, value );
}

/********************************************************************************************************
 * @fn      ZMacAssociateReq
 *
 * @brief   Request an association with a coordinator.
 *
 * @param   structure with info need to associate.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacAssociateReq( ZMacAssociateReq_t *pData )
{
  /* Right now, set security to zero */
  pData->Sec.SecurityLevel = false;

  MAC_MlmeAssociateReq ( (macMlmeAssociateReq_t *)pData);
  return ( ZMacSuccess );
}

/********************************************************************************************************
 * @fn      ZMacAssociateRsp
 *
 * @brief   Request to send an association response message.
 *
 * @param   structure with associate response and info needed to send it.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacAssociateRsp( ZMacAssociateRsp_t *pData )
{
  /* Right now, set security to zero */
  pData->Sec.SecurityLevel = false;

  MAC_MlmeAssociateRsp( (macMlmeAssociateRsp_t *) pData );
  return ( ZMacSuccess );
}

/********************************************************************************************************
 * @fn      ZMacDisassociateReq
 *
 * @brief   Request to send a disassociate request message.
 *
 * @param   structure with info need send it.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacDisassociateReq( ZMacDisassociateReq_t *pData )
{
  /* Right now, set security to zero */
  pData->Sec.SecurityLevel = false;

  MAC_MlmeDisassociateReq( (macMlmeDisassociateReq_t *)pData);
  return ( ZMacSuccess );
}

/********************************************************************************************************
 * @fn      ZMacOrphanRsp
 *
 * @brief   Allows next higher layer to respond to an orphan indication message.
 *
 * @param   structure with info need send it.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacOrphanRsp( ZMacOrphanRsp_t *pData )
{
  /* Right now, set security to zero */
  pData->Sec.SecurityLevel = false;

  MAC_MlmeOrphanRsp( (macMlmeOrphanRsp_t *)pData);
  return ( ZMacSuccess );
}

/********************************************************************************************************
 * @fn      ZMacRxEnableReq
 *
 * @brief   This function contains timing information that tells the
 *          device when to enable or disable its receiver, in order
 *          to schedule a data transfer between itself and another
 *          device. The information is sent from the upper layers
 *          directly to the MAC sublayer.
 *
 * @param   structure with info need send it.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacRxEnableReq( ZMacRxEnableReq_t *pData )
{
  /* This feature is no longer in the TIMAC */
  return ( ZMacUnsupported );
}

/********************************************************************************************************
 * @fn      ZMacScanReq
 *
 * @brief   This function is called to perform a network scan.
 *
 * @param   param - structure with info need send it.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacScanReq( ZMacScanReq_t *pData )
{
  _ScanChannels = pData->ScanChannels;

  /* scan in progress */
  if (ZMac_ScanBuf != NULL)
  {
    return MAC_SCAN_IN_PROGRESS;
  }

  if (pData->ScanType != ZMAC_ORPHAN_SCAN)
  {
    /* Allocate memory depends on the scan type */
    if (pData->ScanType == ZMAC_ED_SCAN)
    {
      if ((ZMac_ScanBuf = osal_mem_alloc(ZMAC_ED_SCAN_MAXCHANNELS)) == NULL)
      {
        return MAC_NO_RESOURCES;
      }
      osal_memset(ZMac_ScanBuf, 0, ZMAC_ED_SCAN_MAXCHANNELS);
      pData->Result.pEnergyDetect = ((uint8*)ZMac_ScanBuf) + MAC_CHAN_11;
    }
    else if (pData->MaxResults > 0)
    {
      if ((ZMac_ScanBuf = pData->Result.pPanDescriptor =
           osal_mem_alloc( sizeof( ZMacPanDesc_t ) * pData->MaxResults )) == NULL)
      {
        return MAC_NO_RESOURCES;
      }
    }
  }

  /* Right now, set security to zero */
  pData->Sec.SecurityLevel = false;

  /* Channel Page */
  pData->ChannelPage = 0x00;

  MAC_MlmeScanReq ((macMlmeScanReq_t *)pData);

  return ZMacSuccess;
}


/********************************************************************************************************
 * @fn      ZMacStartReq
 *
 * @brief   This function is called to tell the MAC to transmit beacons
 *          and become a coordinator.
 *
 * @param   structure with info need send it.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacStartReq( ZMacStartReq_t *pData )
{
  uint8 stat;

  // Probably want to keep the receiver on
  stat = true;
  MAC_MlmeSetReq( MAC_RX_ON_WHEN_IDLE, &stat );

  /* Right now, set security to zero */
  pData->RealignSec.SecurityLevel = false;
  pData->BeaconSec.SecurityLevel = false;


  MAC_MlmeStartReq((macMlmeStartReq_t *) pData);

  // MAC does not issue mlmeStartConfirm(), so we have to
  // mlmeStartConfirm( stat );  This needs to be addressed some how

  return ZMacSuccess;
}

/********************************************************************************************************
 * @fn      ZMacSyncReq
 *
 * @brief   This function is called to request a sync to the current
 *          networks beacons.
 *
 * @param   LogicalChannel -
 * @param   TrackBeacon - true/false
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacSyncReq( ZMacSyncReq_t *pData )
{
  MAC_MlmeSyncReq( (macMlmeSyncReq_t *)pData);
  return ZMacSuccess;
}

/********************************************************************************************************
 * @fn      ZMacPollReq
 *
 * @brief   This function is called to request MAC data request poll.
 *
 * @param   coordAddr -
 * @param   coordPanId -
 * @param   SecurityEnable - true or false.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacPollReq( ZMacPollReq_t *pData )
{
  /* Right now, set security to zero */
  pData->Sec.SecurityLevel = false;

  MAC_MlmePollReq ((macMlmePollReq_t *)pData);
  return ( ZMacSuccess );
}

/********************************************************************************************************
 * @fn      ZMacDataReq
 *
 * @brief   Send a MAC Data Frame packet.
 *
 * @param   structure containing data and where to send it.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacDataReq( ZMacDataReq_t *pData )
{
  macMcpsDataReq_t *pBuf;

  /* Allocate memory */
  pBuf = MAC_McpsDataAlloc(pData->msduLength, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE);

  if (pBuf)
  {
    /* Copy the addresses */
    osal_memcpy (&pBuf->mac, pData, sizeof (macDataReq_t));

    /* Copy data */
    pBuf->msdu.len = pData->msduLength;
    osal_memcpy (pBuf->msdu.p, pData->msdu, pData->msduLength);

    /* Right now, set security to zero */
    pBuf->sec.securityLevel = false;

    /* Call Mac Data Request */
    MAC_McpsDataReq(pBuf);

    return ( ZMacSuccess );
  }

  return MAC_NO_RESOURCES;
}

/********************************************************************************************************
 * @fn      ZMacPurgeReq
 *
 * @brief   Purge a MAC Data Frame packet.
 *
 * @param   MSDU data handle.
 *
 * @return  status
 ********************************************************************************************************/
ROOT uint8 ZMacPurgeReq( byte Handle )
{
  MAC_McpsPurgeReq( Handle );
  return ZMacSuccess;
}

/********************************************************************************************************
 * @fn      - ZMACPwrOnReq
 *
 * @brief   - This function requests the MAC to power on the radio hardware
 *            and wake up.  When the power on procedure is complete the MAC
 *            will send a MAC_PWR_ON_CNF to the application.
 *
 * @input   - None.
 *
 * @output  - None.
 *
 * @return  - None.
 ********************************************************************************************************/
void ZMacPwrOnReq ( void )
{
  MAC_PwrOnReq();
}

/********************************************************************************************************
 * @fn          MAC_PwrMode
 *
 * @brief       This function returns the current power mode of the MAC.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      The current power mode of the MAC.
 ********************************************************************************************************/
uint8 ZMac_PwrMode(void)
{
  return (MAC_PwrMode());
}

/********************************************************************************************************
 * @fn      ZMacSetTransmitPower
 *
 * @brief   Set the transmitter power according to the level setting param.
 *
 * @param   Valid power level setting as defined in ZMAC.h.
 *
 * @return  ZMacSuccess if PHY_TRANSMIT_POWER found or ZMacUnsupportedAttribute.
 ********************************************************************************************************/
ROOT uint8 ZMacSetTransmitPower( ZMacTransmitPower_t level )
{
  uint8 pwr;

  if ( (level /= 2) >= 8 )
  {
    pwr = TxPowerSettings[0];
  }
  else
  {
    pwr = TxPowerSettings[level];
  }

  if ( MAC_MlmeSetReq( ZMacPhyTransmitPower, &pwr ) == ZSUCCESS )
  {
    //msupSetTransmitPower();
    return ZMacSuccess;
  }

  return ZMacUnsupportedAttribute;
}

/********************************************************************************************************
 * @fn      ZMacSendNoData
 *
 * @brief   This function sends an empty msg
 *
 * @param   DstAddr   - destination short address
 *          DstPANId  - destination pan id
 *
 * @return  None
 ********************************************************************************************************/
ROOT void ZMacSendNoData ( uint16 DstAddr, uint16 DstPANId )
{
  macMcpsDataReq_t *pBuf;

  /* Allocate memory */
  pBuf = MAC_McpsDataAlloc(0, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE);

  if (pBuf)
  {
    /* Fill in src information */
    pBuf->mac.srcAddrMode              = SADDR_MODE_SHORT;

    /* Fill in dst information */
    pBuf->mac.dstAddr.addr.shortAddr   = DstAddr;
    pBuf->mac.dstAddr.addrMode         = SADDR_MODE_SHORT;
    pBuf->mac.dstPanId                 = DstPANId;

    /* Misc information */
    pBuf->mac.msduHandle               = 0;
    pBuf->mac.txOptions                = ZMAC_TXOPTION_ACK | ZMAC_TXOPTION_NO_RETRANS | ZMAC_TXOPTION_NO_CNF;

    /* Right now, set security to zero */
    pBuf->sec.securityLevel = false;

    /* Call Mac Data Request */
    MAC_McpsDataReq(pBuf);
  }

}
