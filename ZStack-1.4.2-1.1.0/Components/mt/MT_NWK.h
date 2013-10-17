/*********************************************************************
    Filename:       MT_NWK.h
    Revised:        $Date: 2006-04-06 08:19:08 -0700 (Thu, 06 Apr 2006) $
    Revision:       $Revision: 10396 $

    Description:

        MonitorTest functions for the NWK layer.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/


/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"


/*********************************************************************
 * MACROS
 */
#define NWKCB_CHECK(cbi) (_nwkCallbackSub & (cbi))

/*********************************************************************
 * CONSTANTS
 */
#define SPI_CMD_NWK_INIT                0x0100
#define SPI_CMD_NLDE_DATA_REQ           0x0101
#define SPI_CMD_NLME_INIT_COORD_REQ     0x0102
#define SPI_CMD_NLME_PERMIT_JOINING_REQ 0x0103
#define SPI_CMD_NLME_JOIN_REQ           0x0104
#define SPI_CMD_NLME_LEAVE_REQ          0x0105
#define SPI_CMD_NLME_RESET_REQ          0x0106
#define SPI_CMD_NLME_RX_STATE_REQ       0x0107
#define SPI_CMD_NLME_GET_REQ            0x0108
#define SPI_CMD_NLME_SET_REQ            0x0109
#define SPI_CMD_NLME_PING_REQ           0x010A
#define	SPI_CMD_NLME_NWK_DISC_REQ       0x010B
#define SPI_CMD_NLME_ROUTE_DISC_REQ     0x010C
#define SPI_CMD_NLME_DIRECT_JOIN_REQ    0x010D
#define	SPI_CMD_NLME_ORPHAN_JOIN_REQ    0x010E
#define SPI_CMD_NLME_START_ROUTER_REQ   0x010F

#define SPI_RESP_LEN_NWK_DEFAULT        0x01

#define SPI_NWK_CB_TYPE                 0x0180

#define SPI_CB_NLDE_DATA_CNF            0x0180
#define SPI_CB_NLDE_DATA_IND            0x0181
#define SPI_CB_NLME_INITCOORD_CNF       0x0182
#define SPI_CB_NLME_JOIN_CNF            0x0183
#define SPI_CB_NLME_JOIN_IND            0x0184
#define SPI_CB_NLME_LEAVE_CNF           0x0185
#define SPI_CB_NLME_LEAVE_IND           0x0186
#define SPI_CB_NLME_POLL_CNF            0x0187
#define SPI_CB_NLME_SYNC_IND            0x0189
#define SPI_CB_NLME_PING_CNF            0x018C
#define SPI_CB_NLME_NWK_DISC_CNF        0x018D
#define SPI_CB_NLME_START_ROUTER_CNF    0x018F

//NWK Callback subscription IDs
#define CB_ID_NLDE_DATA_CNF          0x0001
#define CB_ID_NLDE_DATA_IND          0x0002
#define CB_ID_NLME_INIT_COORD_CNF    0x0004
#define CB_ID_NLME_JOIN_CNF          0x0008
#define CB_ID_NLME_JOIN_IND          0x0010
#define CB_ID_NLME_LEAVE_CNF         0x0020
#define CB_ID_NLME_LEAVE_IND         0x0040
#define CB_ID_NLME_POLL_CNF          0x0080
#define CB_ID_NLME_SYNC_IND          0x0200
#define CB_ID_NLME_NWK_DISC_CNF      0x2000
#define CB_ID_NLME_START_ROUTER_CNF	 0x8000


#define NWK_DEFAULT_GET_RESPONSE_LEN  10

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern uint16 _nwkCallbackSub;

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

#ifdef MT_NWK_FUNC
/*
 *   Process all the NWK commands that are issued by test tool
 */
extern void MT_NwkCommandProcessing( uint16 cmd_id , byte len , byte *pData );

#endif   /*NWK Command Processing in MT*/

#ifdef MT_NWK_CB_FUNC

/*
 * Process the callback subscription for NLDE-DATA.confirm
 */
extern void nwk_MTCallbackSubDataConfirm(byte nsduHandle, ZStatus_t status );

/*
 * Process the callback subscription for NLDE-DATA.indication
 */
extern void nwk_MTCallbackSubDataIndication( uint16 SrcAddress, int16 nsduLength,
                                      byte *nsdu, byte LinkQuality );

/*
 * Process the callback subscription for NLME-INIT-COORD.confirm
 */
extern void nwk_MTCallbackSubInitCoordConfirm( ZStatus_t Status );

/*
 * Process the callback subscription for NLME-START-ROUTER.confirm
 */
extern void nwk_MTCallbackSubStartRouterConfirm( ZStatus_t Status );

/*
 * Process the callback subscription for NLME_NWK-DISC.confirm
 */
extern void nwk_MTCallbackSubNetworkDiscoveryConfirm( byte ResultCount,
																									networkDesc_t *NetworkList );

/*
 * Process the callback subscription for NLME-JOIN.confirm
 */
extern void nwk_MTCallbackSubJoinConfirm(  uint16 PanId, ZStatus_t Status );

/*
 * Process the callback subscription for NLME-INIT-COORD.indication
 */
extern void nwk_MTCallbackSubJoinIndication( uint16 ShortAddress, byte *ExtendedAddress,
                                      byte CapabilityInformation );

/*
 * Process the callback subscription for NLME-LEAVE.confirm
 */
extern void nwk_MTCallbackSubLeaveConfirm( byte *DeviceAddress, ZStatus_t Status );

/*
 * Process the callback subscription for NLME-LEAVE.indication
 */
extern void nwk_MTCallbackSubLeaveIndication( byte *DeviceAddress );

/*
 *  Process the callback subscription for NLME-SYNC.indication
 */
extern void nwk_MTCallbackSubSyncIndication( void );

/*
 *  Process the callback subscription for NLME-POLL.confirm
 */
extern void nwk_MTCallbackSubPollConfirm( byte status );

#endif   /*NWK Callback Processing in MT*/
/*
 * Process the callback for Ping
 *
 * @MT SPI_CB_NLME_PING_CNF
 *
 */
extern void nwk_MTCallbackPingConfirm( uint16 DstAddress, byte pingSeqNo,
              uint16 delay, byte routeCnt, byte *routeAddr );

/*********************************************************************
*********************************************************************/
