#include "app_ex.h"
#include "hal.h"
#include "lcd128_64.h"
#include "string.h"
#include "Font.h"
#include "Menu.h"
#include "hal_key.h"
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"
#include "stdio.h"
#include "SampleApp.h"
#include "SampleAppHw.h"
#include "wxl_uart.h"

#include  "hal_types.h"
INT8U NowItem = 0;
INT8U FirstItem = 0;
INT8U TopDisp = 1;
//uint8 16 = 16;   //注意缓冲区的长度最好调成4的倍数
//uint8 29 = 16 + 13;

unsigned char MenuItem[13][17];					//菜单缓冲区
extern INT8U ContrastValue;


extern struct join
{
	uint8 RfdCount;		//RFD计数器
	uint8 RouterCount;	//路由器计数器
	uint8 RfdAddr[20][10];	//存储RFD节点地址用，高两位为网络地址，低8位为物理地址
	uint8 RouterAddr[20][10];//存储RFD节点地址用，高两位为网络地址，低8位为物理地址
}JoinNode;

extern union j{
  uint8 TxBuf[33];                                         //修改缓冲！！

  struct RFTXBUF
  {
        uint8 HeadCom[3]; //命令头
        uint8 Laddr[8];
        uint16 Saddr;
        uint8 DataBuf[20];  //数据缓冲区                   //修改缓冲！！
  }TXDATA;
}RfTx;//无线发送缓冲区

void Page1Display(INT8U ss, INT8U tt);
INT8U  DrawMenu(INT8U MenuItem[][17] , INT8U MenuNo,uint8 key);
void MenuMenuDisp( uint8 keys );
void Menu_all(uint8 key);
void Zigbee_Network(INT8U key);
void Aboat_Driver(uint8 key);
void IEEE_menu(void);
void Short_menu(void);
void Zigbee_Network(INT8U key);
INT8U Ten_to_16(INT8U Source_Data);
void Send_Test(INT8U key);
void Send_Broadcast(uint8 key);
void Send_Option(uint8 key);
extern uint8 SendData(uint8 *buf, uint16 addr, uint8 Leng);
void ClearScreenLcd(void);
void SelectTxAddr(INT8U key);
void Uart_Menu(INT8U key);
void Sensor_Menu(INT8U key);
void Uart_Send(INT8U key);
void Uart_RX(INT8U key);
void Uart_Pingpong(INT8U key);
uint16 ReadBattery(void);
uint16 Read_Temp(void);
uint8 ReadVoltage0(int portnumber);
void Temp_menu(INT8U key);
void ReadBattery_menu(INT8U key);
extern void SingleTxTest(uint8 key);
void PingpongTxTest(uint8 key);
extern void ConsecutionTxTest(uint8 key);
extern void halWait(BYTE wait);

INT8U disp_ieee[17],Short_Add[7];
int Menu_option = Aboat_Menu_2;
int Uart_Rx_Flag = 0;
int Uart_PP_Flag = 0;
int Send_Flag;
int Send_Flag_Consecution = 0;
#ifdef ZG_Coord
INT16U SrcSaddr = 0xffff;//发送数据地址
#else
INT16U SrcSaddr = 0x0000;//发送数据地址
#endif


void ClearScreenLcd(void)
{
  ClearScreen();
#ifdef	ZG_Coord
  Print(0,4,"-----COORD-----",1);
#endif	
#ifdef	ZG_Router
  Print(0,0,"-----ROUTER-----",1);
#endif
#ifdef	ZG_ENDDEVICE
  Print(0,4,"---ENDDEVICE----",1);
#endif
}
//*********************************************************************************
//函数名:INT8U  DrawMenu(INT8U MenuItem[][16] , INT8U MenuNo)
//功能:菜单显示
//输入：无
//输出：无
//*********************************************************************************
INT8U  DrawMenu(INT8U MenuItem[][17] , INT8U MenuNo,uint8 key)
{
	INT8U ii;
        INT8U sel;
	ClearScreenLcd();
        if(HAL_KEY_UP == key)
	{
		if(NowItem > 0) NowItem -= 1;
		else if(FirstItem > 0) FirstItem -= 1;
	}
	if(HAL_KEY_DOWN == key)
	{
		if(MenuNo > 2)
		{
			if(NowItem < 2) 	NowItem += 1;
			else if((FirstItem + 3) < MenuNo) 	FirstItem += 1;
		}
		else
		{
			if(NowItem < 1) 	NowItem += 1;	
		}
	}
	if(HAL_KEY_ENTER == key)
	{
		sel = FirstItem + NowItem;
		
	}
	if(HAL_KEY_CANCEL == key)
	{
		sel = 0xff;
	}
		if(MenuNo > 2)
		{
			for(ii = 0 ; ii < 3 ; ii++)
			{
				if(NowItem != ii)
				{				
					Print((ii*2+2),0,MenuItem[FirstItem + ii],1);
				}
				else
				{	
					Print((ii*2+2),0,MenuItem[FirstItem + ii],0);				
				}
			}
		}
		else
		{
			for(ii = 0 ; ii < MenuNo ; ii++)
			{
				if(NowItem != ii)
				{				
					Print((ii*2+2),0,MenuItem[FirstItem + ii],1);
				}
				else
				{	
					Print((ii*2+2),0,MenuItem[FirstItem + ii],0);				
				}
			}
		}
		
        return sel;
}
//*********************************************************************************
//函数名:void MenuMenuDisp( void )
//功能:主菜单显示
//输入：无
//输出：无
//*********************************************************************************
void MenuMenuDisp( uint8 key )
{
    INT8U sel;
    strcpy((char*)MenuItem[0] ,"1:ZigBee Test   ");
    strcpy((char*)MenuItem[1] ,"2:Uart Test     ");
#ifndef ZG_Coord
    strcpy((char*)MenuItem[2] ,"3:Sensor Test   ");
    strcpy((char*)MenuItem[3] ,"4:About         ");
    sel = DrawMenu(MenuItem , 4,key);
#else
    strcpy((char*)MenuItem[2] ,"3:About         ");
    sel = DrawMenu(MenuItem , 3,key);
#endif


    halWait(5);
    switch(sel)
    {
      case 0:
      {
        Menu_option = Zigbee_Menu_2;
        Menu_all(0);
        //Zigbee_Network(key);
      }break;
      case 1:
      {
        Menu_option = Uart_Menu_2;
        FirstItem = 0;
        NowItem = 0;
        Menu_all(0);
			//stop_watch_main();
      }break;
#ifndef ZG_Coord
      case 2:
      {

	Menu_option = Sensor_Menu_2;

        NowItem = 0;
        Menu_all(0);
      }break;
      case 3:
      {
	Menu_option = Aboat_Menu_2;
        Menu_all(0);			//clockmodes_main();
      }break;
#else
      case 2:
      {
	Menu_option = Aboat_Menu_2;
        Menu_all(0);			//clockmodes_main();
      }break;
#endif
      case 0xFF:
      {
        Menu_all(0);
      }break;
      default:
      break;
    }
}

//*********************************************************************************
//函数名:void Menu_all(uint8 key)
//功能:菜单选择
//输入：无
//输出：无
//*********************************************************************************


void Menu_all(uint8 key)
{
  if(Menu_option == Main_Menu_1)
  {
    MenuMenuDisp(key);
  }
  else if(Menu_option == Zigbee_Menu_2)
  {
    Zigbee_Network(key);
  }
  else if(Menu_option == Aboat_Menu_2)
  {
    Aboat_Driver(key);
  }
  else if(Menu_option == Send_Menu_3)
  {
    Send_Option(key);
  }
  else if((Menu_option == Single_Send_Menu_4)||(Menu_option == Continuous_Send_Menu_4))
  {
    Send_Test(key);
  }
  else if(Menu_option == Send_Broadcast_Menu_5)
  {
    Send_Broadcast(key);
  }
  else if(Menu_option == Pingpong_Test_Menu_5)
  {
    PingpongTxTest(key);
  }
  else if(Menu_option == Nod_Menu_3)
  {
    SelectTxAddr(key);
  }
  else if(Menu_option == Send_Short_Addr_Menu_5)
  {
    SingleTxTest(key);
  }
  else if(Menu_option == Uart_Menu_2)
  {
    Uart_Menu(key);
  }
  else if(Menu_option == Uart_TX_Menu_3)
  {
    Uart_Send(key);
  }
  else if(Menu_option == Uart_RX_Menu_3)
  {
    Uart_RX(key);
  }
  else if(Menu_option == Uart_PP_Menu_3)
  {
    Uart_Pingpong(key);
  }
#ifndef ZG_Coord
  else if(Menu_option == Sensor_ReadBattery_Menu_3)
  {
    ReadBattery_menu(key);
  }
  else if(Menu_option == Sensor_Menu_2)
  {
    Sensor_Menu(key);
  }
  else if(Menu_option == Sensor_Temp_Menu_3)
  {
    Temp_menu(key);
  }
#endif
}
//*********************************************************************************
//函数名:void Zigbee_Network(INT8U key)
//功能:ZIGBEE菜单
//输入：无
//输出：无
//*********************************************************************************
void Zigbee_Network(INT8U key)
{
  INT8U sel;
   strcpy((char*)MenuItem[0] ,"1:Send Test     ");
    strcpy((char*)MenuItem[1] ,"2:Network nod   ");
    sel = DrawMenu(MenuItem ,2,key);
      switch(sel)
      {
        case 0:
          Menu_option = Send_Menu_3;
          Menu_all(0);
          //SelectTxAddr(key);//选择地址
        break;
        case 1:
          Menu_option = Nod_Menu_3;

          NowItem = 0;
          Menu_all(0);
        break;
        case 0xff:
          Menu_option = Main_Menu_1;
          FirstItem = 0;
          NowItem = 0;
          Menu_all(0);
          break;
      }
}
//*********************************************************************************
//函数名:void Send_Option(uint8 key)
//功能:发送菜单选项
//输入：无
//输出：无
//*********************************************************************************
void Send_Option(uint8 key)
{
  INT8U sel;
  strcpy((char*)MenuItem[0] ,"1:Single Send   ");
  strcpy((char*)MenuItem[1] ,"2:Continuous    ");
  sel = DrawMenu(MenuItem ,2,key);
  switch(sel)
  {
    case 0:
      Send_Flag = Send_Flag_Single;
      NowItem = 0;
      Menu_option = Single_Send_Menu_4;
      Menu_all(0);
     break;
    case 1:
      Send_Flag = Send_Flag_Continuous;
      NowItem = 0;
      Menu_option = Continuous_Send_Menu_4;
      Menu_all(0);
    break;
    case 0xff:
      Menu_option = Zigbee_Menu_2;
      FirstItem = 0;
      NowItem = 0;
      Menu_all(0);
    break;
  }
}
//*********************************************************************************
//函数名:void Send_Test(INT8U key)
//功能:发送测试菜单
//输入：无
//输出：无
//*********************************************************************************
void Send_Test(INT8U key)
{
  INT8U sel;
   strcpy((char*)MenuItem[0] ,"1:Short Addr    ");                 //短地址（网络地址）发送
   strcpy((char*)MenuItem[1] ,"2:Broadcast Send");                 //广播发送
#ifdef ZG_Coord
   strcpy((char*)MenuItem[2] ,"3:PingPong  Send");                 //乒乓测试
   sel = DrawMenu(MenuItem ,3,key);
#else
   sel = DrawMenu(MenuItem ,2,key);
#endif
      switch(sel)
      {
        case 0:
          Menu_option = Send_Short_Addr_Menu_5;
          Menu_all(0);
        break;
        case 1:
          Menu_option = Send_Broadcast_Menu_5;
          Menu_all(0);
        break;
        case 2:
          Menu_option = Pingpong_Test_Menu_5;
          Menu_all(0);
        break;
        case 0xff:
          Menu_option = Send_Menu_3;
          FirstItem = 0;
          NowItem = 0;
          Menu_all(0);
          break;
      }
}
//*********************************************************************************
//函数名:void Send_Broadcast(uint8 key)
//功能:广播方式发送
//输入：无
//输出：无
//*********************************************************************************
void Send_Broadcast(uint8 key)
{
  if(Send_Flag == Send_Flag_Single)
  {
    SendData("hello", Send_Mode_Broadcast, 5);
    Menu_option = Single_Send_Menu_4;
    FirstItem = 0;
    NowItem = 0;

  }
  else if(Send_Flag == Send_Flag_Continuous)
  {
    ClearScreenLcd();
    //Rectangle(0,2,127,7);
    Print(3,3,"Consecution Tx",1);
    Send_Flag_Consecution = Broadcast_Continuous;
    FirstItem = 0;
    NowItem = 0;
    Menu_option = Send_Broadcast_Menu_5;
  }
  if(key == HAL_KEY_CANCEL)
  {
    Menu_option = Single_Send_Menu_4;
    Send_Flag_Consecution = 0;
    FirstItem = 0;
    NowItem = 0;
    Menu_all(0);
  }
}

void Aboat_Driver(uint8 key)
{
  ClearScreen();
  Print(0,0,"Name:",1);
  Print(4,0,"IEEE Add:(Hex)",1);
  Print(2,0,"Short Add:",1);
    Short_menu();
  Print(2,80,Short_Add,1);
  IEEE_menu();
  Print(6,0,disp_ieee,1);
#ifdef ZG_Coord
  Print(0,40,"Coord",1);
#endif
#ifdef ZG_Router
  Print(0,40,"Router",1);
#endif
#ifdef ZG_ENDDEVICE
  Print(0,40,"END",1);
#endif
  if(key == HAL_KEY_CANCEL)
  {
    Menu_option = Main_Menu_1;
    FirstItem = 0;
    NowItem = 0;
    Menu_all(0);
  }
}

void IEEE_menu(void)
{
  int j;
  INT8U TEMP[16],ieee[8],*ABC = NLME_GetExtAddr() ;
  for(j=0;j<8;j++)
  {
    ieee[j] = *ABC++;
  }
for(j=0;j<8;j++)
  {
    TEMP[2*j]= ieee[j]%16;
    TEMP[2*j] = Ten_to_16(TEMP[2*j]);
  }
for(j=0;j<8;j++)
  {
    TEMP[2*j+1]= ieee[j] / 16;
    TEMP[2*j+1] = Ten_to_16(TEMP[2*j+1]);
  }

  for(j=15;j>=0;j--)
  {
    disp_ieee[15-j] = TEMP[j] ;
  }
  disp_ieee[16] = '\0';
}

INT8U Ten_to_16(INT8U Source_Data)
{
  INT8U Destination_Data;
  if(Source_Data == 10)
    Destination_Data = 'A';
  if(Source_Data == 11)
    Destination_Data = 'B';
  if(Source_Data == 12)
    Destination_Data = 'C';
  if(Source_Data == 13)
    Destination_Data = 'D';
  if(Source_Data == 14)
    Destination_Data = 'E';
  if(Source_Data == 15)
    Destination_Data = 'F';
  if((Source_Data>=0) &&(Source_Data<=9))
    Destination_Data = Source_Data + 48;
  return Destination_Data;
}

void Short_menu(void)
{
  uint16 Short;
  int j ;
  INT8U TEMP[7];
  Short = NLME_GetShortAddr();
  TEMP[2] = Short / 16 / 16 / 16;
  TEMP[3] = Short / 16 / 16 % 16;
  TEMP[4] = Short / 16 % 16;
  TEMP[5] = Short % 16;
  for(j=2;j<6;j++)
  {
    Short_Add[j] = Ten_to_16(TEMP[j]);
  }
  Short_Add[0] = '0';
  Short_Add[1] = 'x';
  Short_Add[6] = '\0';
}

//*********************************************************************************
//函数名:void SelectTxAddr(INT8U key)
//功能:查看ROUTER的个数
//输入：按键值
//输出：无
//*********************************************************************************
int Short_Addr_number;
void SelectTxAddr(INT8U key)
{
	INT8U arr[20];
	INT8U i, sel;
	
	ClearScreenLcd();
	if((JoinNode.RouterCount == 0) && (JoinNode.RfdCount == 0))
	{
		//Rectangle(0,2,127,5);
		strcpy((char*)arr, "NO JOIN NODE");
		Print(3,3,arr,1);
	}
	else
	{
		for(i=0; i<JoinNode.RouterCount; i++)
		{
			MenuItem[i][0] = i+'1';
			MenuItem[i][1] = '.';
			MenuItem[i][2] = 'R';
			MenuItem[i][3] = 'o';
			MenuItem[i][4] = 'u';
			memcpy(&MenuItem[i][5], &JoinNode.RouterAddr[i][0], 8);
                        MenuItem[i][13] = ' ';
			MenuItem[i][14] = ' ';
			MenuItem[i][15] = ' ';
			MenuItem[i][16] = '\0';
		}
		for(i=JoinNode.RouterCount; i<(JoinNode.RouterCount+JoinNode.RfdCount); i++)
		{
			MenuItem[i][0] = i+'1';
			MenuItem[i][1] = '.';
			MenuItem[i][2] = 'R';
			MenuItem[i][3] = 'f';
			MenuItem[i][4] = 'd';	
			memcpy(&MenuItem[i][5], &JoinNode.RfdAddr[i-JoinNode.RouterCount][0], 8);
			MenuItem[i][13] = ' ';
			MenuItem[i][14] = ' ';
			MenuItem[i][15] = ' ';
			MenuItem[i][16] = '\0';
		}
			
		sel = DrawMenu(MenuItem , JoinNode.RouterCount+JoinNode.RfdCount,key);
                if((sel == 0xff))
		{
			NowItem = 0;
			FirstItem = 0;
                        Menu_option = Zigbee_Menu_2;
                        Menu_all(0);
		}
		else if(sel < JoinNode.RouterCount)
		{
			SrcSaddr = JoinNode.RouterAddr[sel][8];
			SrcSaddr <<= 8;
			SrcSaddr += JoinNode.RouterAddr[sel][9];
		}
		else if(sel < (JoinNode.RouterCount+JoinNode.RfdCount))
		{
			SrcSaddr = JoinNode.RfdAddr[sel-JoinNode.RouterCount][8];
			SrcSaddr <<= 8;
			SrcSaddr += JoinNode.RfdAddr[sel-JoinNode.RouterCount][9];
   		}
        }
        if(key == HAL_KEY_CANCEL)
        {
          NowItem = 0;
          FirstItem = 0;
          Menu_option = Zigbee_Menu_2;
          Menu_all(0);
        }
}
//*********************************************************************************
//函数名:void SingleTxTest(uint8 key)
//功能:单次发送函数
//输入：无
//输出：无
//*********************************************************************************
void SingleTxTest(uint8 key)
{
  RfTx.TXDATA.HeadCom[0] = 'S';         //单次
  RfTx.TXDATA.HeadCom[1] = 'I';
  RfTx.TXDATA.HeadCom[2] = 'N';

  strcpy((char*)RfTx.TXDATA.DataBuf, "3w.c51rf.com");
  ClearScreenLcd();
  Print(3,0,"Send Data:",10);
  Print(5,0,RfTx.TXDATA.DataBuf,12);
  if(Send_Flag == Send_Flag_Single)
  {
    SendData(RfTx.TxBuf, SrcSaddr, 33);             //修改缓冲！！
    for(int i=0;i<5;i++)
    halWait(200);
    NowItem = 0;
    FirstItem = 0;
    Menu_option = Single_Send_Menu_4;
    Menu_all(0);
  }
  else if(Send_Flag == Send_Flag_Continuous)
  {
    Send_Flag_Consecution = Short_Send_Continuous;
    if(key == HAL_KEY_CANCEL)
    {
      Send_Flag_Consecution = 0;
      NowItem = 0;
      FirstItem = 0;
      Menu_option = Single_Send_Menu_4;
      Menu_all(0);
    }
  }

}

//*********************************************************************************
//函数名:void PingpongTxTest(uint8 key)
//功能:Pingpong发送函数
//输入：无
//输出：无
//*********************************************************************************
int flag = 0;
extern int Send_Data;
void PingpongTxTest(uint8 key)
{
  uint8 S_Addr[7];
  ClearScreenLcd();
  Print(4,0,"S_Data:",10);
  Print(6,0,"R_Data:",10);

  if(Send_Flag == Send_Flag_Single)
  {
    RfTx.TXDATA.HeadCom[0] = 'S';         //单次
    RfTx.TXDATA.HeadCom[1] = 'P';         //乒乓方式
    RfTx.TXDATA.HeadCom[2] = 'N';
    strcpy((char*)RfTx.TXDATA.DataBuf, "c51rf.com");
    Print(4,56,RfTx.TXDATA.DataBuf,9);
    if(flag == 0)
    SendData(RfTx.TxBuf, SrcSaddr, 33);          //修改缓冲！！
    flag++;
    if(key != 0)
    {
      flag = 0;
      Send_Flag_Consecution = 0;
      NowItem = 0;
      FirstItem = 0;
      Menu_option = Single_Send_Menu_4;
      Menu_all(0);
    }
  }
  else if(Send_Flag == Send_Flag_Continuous)
  {
    RfTx.TXDATA.HeadCom[0] = 'C';         //连续
    RfTx.TXDATA.HeadCom[1] = 'P';         //乒乓方式
    RfTx.TXDATA.HeadCom[2] = 'N';
    Send_Flag_Consecution = Pingpong_Send_Continuous;
    Send_Data = 0;
    sprintf(RfTx.TXDATA.DataBuf,(char *)"%d",Send_Data);
    SendData(RfTx.TxBuf, SrcSaddr, 33);                       //修改缓冲！！
    S_Addr[0] = '0';
    S_Addr[1] = 'x';
    S_Addr[6] = '\0';
    S_Addr[2] = SrcSaddr /16/16/16;
    S_Addr[3] = SrcSaddr /16/16%16;
    S_Addr[4] = SrcSaddr /16%16;
    S_Addr[5] = SrcSaddr %16;
    for(int j=2;j<6;j++)
    {
      S_Addr[j] = Ten_to_16(S_Addr[j]);
    }
    Print(2,0,"Short add:",1);
    Print(2,80,S_Addr,1);
    Print(4,56,RfTx.TXDATA.DataBuf,1);
    if(key == HAL_KEY_CANCEL)
    {
      Send_Flag_Consecution = 0;
      NowItem = 0;
      FirstItem = 0;
      Menu_option = Single_Send_Menu_4;
      Menu_all(0);
    }
  }

}
#ifndef ZG_Coord
//********************************************************************************
//函数名：void Sensor_Menu(INT8U key)
//功能：传感器测试菜单
//输入：KEY
//输出：null
//********************************************************************************
void Sensor_Menu(INT8U key)
{
  INT8U sel;
  strcpy((char*)MenuItem[0] ,"1:Power Check   ");
  strcpy((char*)MenuItem[1] ,"2:Temp Check    ");
  //strcpy((char*)MenuItem[2] ,"3:GuangM  Check ");
  //strcpy((char*)MenuItem[3] ,"3:Pot  Check    ");
  sel = DrawMenu(MenuItem ,2,key);
  switch(sel)
  {
    case 0:
      Menu_option = Sensor_ReadBattery_Menu_3;
      Menu_all(0);
    break;
    case 1:
      Menu_option = Sensor_Temp_Menu_3;
      Menu_all(0);
    break;
    case 2:
      Menu_option = Sensor_ReadBattery_Menu_3;
      NowItem = 0;
      Menu_all(0);
    break;
    case 3:
      Menu_option = Sensor_ReadBattery_Menu_3;
      NowItem = 0;
      Menu_all(0);
    break;
    case 0xff:
      Menu_option = Main_Menu_1;
      FirstItem = 0;
      NowItem = 0;
      Menu_all(0);
    break;
  }
}
#endif
//********************************************************************************
//函数名：void Uart_Menu(INT8U key)
//功能：串口测试菜单
//输入：KEY
//输出：null
//********************************************************************************
void Uart_Menu(INT8U key)
{
  INT8U sel;
  strcpy((char*)MenuItem[0] ,"1:Tx Test       ");
  strcpy((char*)MenuItem[1] ,"2:Rx Test       ");
  strcpy((char*)MenuItem[2] ,"3:Pingpong Test ");
  sel = DrawMenu(MenuItem ,3,key);
  switch(sel)
  {
    case 0:
      Menu_option = Uart_TX_Menu_3;
      Menu_all(0);
    break;
    case 1:
      Menu_option = Uart_RX_Menu_3;
      Menu_all(0);
    break;
    case 2:
      Menu_option = Uart_PP_Menu_3;
      NowItem = 0;
      Menu_all(0);
    break;
    case 0xff:
      Menu_option = Main_Menu_1;
      FirstItem = 0;
      NowItem = 0;
      Menu_all(0);
    break;
  }
}
//********************************************************************************
//函数名：void Uart_Send(INT8U key)
//功能：串口发送测试
//输入：KEY
//输出：null
//********************************************************************************
void Uart_Send(INT8U key)
{
  uint8 Baud_rate[6];
  ClearScreenLcd();
  Uart_Baud_rate(Baud_rate_38400);
  Print(2,8,"Uart send test",1);
  Print(4,0,"Baud rate:",1);
  sprintf(Baud_rate,(char *)"%d00",Baud_rate_38400);
  Print(4,80,Baud_rate,1);
  Print(6,0,"Data:hello word!",1);
  UartTX_Send_String("ChengDu WXL Communication Technology C0.,LTD\n",45);
  UartTX_Send_String("Uart send test\n",15);
  UartTX_Send_String("Baud rate:38400\n",16);
  Menu_option = Uart_TX_Menu_3;
  if(key == HAL_KEY_CANCEL)
  {
    UartTX_Send_String("Exit\n",5);
    Menu_option = Uart_Menu_2;
    FirstItem = 0;
    NowItem = 0;
    Menu_all(0);
  }
  else if(key == HAL_KEY_ENTER)
  {
    UartTX_Send_String("hello word!\n",12);
  }
}
//********************************************************************************
//函数名：void Uart_RX(INT8U key);
//功能：串口接收测试
//输入：KEY
//输出：null
//********************************************************************************
void Uart_RX(INT8U key)
{

  uint8 Baud_rate[6];
  ClearScreenLcd();
  Uart_Baud_rate(Baud_rate_38400);
  Print(2,16,"Uart Rx test",1);
  Print(4,0,"Baud rate:",1);
  sprintf(Baud_rate,(char *)"%d00",Baud_rate_38400);
  Print(4,80,Baud_rate,1);
  Print(6,0,"Data:",1);
  UartTX_Send_String("ChengDu WXL Communication Technology C0.,LTD\n",45);
  UartTX_Send_String("Uart RX test\n",13);
  UartTX_Send_String("Baud rate:38400\n",16);
  Uart_Rx_Flag = Open;
  Menu_option = Uart_RX_Menu_3;
  if(key == HAL_KEY_CANCEL)
  {
    Uart_Rx_Flag = Close;
    Menu_option = Uart_Menu_2;
    FirstItem = 0;
    NowItem = 0;
    Menu_all(0);
  }
}
//********************************************************************************
//函数名：void Uart_Pingpong(INT8U key);
//功能：串口收发测试
//输入：KEY
//输出：null
//********************************************************************************
void Uart_Pingpong(INT8U key)
{
  uint8 Baud_rate[6];
  ClearScreenLcd();
  Uart_Baud_rate(Baud_rate_38400);
  Print(2,16,"Uart PP test",1);
  Print(4,0,"Baud rate:",1);
  sprintf(Baud_rate,(char *)"%d00",Baud_rate_38400);
  Print(4,80,Baud_rate,1);
  Print(6,0,"Data:",1);
  UartTX_Send_String("ChengDu WXL Communication Technology C0.,LTD\n",45);
  UartTX_Send_String("Uart Pingpong test\n",19);
  UartTX_Send_String("Baud rate:38400\n",16);
  Uart_PP_Flag = Open;
  Menu_option = Uart_PP_Menu_3;
  if(key == HAL_KEY_CANCEL)
  {
    Uart_PP_Flag = Close;
    Menu_option = Uart_Menu_2;
    FirstItem = 0;
    NowItem = 0;
    Menu_all(0);
  }
}


#ifndef ZG_Coord
extern int Sensor_Flag;
//****************************************************************************
//函数名：uint16 ReadBattery_menu(INT8U key)
//功  能：读电源电压
//输  入：key
//输  出：
//****************************************************************************
void ReadBattery_menu(INT8U key)
{
  ClearScreenLcd();
  Print(2,0,"Power value",1);
  Sensor_Flag = Sensor_ReadBattery;
  if(key == HAL_KEY_CANCEL)
  {
    Sensor_Flag = 0;
    Menu_option = Sensor_Menu_2;
    FirstItem = 0;
    NowItem = 0;
    Menu_all(0);
  }

}

//****************************************************************************
//函数名：uint16 Temp_menu(INT8U key)
//功  能：读片内温度菜单
//输  入：key
//输  出：
//****************************************************************************
void Temp_menu(INT8U key)
{
  ClearScreenLcd();
  Print(2,0,"Temp value",1);
  Sensor_Flag = Sensor_Temp;
  if(key == HAL_KEY_CANCEL)
  {
    Sensor_Flag = 0;
    Menu_option = Sensor_Menu_2;
    FirstItem = 0;
    NowItem = 0;
    Menu_all(0);
  }

}
//****************************************************************************
//函数名：uint16 Read_Temp(void)
//功  能：读电源电压
//输  入：key
//输  出：
//****************************************************************************
uint16 Read_Temp(void)
{
  uint16 value;
  ADCIF = 0;//清除ADC标志
  ADCCON3 = (HAL_ADC_REF_125V | HAL_ADC_DEC_512 | HAL_ADC_CHN_TEMP);        //采用内部基准，12位ADC，温度通道
  while ( !ADCIF );//等到转化完成
  value = ADCL;
  value |= ((uint16) ADCH) << 8;      //取ADC值
  /*
   * 从0到0x8000表示0到1.25V
   * 0℃ = 0.763 V = 20002
   * 温度系数 = 0.0024 V/C = 62.9 /C
   * 上面的参数都是典型值需要校准请参见数据手册
   */
#define VOLTAGE_AT_TEMP_ZERO      20002   // 0.763 V
#define TEMP_COEFFICIENT          63.9    // 0.0024 V/C

  // 最低温度限制
  if ( value < VOLTAGE_AT_TEMP_ZERO )
    value = VOLTAGE_AT_TEMP_ZERO;

  value = value - VOLTAGE_AT_TEMP_ZERO;

  //最高温度限制
  if ( value > TEMP_COEFFICIENT * 99 )
    value = TEMP_COEFFICIENT * 99;

  return ( (uint8)(value/TEMP_COEFFICIENT) );

}



//****************************************************************************
//函数名：uint16 ReadBattery(void)
//功  能：读电源电压
//输  入：key
//输  出：
//****************************************************************************
uint16 ReadBattery(void)
{
   uint16 value;

  ADCIF = 0;    //清除ADC中断标志

 // ADCCON3 = (HAL_ADC_REF_125V | HAL_ADC_DEC_128 | HAL_ADC_CHN_VDD3);          //选择内部基准，用10位ADC，检测电源电压
  ADCCON3 = (HAL_ADC_REF_125V | HAL_ADC_DEC_128 | HAL_ADC_CHN_VDD3);          //选择内部基准，用10位ADC，检测1号通道电压

  /*等待转换完成*/
  while ( !ADCIF );

  //返回结果
  value = ADCL;
  value |= ((uint16) ADCH) << 8;

  /*
   * 这个值是测量电源电压值
   * 1.25V 0表示0V，32767表示1.25V
   * voltage = (value*3*1.25)/32767 volts
   * 增加10倍电源电压，以表示有小树位增加精度
   */

  value = value >> 6;   // divide first by 2^6
  value = value * 37.5;
  value = value >> 9;   // ...and later by 2^9...to prevent overflow during multiplication

  return value;
}


#endif