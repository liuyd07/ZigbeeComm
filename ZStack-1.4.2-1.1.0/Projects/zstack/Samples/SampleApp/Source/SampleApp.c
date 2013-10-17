/*********************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2007-05-31 15:56:04 -0700 (Thu, 31 May 2007) $
  Revision:       $Revision: 14490 $

  Description:
				  - Sample Application (no Profile).
				
          This application isn't intended to do anything useful,
          it is intended to be a simple example of an application's
          structure.

          This application sends it's messages either as broadcast or
          broadcast filtered group messages.  The other (more normal)
          message addressing is unicast.  Most of the other
          sample applications are written to support the unicast
          message model.

          Key control:
            SW1:  Sends a flash command to all devices in Group 1.
            SW2:  Adds/Removes (toggles) this device in and out
                  of Group 1.  This will enable and disable the
                  reception of the flash command.

  Notes:

  Copyright (c) 2007 by Texas Instruments, Inc.
  All Rights Reserved.  Permission to use, reproduce, copy, prepare
  derivative works, modify, distribute, perform, display or sell this
  software and/or its documentation for any purpose is prohibited
  without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"
#include "stdio.h"

#include "NLMEDE.h"
#include "string.h"
/* HAL */
#include "lcd128_64.h"
#include "hal_led.h"
#include "hal_key.h"

#include "Menu.h"
#include "hal.h"
#include "wxl_uart.h"

INT8U LanguageSel = 1;
//uint8 16 = 16;   //注意缓冲区的长度最好调成4的倍数
//uint8 29 = 16 + 13;
//01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

//-----------------------------------------------------------------------------------------
//变量定义
//-----------------------------------------------------------------------------------------
union h{
  uint8 RxBuf[33];                             //修改缓冲！
  struct RFRXBUF
  {
    uint8 HeadCom[3]; //命令头
    uint8 Laddr[8];   //物理地址
    uint8 Saddr[2];   //网络地址
    uint8 DataBuf[20];  //数据缓冲区       //修改缓冲！   注意一定要修改这个地方让接收和发送的缓冲更大些！！
  }RXDATA;
}RfRx;//无线接收缓冲区

union j{
  uint8 TxBuf[33];                             //修改缓冲！
  struct RFTXBUF
  {
    uint8 HeadCom[3]; //命令头
    uint8 Laddr[8];
    uint16 Saddr;
    uint8 DataBuf[20];  //数据缓冲区       //修改缓冲！  注意一定要修改这个地方让接收和发送的缓冲更大些！！
  }TXDATA;
}RfTx;//无线发送缓冲区

// 这个列表列出了应用程序特殊的 Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

struct join
{
  uint8 RfdCount;		//RFD计数器
  uint8 RouterCount;	//路由器计数器
  uint8 RfdAddr[20][10];	//存储RFD节点地址用，高两位为网络地址，低8位为物理地址
  uint8 RouterAddr[20][10];//存储RFD节点地址用，高两位为网络地址，低8位为物理地址
}JoinNode;

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.

endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.

uint8 Menu_Key;
devStates_t SampleApp_NwkState;
uint8 SampleApp_TransID;  // 这是个唯一的消息ID(counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;

aps_Group_t SampleApp_Group;

NLME_LeaveCnf_t* Remove;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;
uint8 state2 = 0;
uint8 arr[20];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendFlashMessage( uint16 flashTime );
uint8 SendData(uint8 *buf, uint16 addr, uint8 Leng);
uint16 get_short_addr(void);

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   应用初始化函数包括了硬件初始化、平台初始化、功耗初始化等
 *
 * @param   task_id - 操作系统分配的任务ID，这个ID用来发送信息和设置时间
 *
 * @return  none
 */
void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

 #if defined ( SOFT_START )
  // The "Demo" target is setup to have SOFT_START and HOLD_AUTO_START
  // SOFT_START is a compile option that allows the device to start
  //  as a coordinator if one isn't found.
  // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
  // together - if they are - we will start up a coordinator. Otherwise,
  // the device will start as a router.
	//选择Zigbee设备协调者
	#ifdef ZG_Coord
		zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
	#endif
	//选择Zigbee设备为路由器
	#ifdef ZG_Router
		zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
	#endif
	//选择Zigbee设备为终端节点
	#ifdef ZG_ENDDEVICE
		zgDeviceLogicalType = ZG_DEVICETYPE_ENDDEVICE;
	#endif
#endif //SOFT_START

#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif

  // 设置通讯的目的地址
  // 广播所有节点
/*  SampleApp_All_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_All_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_All_DstAddr.addr.shortAddr = 0xFFFF;
*/
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

  //配置为单播的发送方式
/*  SampleApp_Single_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  SampleApp_Single_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
*/


  // Setup for the flash command's destination address - Group 1
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;

  // 填充终端类型
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID );

  // By default, all devices start out in Group 1
  SampleApp_Group.ID = 0x0001;
  osal_memcpy( SampleApp_Group.name, "Group 1", 7  );
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );

  //初始化串口
  initUARTtest();
}

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - 操作系统分配的任务ID.
 * @param   events -   事件处理
 *
 * @return  none
 */
extern void halWait(BYTE wait);
extern int Send_Flag;
int Send_Data=0;
extern INT16U SrcSaddr;//发送数据地址
extern int Send_Flag_Consecution;
void Frist_Data(void);
int Pingpong_Data = 0;
int Pingpong_Flag = 0;
int Sensor_Flag = 0;
int count = 0;
extern uint16 ReadBattery(void);
extern uint16 Read_Temp(void);
extern uint8 ReadVoltage0(int portnumber);
uint16 Power_value;
uint16 Temp_value;
uint8 Volt_value;

uint8 rxBuf[16]={0x68,0x00,0x00,0x00,0x00,0x00,0x00,0x68,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x16};//同步指令
uint8 outputnum = 0;

unsigned char status;

uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  //uint8 buffer[4];
  //uint8 buf[5];

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // 按键事件
        case KEY_CHANGE:
        //SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
        //MenuMenuDisp(((keyChange_t *)MSGpkt)->keys);
        //屏蔽菜单目录
        Menu_all(((keyChange_t *)MSGpkt)->keys);
        break;

        // 收到信息事件（接收到射频信息，并将其转到CC2430串口输出）
        case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
          MSGpkt->hdr.event ^= AF_INCOMING_MSG_CMD;
          break;

        //设备加入网络
        case ZDO_STATE_CHANGE:
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( (SampleApp_NwkState == DEV_ZB_COORD)
              || (SampleApp_NwkState == DEV_ROUTER)
              || (SampleApp_NwkState == DEV_END_DEVICE) )
          {
            // 在一个范围内发送一个信息
            Frist_Data();
            state2=10;
            for(int i=0;i<10;i++)
            halWait(200);
            osal_start_timerEx( SampleApp_TaskID,
                              SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                              SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
          }
          else
          {
            // 设备不在网络中
          }
          break;

        default:
          break;
      }

      // 释放存储器
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // 返回未处理事件
    return (events ^ SYS_EVENT_MSG);
  }


  // 发送一个信息 - 这个事件时产生一个时间
  if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
  {
    #if(defined(ZG_Coord))//当为Sink时，周期性发送同步指令
       SendData(rxBuf, 0xFFFF, 16);//广播发送

       //测试程序：串口自发自收
 /*      outputnum = 0;
       while(outputnum<16)
       {
         //接收PC数据
         RfRx.RXDATA.DataBuf[outputnum] = UartRX_Receive_Char();
         outputnum = outputnum + 1;
        }
       URX0IF = 0;//清空串口接收缓存
        //将ARM返回信息发送至PC
        UartTX_Send_String(RfRx.RXDATA.DataBuf,16);
*/
    #endif

    osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                       2000);
    // return unprocessed events
    return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
  }

  // 放弃未知事件
  return 0;
}

uint16 get_short_addr(void)
{
  uint16 Short_addr;
  Short_addr = RfRx.RXDATA.Saddr[0];
  Short_addr <<= 8;
  Short_addr = RfRx.RXDATA.Saddr[1];
  return Short_addr;
}

void Frist_Data(void)
{

#ifdef ZG_Coord
            HalLedBlink( HAL_LED_4, 2, 50, (1000 / 4) );
            ClearScreen();
            Print(3,0,"Found network OK",1);
            halWait(200);
            RfTx.TXDATA.HeadCom[0] = 'l';
            RfTx.TXDATA.HeadCom[1] = 'o';
            RfTx.TXDATA.HeadCom[2] = 'k';
            SendData(RfTx.TxBuf, Send_Mode_Broadcast, 20);            //修改缓冲！！
#endif
#ifdef ZG_Router //路由器
            uint8 *ieeeAddr;
            HalLedBlink( HAL_LED_4, 4, 50, (1000 / 4) );
            ClearScreen();
            Print(3,0,"Join network OK",1);
            Print(0,0,"-----ROUTER-----",1);						
            RfTx.TXDATA.HeadCom[0] = 'J';
            RfTx.TXDATA.HeadCom[1] = 'O';
            RfTx.TXDATA.HeadCom[2] = 'N';
				
            ieeeAddr = NLME_GetExtAddr();
            memcpy(RfTx.TXDATA.Laddr,ieeeAddr,8);
            RfTx.TXDATA.Saddr = NLME_GetShortAddr();
            RfTx.TXDATA.DataBuf[0] = 'R';
            RfTx.TXDATA.DataBuf[1] = 'O';
            RfTx.TXDATA.DataBuf[2] = 'U';
            SendData(RfTx.TxBuf, 0x0000, 33);                         //修改缓冲！！
#endif
#ifdef ZG_ENDDEVICE //终端设备
            uint8 *ieeeAddr;
            HalLedBlink( HAL_LED_4, 3, 50, (1000 / 4) );

            //ClearScreen();
            //Print(3,0,"Join network OK",1);
            //Print(0,4,"---ENDDEVICE----",1);
			
            RfTx.TXDATA.HeadCom[0] = 'J';
            RfTx.TXDATA.HeadCom[1] = 'O';
            RfTx.TXDATA.HeadCom[2] = 'N';
						
            ieeeAddr = NLME_GetExtAddr();
            memcpy(RfTx.TXDATA.Laddr,ieeeAddr,8);
            RfTx.TXDATA.Saddr = NLME_GetShortAddr();

            RfTx.TXDATA.DataBuf[0] = 'R';
            RfTx.TXDATA.DataBuf[1] = 'F';
            RfTx.TXDATA.DataBuf[2] = 'D';
            SendData(RfTx.TxBuf, 0x0000, 33);                        //修改缓冲！！
#endif
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      SampleApp_HandleKeys
 *
 * @brief   手柄和按键的驱动程序
 *
 * @param   shift -  if 是 shift/alt表示为true
 * @param   keys - 位扫描按键事件. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
endPointDesc_t SampleApp_epDesc;
void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
     //MenuMenuDisp(keys);
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   回收数据处理器.  这个函数用来处理输入的数据，数据可能来自于其他设备.
 *          所以建立在不同串ID之上，完成不同的作用.
 *
 * @param   none
 *
 * @return  none
 */
extern void ClearScreenLcd(void);
//uint8 23 = 16 + 7;  地址位最高有3位
//uint8 UartStrSub[5];
uint8 UartStr[27];//修改缓冲！！

void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  ///HalLedBlink( HAL_LED_4, 1, 50, (1000 / 4) );
  P1DIR |= 0x02;
  P1_1 = 0;
  halWait(200);
  P1_1 = 1;
#ifdef	ZG_Coord
	uint8 i, j;
        uint8 HaveFlag;
	memcpy(RfRx.RxBuf,pkt->cmd.Data,33);                   //修改缓冲！！
	if((RfRx.RXDATA.HeadCom[0] == 'J') && (RfRx.RXDATA.HeadCom[1] == 'O') && (RfRx.RXDATA.HeadCom[2] == 'N'))//有新节点加入网络
	{
		if((RfRx.RXDATA.DataBuf[0] == 'R') && (RfRx.RXDATA.DataBuf[1] == 'F') && (RfRx.RXDATA.DataBuf[2] == 'D'))//RFD节点
		{
			for(i=0; i<8; i++)
			{
				JoinNode.RfdAddr[JoinNode.RfdCount][i] = RfRx.RXDATA.Laddr[i];
			}
			for(i=0; i<2; i++)
			{
				JoinNode.RfdAddr[JoinNode.RfdCount][8+i] = RfRx.RXDATA.Saddr[1-i];
			}
				
			for(j=0; j<JoinNode.RfdCount; j++)//判断有无重复加入的节点
			{
				HaveFlag = 1;
				for(i=0; i<8; i++)
				{
					if(JoinNode.RfdAddr[JoinNode.RfdCount][i] != JoinNode.RfdAddr[j][i])
					{
						HaveFlag = 0;
						break;//不是
					}
				}
				if(HaveFlag == 0)continue;
				JoinNode.RfdCount--;//是
				JoinNode.RfdAddr[j][8] = RfRx.RXDATA.Saddr[1];
				JoinNode.RfdAddr[j][9] = RfRx.RXDATA.Saddr[0];	//修改它的网络地址
				break;
			}
			JoinNode.RfdCount++;
		}
		else if((RfRx.RXDATA.DataBuf[0] == 'R') && (RfRx.RXDATA.DataBuf[1] == 'O') && (RfRx.RXDATA.DataBuf[2] == 'U'))//路由节点
		{
			for(i=0; i<8; i++)
			{
				JoinNode.RouterAddr[JoinNode.RouterCount][i] = RfRx.RXDATA.Laddr[i];
			}
			for(i=0; i<2; i++)
			{
				JoinNode.RouterAddr[JoinNode.RouterCount][8+i] = RfRx.RXDATA.Saddr[1-i];
			}
				
			for(j=0; j<JoinNode.RouterCount; j++)//判断有无重复加入的节点
			{
				HaveFlag = 1;
				for(i=0; i<8; i++)
				{
					if(JoinNode.RouterAddr[JoinNode.RouterCount][i] != JoinNode.RouterAddr[j][i])
					{
						HaveFlag = 0;
						break;//不是
					}
				}
				if(HaveFlag == 0)continue;
				JoinNode.RouterCount--;//是
				JoinNode.RouterAddr[j][8] = RfRx.RXDATA.Saddr[1];
				JoinNode.RouterAddr[j][9] = RfRx.RXDATA.Saddr[0];	//修改它的网络地址
				break;
			}
			JoinNode.RouterCount++;
                }
                UartTX_Send_String(RfRx.RxBuf,20);                                     //修改缓冲！！(UartStr,27)
	}
    else
   {
        if (RfRx.RXDATA.HeadCom[0] == 'D')
        {
            if (RfRx.RXDATA.HeadCom[1] == 'R')
            {
              sprintf(UartStr,(char *)"%c%c%c%c%c%c%s",RfRx.RXDATA.HeadCom[1],RfRx.RXDATA.Laddr[0],RfRx.RXDATA.Laddr[1],RfRx.RXDATA.Laddr[2],RfRx.RXDATA.Saddr[0],RfRx.RXDATA.Saddr[1],RfRx.RXDATA.DataBuf);
            }
            else if (RfRx.RXDATA.HeadCom[1] == 'E')
            {
              sprintf(UartStr,(char *)"%c%c%c%c%c%c%s",RfRx.RXDATA.HeadCom[2],RfRx.RXDATA.Laddr[0],RfRx.RXDATA.Laddr[1],RfRx.RXDATA.Laddr[2],RfRx.RXDATA.Saddr[0],RfRx.RXDATA.Saddr[1],RfRx.RXDATA.DataBuf);
            }
        }
        UartStr[26] = '\n';//修改缓冲！！
        UartTX_Send_String(RfRx.RxBuf,20);                                     //修改缓冲！！(UartStr,27)
    }
#endif
#if (defined(ZG_Router) || defined(ZG_ENDDEVICE))
/*
    SendData(RfRx.RxBuf, 0x0000, 16);//发送至Sink
    memcpy(RfRx.RxBuf,pkt->cmd.Data,20);                                          //修改缓冲！！
    if((RfRx.RXDATA.HeadCom[0] == 'l')&&(RfRx.RXDATA.HeadCom[1] == 'o')&&(RfRx.RXDATA.HeadCom[2] == 'k'))
    {
        halWait(200);
        Frist_Data();
    }

    //终端接收到射频信息转发至ARM
    UartTX_Send_String(RfRx.RxBuf,16);
    //读取ARM返回信息，"16"为终止符
    //uint8 temp = 1;
    //uint8 counter = 0;
    outputnum = 0;
    while((outputnum < 20) && (RfRx.RXDATA.DataBuf[outputnum-1] != 0x16))
    {
        RfRx.RXDATA.DataBuf[outputnum] = UartRX_Receive_Char();
        outputnum = outputnum + 1;
    }
    URX0IF = 0;//清空串口接收缓存
    //将ARM返回信息发送至Sink
    SendData(RfRx.RxBuf, 0x0000, 33);//发送至Sink
*/

    //测试程序-2：节点自发自收
    memcpy(RfRx.RxBuf,pkt->cmd.Data,pkt->cmd.DataLength);
    SendData(RfRx.RxBuf, 0x0000, 16);//发送至Sink

    UartTX_Send_String(RfRx.RxBuf,16);
    outputnum = 0;
    while((outputnum < 20) && (RfRx.RXDATA.DataBuf[outputnum-1] != 0x16))
    {
        RfRx.RXDATA.DataBuf[outputnum] = UartRX_Receive_Char();
        outputnum = outputnum + 1;
    }
    URX0IF = 0;//清空串口接收缓存
    //将ARM返回信息发送至Sink
    SendData(RfRx.RXDATA.DataBuf, 0x0000, 20);//发送至Sink
#endif	

}

/*********************************************************************
 * @fn      SampleApp_SendPeriodicMessage
 *
 * @brief   发送一个周期数据
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_SendPeriodicMessage( void )
{
  if ( AF_DataRequest( &SampleApp_Periodic_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_PERIODIC_CLUSTERID,
                       1,
                       (uint8*)&SampleAppPeriodicCounter,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
 * @fn      SampleApp_SendFlashMessage
 *
 * @brief   发送一组闪烁（小灯闪烁的周期）数据.
 *
 * @param   flashTime - 毫秒
 *
 * @return  none
 */
void SampleApp_SendFlashMessage( uint16 flashTime )
{
  uint8 buffer[3];
  buffer[0] = (uint8)(SampleAppFlashCounter++);
  buffer[1] = LO_UINT16( flashTime );
  buffer[2] = HI_UINT16( flashTime );

  if ( AF_DataRequest( &SampleApp_Flash_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_FLASH_CLUSTERID,
                       3,
                       buffer,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

//**********************************************************************
//**以短地址方式发送数据
//buf ::发送的数据
//addr::目的地址
//Leng::数据长度
//********************************************************************
uint8 SendData(uint8 *buf, uint16 addr, uint8 Leng)
{
	afAddrType_t SendDataAddr;
	
	SendDataAddr.addrMode = (afAddrMode_t)Addr16Bit;         //短地址发送
	SendDataAddr.endPoint = SAMPLEAPP_ENDPOINT;
	SendDataAddr.addr.shortAddr = addr;
        if ( AF_DataRequest( &SendDataAddr, //发送的地址和模式
                       &SampleApp_epDesc,   //终端（比如操作系统中任务ID等）
                       2,//发送串ID
                       Leng,
                       buf,
                       &SampleApp_TransID,  //信息ID（操作系统参数）
                       AF_DISCV_ROUTE,
                     //  AF_ACK_REQUEST,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
	{
		return 1;
	}
	else
	{
		return 0;// Error occurred in request to send.
	}
}


//**********************************************************************
//**以扩展（长）地址方式发送数据
//buf ::发送的数据
//addr::目的地址
//Leng::数据长度
/********************************************************************
uint8 SendData(uint8 *buf, uint16 *addr, uint8 Leng)
{
	afAddrType_t SendDataAddr;
	
	SendDataAddr.addrMode = (afAddrMode_t)Addr64Bit;         //长地址发送
	SendDataAddr.endPoint = SAMPLEAPP_ENDPOINT;
	SendDataAddr.addr.shortAddr = addr;
        if ( AF_DataRequest( &SendDataAddr, //发送的地址和模式
                       &SampleApp_epDesc,   //终端（比如操作系统中任务ID等）
                       2,//发送串ID
                       Leng,
                       buf,
                       &SampleApp_TransID,  //信息ID（操作系统参数）
                       AF_DISCV_ROUTE,
                     //  AF_ACK_REQUEST,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
	{
		return 1;
	}
	else
	{
		return 0;// Error occurred in request to send.
	}
}

********************************************************************
*********************************************************************/


//Graduation Test Gao Lao Zhuang Tested Wadnd Wanted Helld