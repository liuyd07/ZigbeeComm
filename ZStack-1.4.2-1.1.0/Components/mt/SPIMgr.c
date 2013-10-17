/***************************************************************************************************
    Filename:       SPIMgr.c
    Revised:        $Date: 2006-10-09 17:35:06 -0700 (Mon, 09 Oct 2006) $
    Revision:       $Revision: 12239 $

    Description:
       This module handles anything dealing with the serial port.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
***************************************************************************************************/


/***************************************************************************************************
 *                                           INCLUDES
 ***************************************************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "hal_uart.h"
#include "MTEL.h"
#include "SPIMgr.h"
#include "OSAL_Memory.h"
#include "wxl_uart.h"
#include "Menu.h"


/***************************************************************************************************
 *                                            MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                           CONSTANTS
 ***************************************************************************************************/

/* State values for ZTool protocal */
#define SOP_STATE      0x00
#define CMD_STATE1     0x01
#define CMD_STATE2     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

/***************************************************************************************************
 *                                            TYPEDEFS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                         GLOBAL VARIABLES
 ***************************************************************************************************/
byte App_TaskID;

/* ZTool protocal parameters */

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
uint8 state;
uint8  CMD_Token[2];
uint8  LEN_Token;
uint8  FSC_Token;
mtOSALSerialData_t  *SPI_Msg;
uint8  tempDataLen;
#endif //ZTOOL

#if defined (ZAPP_P1) || defined (ZAPP_P2)
uint16  SPIMgr_MaxZAppBufLen;
bool    SPIMgr_ZAppRxStatus;
#endif

extern struct join
{
	uint8 RfdCount ;		//RFD计数器
	uint8 RouterCount;	//路由器计数器
	uint8 RfdAddr[20][10];	//存储RFD节点地址用，高两位为网络地址，低8位为物理地址
	uint8 RouterAddr[20][10];//存储RFD节点地址用，高两位为网络地址，低8位为物理地址
}JoinNode;
extern union j{
  uint8 TxBuf[66];
  struct RFTXBUF
  {
        uint8 HeadCom[3]; //命令头
        uint8 Node_type[3];
        uint8 IEEE[8];
        uint16 Saddr;
        uint8 DataBuf[50];  //数据缓冲区
  }TXDATA;
}RfTx;//无线发送缓冲区
uint16 Short_Addr = 0xFFFF;

/***************************************************************************************************
 *                                          LOCAL FUNCTIONS
 ***************************************************************************************************/

/***************************************************************************************************
 * @fn      SPIMgr_Init
 *
 * @brief
 *
 * @param   None
 *
 * @return  None
***************************************************************************************************/
void SPIMgr_Init ()
{
  halUARTCfg_t uartConfig;

  /* Initialize APP ID */
  App_TaskID = 0;

  /* UART Configuration */
  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = SPI_MGR_DEFAULT_BAUDRATE;
  uartConfig.flowControl          = SPI_MGR_DEFAULT_OVERFLOW;
  uartConfig.flowControlThreshold = SPI_MGR_DEFAULT_THRESHOLD;
  uartConfig.rx.maxBufSize        = SPI_MGR_DEFAULT_MAX_RX_BUFF;
  uartConfig.tx.maxBufSize        = SPI_MGR_DEFAULT_MAX_TX_BUFF;
  uartConfig.idleTimeout          = SPI_MGR_DEFAULT_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  uartConfig.callBackFunc         = SPIMgr_ProcessZToolData;
#elif defined (ZAPP_P1) || defined (ZAPP_P2)
  uartConfig.callBackFunc         = SPIMgr_ProcessZAppData;
#else
  uartConfig.callBackFunc         = NULL;
#endif

  /* Start UART */
#if defined (SPI_MGR_DEFAULT_PORT)
  HalUARTOpen (SPI_MGR_DEFAULT_PORT, &uartConfig);
#else
  /* Silence IAR compiler warning */
  (void)uartConfig;
#endif

  /* Initialize for ZApp */
#if defined (ZAPP_P1) || defined (ZAPP_P2)
  /* Default max bytes that ZAPP can take */
  SPIMgr_MaxZAppBufLen  = 1;
  SPIMgr_ZAppRxStatus   = SPI_MGR_ZAPP_RX_READY;
#endif


}

/***************************************************************************************************
 * @fn      MT_SerialRegisterTaskID
 *
 * @brief
 *
 *   This function registers the taskID of the application so it knows
 *   where to send the messages whent they come in.
 *
 * @param   void
 *
 * @return  void
 ***************************************************************************************************/
void SPIMgr_RegisterTaskID( byte taskID )
{
  App_TaskID = taskID;
}

/***************************************************************************************************
 * @fn      SPIMgr_CalcFCS
 *
 * @brief
 *
 *   Calculate the FCS of a message buffer by XOR'ing each byte.
 *   Remember to NOT include SOP and FCS fields, so start at the CMD
 *   field.
 *
 * @param   byte *msg_ptr - message pointer
 * @param   byte len - length (in bytes) of message
 *
 * @return  result byte
 ***************************************************************************************************/
byte SPIMgr_CalcFCS( uint8 *msg_ptr, uint8 len )
{
  byte x;
  byte xorResult;

  xorResult = 0;

  for ( x = 0; x < len; x++, msg_ptr++ )
    xorResult = xorResult ^ *msg_ptr;

  return ( xorResult );
}


#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/***************************************************************************************************
 * @fn      SPIMgr_ProcessZToolRxData
 *
 * @brief   | SOP | CMD  |   Data Length   | FSC  |
 *          |  1  |  2   |       1         |  1   |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to correct place (MT or APP)
 *
 * @param   pBuffer  - pointer to the buffer that contains the data
 *          length   - length of the buffer
 *
 *
 * @return  None
 ***************************************************************************************************/
int Uart_len = 0;
uint8 Uart_Rx_Data[50];
extern uint8 SendData(uint8 *buf, uint16 addr, uint8 Leng);
void SPIMgr_ProcessZToolData ( uint8 port, uint8 event )
{
  int s;
  Uart_len = 0;
#ifdef ZDO_COORDINATOR
  int k,f;
  int new_node_flag = 0;
#endif

  /* Verify events */
  if (event == HAL_UART_TX_FULL)
  {
    // Do something when TX if full
    return;
  }

  if (event & (HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT))
  {
    while (Hal_UART_RxBufLen(SPI_MGR_DEFAULT_PORT))
    {
      HalUARTRead (SPI_MGR_DEFAULT_PORT, &Uart_Rx_Data[Uart_len], 1);         //读取串口数据


    switch (state)
      {
        case SOP_STATE:
          if (Uart_Rx_Data[Uart_len] == SOP_VALUE)
            state = CMD_STATE1;
          break;

        case CMD_STATE1:
          CMD_Token[0] = Uart_Rx_Data[Uart_len];
          state = CMD_STATE2;
          break;

        case CMD_STATE2:
          CMD_Token[1] = Uart_Rx_Data[Uart_len];
          state = LEN_STATE;
          break;

        case LEN_STATE:
          LEN_Token = Uart_Rx_Data[Uart_len];
          if (Uart_Rx_Data[Uart_len] == 0)
            state = FCS_STATE;
          else
            state = DATA_STATE;

          tempDataLen = 0;

          // Allocate memory for the data
          SPI_Msg = (mtOSALSerialData_t *)osal_msg_allocate( sizeof ( mtOSALSerialData_t ) + 2+1+LEN_Token );

          if (SPI_Msg)
          {
            // Fill up what we can
            SPI_Msg->hdr.event = CMD_SERIAL_MSG;
            SPI_Msg->msg = (uint8*)(SPI_Msg+1);
            SPI_Msg->msg[0] = CMD_Token[0];
            SPI_Msg->msg[1] = CMD_Token[1];
            SPI_Msg->msg[2] = LEN_Token;
          }
          else
          {
            state = SOP_STATE;
            return;
          }

          break;

        case DATA_STATE:
            SPI_Msg->msg[3 + tempDataLen++] = Uart_Rx_Data[Uart_len];
            if ( tempDataLen == LEN_Token )
              state = FCS_STATE;
          break;

        case FCS_STATE:

          FSC_Token = Uart_Rx_Data[Uart_len];

          //Make sure it's correct
          if ((SPIMgr_CalcFCS ((uint8*)&SPI_Msg->msg[0], 2 + 1 + LEN_Token) == FSC_Token))
          {
            osal_msg_send( MT_TaskID, (byte *)SPI_Msg );
          }
          else
          {
            // deallocate the msg
            osal_msg_deallocate ( (uint8 *)SPI_Msg);
          }

          //Reset the state, send or discard the buffers at this point
          state = SOP_STATE;

          break;

        default:
         break;

      }
      Uart_len++;
    }
#ifdef ZDO_COORDINATOR

          for(k=0;k<JoinNode.RouterCount;k++)
          {
            for( s=0;s<8;s++)
            {
              if(JoinNode.RouterAddr[k][s] == Uart_Rx_Data[s])          //判断是否有相同地址
              {
                new_node_flag++;                                                      //判断位相同标志加1
              }
              else
              {
                new_node_flag = 0;                                                    //判断位不同，表示地址不同，标志清0
                s += 8;
              }

            }
              if(new_node_flag == 8)
              {
                f = k;
                Short_Addr = JoinNode.RouterAddr[k][9];              //取短地址地位
                k += JoinNode.RouterCount;
                Short_Addr <<= 8;                                     //退出查询
              }
          }
          if(new_node_flag == 8)
          {
            Short_Addr |= JoinNode.RouterAddr[f][8];                 //取短地址高位
            for(s=0;s<(Uart_len - 8);s++)
            {
              RfTx.TXDATA.DataBuf[s] = Uart_Rx_Data[8+s];           //取数据前8位是物理地址这里用ASCII表示
            }
            SendData(RfTx.TXDATA.DataBuf,Short_Addr,Uart_len-8);          //发送数据
          }

#elif defined( ZG_ENDDEVICE)
            for(s=0;s<Uart_len;s++)
            {
              RfTx.TXDATA.DataBuf[s] = Uart_Rx_Data[s];                //取串口接收的数据到发送buf中
            }
            SendData(RfTx.TXDATA.DataBuf,0x0000,Uart_len);            //将所有数据到网管
#else
#endif
  }
}
#endif //ZTOOL

#if defined (ZAPP_P1) || defined (ZAPP_P2)
/***************************************************************************************************
 * @fn      SPIMgr_ProcessZAppRxData
 *
 * @brief   | SOP | CMD  |   Data Length   | FSC  |
 *          |  1  |  2   |       1         |  1   |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to correct place (MT or APP)
 *
 * @param   pBuffer  - pointer to the buffer that contains the data
 *          length   - length of the buffer
 *
 *
 * @return  None
 ***************************************************************************************************/
void SPIMgr_ProcessZAppData ( uint8 port, uint8 event )
{

  osal_event_hdr_t  *msg_ptr;
  uint16 length = 0;
  uint16 rxBufLen  = Hal_UART_RxBufLen(SPI_MGR_DEFAULT_PORT);

  /*
     If maxZAppBufferLength is 0 or larger than current length
     the entire length of the current buffer is returned.
  */
  if ((SPIMgr_MaxZAppBufLen != 0) && (SPIMgr_MaxZAppBufLen <= rxBufLen))
  {
    length = SPIMgr_MaxZAppBufLen;
  }
  else
  {
    length = rxBufLen;
  }

  /* Verify events */
  if (event == HAL_UART_TX_FULL)
  {
    // Do something when TX if full
    return;
  }

  if (event & ( HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT))
  {
    if ( App_TaskID )
    {
      /*
         If Application is ready to receive and there is something
         in the Rx buffer then send it up
      */
      if ((SPIMgr_ZAppRxStatus == SPI_MGR_ZAPP_RX_READY ) && (length != 0))
      {
        /* Disable App flow control until it processes the current data */
         SPIMgr_AppFlowControl ( SPI_MGR_ZAPP_RX_NOT_READY );

        /* 2 more bytes are added, 1 for CMD type, other for length */
        msg_ptr = (osal_event_hdr_t *)osal_msg_allocate( length + sizeof(osal_event_hdr_t) );
        if ( msg_ptr )
        {
          msg_ptr->event = SPI_INCOMING_ZAPP_DATA;
          msg_ptr->status = length;

          /* Read the data of Rx buffer */
          HalUARTRead( SPI_MGR_DEFAULT_PORT, (uint8 *)(msg_ptr + 1), length );

          /* Send the raw data to application...or where ever */
          osal_msg_send( App_TaskID, (uint8 *)msg_ptr );
        }
      }
    }
  }
}

/***************************************************************************************************
 * @fn      SPIMgr_ZAppBufferLengthRegister
 *
 * @brief
 *
 * @param   maxLen - Max Length that the application wants at a time
 *
 * @return  None
 *
 ***************************************************************************************************/
void SPIMgr_ZAppBufferLengthRegister ( uint16 maxLen )
{
  /* If the maxLen is larger than the RX buff, something is not right */
  if (maxLen <= SPI_MGR_DEFAULT_MAX_RX_BUFF)
    SPIMgr_MaxZAppBufLen = maxLen;
  else
    SPIMgr_MaxZAppBufLen = 1; /* default is 1 byte */
}

/***************************************************************************************************
 * @fn      SPIMgr_AppFlowControl
 *
 * @brief
 *
 * @param   status - ready to send or not
 *
 * @return  None
 *
 ***************************************************************************************************/
void SPIMgr_AppFlowControl ( bool status )
{

  /* Make sure only update if needed */
  if (status != SPIMgr_ZAppRxStatus )
  {
    SPIMgr_ZAppRxStatus = status;
  }

  /* App is ready to read again, ProcessZAppData have to be triggered too */
  if (status == SPI_MGR_ZAPP_RX_READY)
  {
    SPIMgr_ProcessZAppData ( SPI_MGR_DEFAULT_PORT, HAL_UART_RX_TIMEOUT );
  }

}

#endif //ZAPP

/***************************************************************************************************
***************************************************************************************************/
