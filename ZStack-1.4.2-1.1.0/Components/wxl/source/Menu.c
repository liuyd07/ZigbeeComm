#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

#include "NLMEDE.h"
/* HAL */
#include "lcd128_64.h"
#include "hal_led.h"
#include "hal_key.h"
#include "wxl_uart.h"
#include "Menu.h"

uint8 menu_TaskID;
endPointDesc_t Menu_epDesc =
{
  SAMPLEAPP_ENDPOINT,
  &menu_TaskID,
  (SimpleDescriptionFormat_t *)NULL,
  (afNetworkLatencyReq_t)0
};

void menu_Init(uint8 task_id)
{
  menu_TaskID = task_id;
  afRegister( &Menu_epDesc );
}
extern void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt );
uint16 menu_ProcessEvent( uint8 task_id, uint16 events )
{

}