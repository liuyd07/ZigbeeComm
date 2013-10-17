#ifndef MT_MAC_H
#define MT_MAC_H
/*********************************************************************
    Filename:       MT_MAC.h
    Revised:        $Date: 2006-10-11 11:08:17 -0700 (Wed, 11 Oct 2006) $
    Revision:       $Revision: 12249 $

    Description:

        MonitorTest functions for the MAC layer.

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
#include "ZMAC.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#if defined ( MT_MAC_CB_FUNC )

  #define SPI_CMD_MAC_RESET_REQ           0x0081
  #define SPI_CMD_MAC_INIT                0x0082
  #define SPI_CMD_MAC_GET_REQ             0x0087
  #define SPI_CMD_MAC_SET_REQ             0x008c
  #define SPI_CMD_MAC_ASSOCIATE_REQ       0x0084
  #define SPI_CMD_MAC_ASSOCIATE_RSP       0x0085
  #define SPI_CMD_MAC_DISASSOCIATE_REQ    0x0086
  #define SPI_CMD_MAC_SCAN_REQ            0x008b
  #define SPI_CMD_MAC_START_REQ           0x008d
  #define SPI_CMD_MAC_DATA_REQ            0x0083
  #define SPI_CMD_MAC_GTS_REQ             0x0088
  #define SPI_CMD_MAC_ORPHAN_RSP          0x0089
  #define SPI_CMD_MAC_RX_ENABLE_REQ       0x008a
  #define SPI_CMD_MAC_SYNC_REQ            0x008e
  #define SPI_CMD_MAC_POLL_REQ            0x008f
  #define SPI_CMD_MAC_PURGE_REQ           0x0090

  #define SPI_LEN_MAC_INIT                0x00
  #define SPI_RESP_LEN_MAC_INIT           0x01
  #define SPI_RESP_LEN_MAC_GET_REQ        0x01
  #define SPI_RESP_LEN_MAC_DEFAULT        0x01

  //MAC callbacks
  #define SPI_MAC_CB_TYPE                 0x2080

  #define SPI_CB_NWK_SYNC_LOSS_IND        0x2080
  #define SPI_CB_NWK_ASSOCIATE_IND        0x2081
  #define SPI_CB_NWK_ASSOCIATE_CNF        0x2082
  #define SPI_CB_NWK_BEACON_NOTIFY_IND    0x2083
  #define SPI_CB_NWK_DATA_CNF             0x2084
  #define SPI_CB_NWK_DATA_IND             0x2085
  #define SPI_CB_NWK_DISASSOCIATE_IND     0x2086
  #define SPI_CB_NWK_DISASSOCIATE_CNF     0x2087
  #define SPI_CB_NWK_GTS_CNF              0x2088
  #define SPI_CB_NWK_GTS_IND              0x2089
  #define SPI_CB_NWK_ORPHAN_IND           0x208a
  #define SPI_CB_NWK_POLL_CNF             0x208b
  #define SPI_CB_NWK_SCAN_CNF             0x208c
  #define SPI_CB_NWK_COMM_STATUS_IND      0x208d
  #define SPI_CB_NWK_START_CNF            0x208e
  #define SPI_CB_NWK_RX_ENABLE_CNF        0x208f
  #define SPI_CB_NWK_PURGE_CNF            0x2090

  #define SPI_LEN_NWK_ASSOCIATE_IND       0x14          /* Associate Indication */
  #define SPI_LEN_NWK_ASSOCIATE_CNF       0x0E          /* Associate Confirmation */
  #define SPI_LEN_NWK_DISASSOCIATE_IND    0x14          /* Disassociate Indication */
  #define SPI_LEN_NWK_DISASSOCIATE_CNF    0x0c          /* Disassociate Confirmation */
  #define SPI_LEN_NWK_BEACON_NOTIFY_IND   0x63          /* Beacon Notification */
  #define SPI_LEN_NWK_ORPHAN_IND          0x13          /* Orphan Indication */
  #define SPI_LEN_NWK_SCAN_CNF            0x09          /* Scan Confirmation */
  #define SPI_LEN_NWK_SYNC_LOSS_IND       0x10          /* Sync Loss Indication */
  #define SPI_LEN_NWK_COMM_STATUS_IND     0x21          /* Comm Status Indication */
  #define SPI_LEN_NWK_DATA_CNF            0x08          /* Data Confirmation */
  #define SPI_LEN_NWK_DATA_IND            0x2C          /* Data Indication */
  #define SPI_LEN_NWK_PURGE_CNF           0x02          /* Purge Confirmation */

  #define SPI_LEN_MAC_INIT                0x00
  #define SPI_RESP_LEN_MAC_INIT           0x01
  #define SPI_RESP_LEN_MAC_GET_REQ        0x01
  #define SPI_RESP_LEN_NWK_DEFAULT        0x01

  //MAC Callback subscription IDs
  #define CB_ID_NWK_SYNC_LOSS_IND         0x0001
  #define CB_ID_NWK_ASSOCIATE_IND         0x0002
  #define CB_ID_NWK_ASSOCIATE_CNF         0x0004
  #define CB_ID_NWK_BEACON_NOTIFY_IND     0x0008
  #define CB_ID_NWK_DATA_CNF              0x0010
  #define CB_ID_NWK_DATA_IND              0x0020
  #define CB_ID_NWK_DISASSOCIATE_IND      0x0040
  #define CB_ID_NWK_DISASSOCIATE_CNF      0x0080
  //#define CB_ID_NWK_GTS_CNF               0x0100
  //#define CB_ID_NWK_GTS_IND               0x0200
  // reuse GTS value for purge
  #define CB_ID_NWK_PURGE_CNF             0x0100
  #define CB_ID_NWK_ORPHAN_IND            0x0400
  #define CB_ID_NWK_POLL_CNF              0x0800
  #define CB_ID_NWK_SCAN_CNF              0x1000
  #define CB_ID_NWK_COMM_STATUS_IND       0x2000
  #define CB_ID_NWK_START_CNF             0x4000
  #define CB_ID_NWK_RX_ENABLE_CNF         0x8000
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern uint16 _macCallbackSub;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
/*********************************************************************
 * LOCAL FUNCTIONS
 */

#ifdef MT_MAC_FUNC
/*
 *   Process all the MAC commands that are issued by test tool
 */
extern void MT_MacCommandProcessing( uint16 cmd_id , byte len , byte *pDdata );

#endif   /*MAC Command Processing in MT*/


#if defined ( MT_MAC_CB_FUNC )

/*
 *  Process the callback subscription for nwk_associate_ind
 */
extern byte nwk_MTCallbackSubNwkAssociateInd( ZMacAssociateInd_t *param );

/*
 *  Process the callback subscription for nwk_associate_cnf
 */
extern byte nwk_MTCallbackSubNwkAssociateCnf( ZMacAssociateCnf_t *param );

/*
 *  Process the callback subscription for nwk_data_cnf
 */
extern byte nwk_MTCallbackSubNwkDataCnf( ZMacDataCnf_t *param );

/*
 *  Process the callback subscription for nwk_data_ind
 */
extern byte nwk_MTCallbackSubNwkDataInd( ZMacDataInd_t *param );


/*
 * Process the callback subscription for nwk_disassociate_ind
 */
extern byte nwk_MTCallbackSubNwkDisassociateInd( ZMacDisassociateInd_t *param );

/*
 *  Process the callback subscription for nwk_disassociate_cnf
 */
extern byte nwk_MTCallbackSubNwkDisassociateCnf( ZMacDisassociateCnf_t *param );

/*
 *  Process the callback subscription for nwk_orphan_ind
 */
extern byte nwk_MTCallbackSubNwkOrphanInd( ZMacOrphanInd_t *param );

/*
 *  Process the callback subscription for nwk_poll_cnf
 */
extern byte nwk_MTCallbackSubNwkPollCnf( byte Status );

/*
 *  Process the callback subscription for nwk_scan_cnf
 */
extern byte nwk_MTCallbackSubNwkScanCnf( ZMacScanCnf_t *param );

/*
 *  Process the callback subscription for nwk_start_cnf
 */
extern void nwk_MTCallbackSubNwkStartCnf( byte Status );

/*
 *  Process the callback subscription for nwk_syncloss_ind
 */
extern byte nwk_MTCallbackSubNwkSyncLossInd( ZMacSyncLossInd_t *param );

/*
 *  Process the callback subscription for nwk_Rx_Enable_cnf
 */
extern byte nwk_MTCallbackSubNwkRxEnableCnf ( byte Status );

/*
 *  Process the callback subscription for nwk_Comm_Status_ind
 */
extern byte nwk_MTCallbackSubCommStatusInd ( ZMacCommStatusInd_t *param );

/*
 *  Process the callback subscription for nwk_Purge_cnf
 */
extern byte nwk_MTCallbackSubNwkPurgeCnf( ZMacPurgeCnf_t *param );

/*
 *  Process the callback subscription for nwk_Beacon_Notify_ind
 */
extern byte nwk_MTCallbackSubNwkBeaconNotifyInd ( ZMacBeaconNotifyInd_t *param );
#endif

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MT_MAC_H */
