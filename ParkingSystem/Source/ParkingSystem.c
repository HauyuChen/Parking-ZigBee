/*********************************************************************
** ��Ŀ���ƣ�����ͣ�������������ϵͳ
** ���ܸ��������������ݲɼ�����λ״̬��������Ϣ;
             GPRSģ�����ݴ��䣺TCP���䡢���ŷ���;
             LED���ƣ���λ״̬LED�ơ�LED�����
** ���˵������������SampleApp�޸�
** �޸����ڣ�2015��5��8��
*********************************************************************/
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "ParkingSystem.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

#include "string.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

/* ��λ��Ϣ */
#include "ParkingStatus.h"
#include "LightControl.h"

/* GPRSģ�����ATָ��� */
uchar CGDCONT[27] = {"AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n"};  //����APN
uchar ETCPIP[11] = {"AT%ETCPIP\r\n"};     //����TCP/IP����
uchar IPOPEN[38] = {"AT%IPOPEN=\"TCP\",\"119.146.68.41\",5000\r\n"};  //����TCP����
uchar IOMODE[13] = {"AT%IOMODE=0\r\n"};   //�����ı�ģʽ
uchar IPSEND[11] = {"AT%IPSEND=\""};      //�������ݣ��ں��油���ݰ�
uchar IPCLOSE[12] = {"AT%IPCLOSE\r\n"};   //�ر�����
uint8 PHONE[12] = " ";  //�ֻ�����
uchar MSG[80] = "0008A72260A876848F665DF279BB5F008F664F4DFF0C598267095F025E388BF781F475350058"; //����ģ��

/* ��ر������� */
uint8 LX;                 //����ֵLX
uint8 MAU;                //�������ֶ�����ģʽ
uchar LStatus[1];         //������״̬
uint8 PA,PB;              //A����λ������B����λ����
uint PnumA[2]={0};        //LEDָʾ����A����λ����
uint PnumB[2]={0};        //LEDָʾ����B����λ����
uint8 GPRS_FLAG = 0;      //GPRSģ���ʼ��ATָ��˳���־
uint8 SENDMSG_FLAG = 0;   //GPRSģ�鷢�Ͷ���ATָ��˳���־

/* LED��IO�ڶ��� */
#define LED P1_4
#define BLOCKA P0_0
#define BLOCKB P0_7

/*********************************************************************
** ��������
*/
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_SEND_DATA_CAR_CLUSTERID,    //���³�λ״̬ID
  SAMPLEAPP_SEND_DATA_LIGHT_CLUSTERID,  //����������ϢID
  SAMPLEAPP_LED_CONTROL_CLUSTERID       //�����ƿ���ID
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

endPointDesc_t SampleApp_epDesc;

uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;
afAddrType_t Point_To_Point_DstAddr;

aps_Group_t SampleApp_Group;

uint8 SampleAppFlashCounter = 0;


/*********************************************************************
** ����ԭ��
*/
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt ); //��Ϣ�����¼�
static void UART_CallBack (uint8 port,uint8 event);         //���ڻص�����
void SampleApp_SendPointToPointMessage_Car(void);           //��λ���ݷ���
void SampleApp_SendPointToPointMessage_Light(void);         //�������ݷ���
void LightYN(uint n);                                       //�����ƿ���

/*********************************************************************
** ��������
*/
void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;

  /* �������ýṹ�� */
  halUARTCfg_t uartConfig;
  /* ���ڳ�ʼ�� */
  uartConfig.configured = TRUE;
  uartConfig.baudRate   = HAL_UART_BR_9600;  //����������Ϊ9600
  uartConfig.flowControl  = FALSE;
  uartConfig.flowControlThreshold = 64;
  uartConfig.rx.maxBufSize        = 128;
  uartConfig.tx.maxBufSize        = 128;
  uartConfig.idleTimeout          = 6;
  uartConfig.intEnable            = TRUE;
  uartConfig.callBackFunc = UART_CallBack;    //���ڻص�����UART_CallBack
  HalUARTOpen(0,&uartConfig);                 //��������
  
  /* LED�Ƴ�ʼ�� */
  P1DIR |= 0x16;
  LED = 0;
  BLOCKA = 0;
  BLOCKB = 0;

#if defined ( BUILD_ALL_DEVICES )
  if ( readCoordinatorJumper() )
    zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
  else
    zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
#endif // BUILD_ALL_DEVICES

#if defined ( HOLD_AUTO_START )
  ZDOInitDevice(0);
#endif

  /* �鲥ͨѶ���� */
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;

  /* ��Ե�ͨѶ���� */
  Point_To_Point_DstAddr.addrMode = (afAddrMode_t)Addr16Bit; //�㲥
  Point_To_Point_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  Point_To_Point_DstAddr.addr.shortAddr = 0x0000;           //����Э����
  
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  afRegister( &SampleApp_epDesc );

  RegisterForKeys( SampleApp_TaskID );

  SampleApp_Group.ID = 0x0001;
  osal_memcpy( SampleApp_Group.name, "Group 1", 7  );
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
}

uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  /* ϵͳ�¼� */
  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        /* ���յ�������Ϣ */
        case AF_INCOMING_MSG_CMD: 
          SampleApp_MessageMSGCB( MSGpkt );   //��Ϣ�����¼�
          break;
        
        /* �豸״̬�ı� */
        case ZDO_STATE_CHANGE:
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          
          if ( SampleApp_NwkState == DEV_ZB_COORD ) //�����ZigBeeЭ����
          {
            osal_start_timerEx( SampleApp_TaskID,
                              PARKINGSYSTEM_GPRS_EVT,
                              PARKINGSYSTEM_GPRS_TIMEOUT );
          }
          else if ( (SampleApp_NwkState == DEV_ROUTER)
              || (SampleApp_NwkState == DEV_END_DEVICE) ) //�����ZigBee·��������ZigBee�ն�
          {
            //ִ�г�λ��⹦�ܣ����ݽڵ㹦��ѡ���Ƿ���룩
            osal_start_timerEx( SampleApp_TaskID,
                              PARKINGSYSTEM_CAR_EVT,
                              PARKINGSYSTEM_GPRS_TIMEOUT );
            
            //ִ�����ȼ�⹦�ܣ����ݽڵ㹦��ѡ���Ƿ���룩
            osal_start_timerEx( SampleApp_TaskID,
                              PARKINGSYSTEM_LIGHT_EVT,
                              PARKINGSYSTEM_LIGHT_TIMEOUT );
          }
          else
          {
            // Device is no longer in the network
          }
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );

      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
  /* ��λ����¼� */
  if ( events & PARKINGSYSTEM_CAR_EVT )
  {  
    Parking_Status();  //��λ״̬���
    
    SampleApp_SendPointToPointMessage_Car();  //���ͳ�λ״̬
    
    //ÿ5�붨ʱ��⳵λ����������
    osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_CAR_EVT,
        (PARKINGSYSTEM_CAR_TIMEOUT + (osal_rand() & 0x00FF)) );
    // return unprocessed events
    return (events ^ PARKINGSYSTEM_CAR_EVT);
  }
  
  /* ��������¼� */
  if ( events & PARKINGSYSTEM_LIGHT_EVT )
  {  
    Brightness();  //������Ϣ���

    SampleApp_SendPointToPointMessage_Light();  //����������Ϣ
    
    //ÿ10�붨ʱ���������Ϣ����������
    osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_LIGHT_EVT,
        (PARKINGSYSTEM_LIGHT_TIMEOUT + (osal_rand() & 0x00FF)) );
    // return unprocessed events
    return (events ^ PARKINGSYSTEM_LIGHT_EVT);
  }
  
  /* GPRSģ���¼�����ʼ��TCP���� */
  if ( events & PARKINGSYSTEM_GPRS_EVT )
  {  
    /* ÿ5�뷢��һ��ATָ�� */
    switch(GPRS_FLAG)
    {
      case 0:         //ATָ��ر�TCP���ӣ���AT%IPCLOSE
        HalUARTWrite(0,IPCLOSE,sizeof(IPCLOSE));
        GPRS_FLAG++;  //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
      case 1:         //ATָ�����APN����AT+CGDCONT=1,��IP��,��CMNET��
        HalUARTWrite(0,CGDCONT,sizeof(CGDCONT));
        GPRS_FLAG++;  //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
      case 2:         //ATָ�����tcpip ���ܣ���AT%ETCPIP
        HalUARTWrite(0,ETCPIP,sizeof(ETCPIP));
        GPRS_FLAG++;  //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
      case 3:         //ATָ���һ��tcp���ӣ���AT%IPOPEN=��TCP��,��119.146.68.41��,5000
        HalUARTWrite(0,IPOPEN,sizeof(IPOPEN));
        GPRS_FLAG++;  //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
      case 4:         //ATָ�IOMODE���ã���AT%IOMODE=0
        HalUARTWrite(0,IOMODE,sizeof(IOMODE));
        GPRS_FLAG++;  //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
      case 5:         //ATָ��������ݵ�tcp �Զˣ���AT%IPSEND=��Zigbee��
        HalUARTWrite(0,IPSEND,sizeof(IPSEND));
        HalUARTWrite(0,"Zigbee",6);   //��־
        HalUARTWrite(0,"\"\r\n",3);
        GPRS_FLAG++;  //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
      case 6:         //ATָ��������ݵ�tcp �Զˣ���AT%IPSEND=��5226��
        HalUARTWrite(0,IPSEND,sizeof(IPSEND));
        HalUARTWrite(0,"5226",4);   
        HalUARTWrite(0,"\"\r\n",3);
        GPRS_FLAG++;  //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
      case 7:         //ATָ�ѡ��ģʽ����AT+CMGF=0
        HalUARTWrite(0,"AT+CMGF=0\r\n",11);
        GPRS_FLAG=0;  //ATָ��ִ�������0
      break;
      default:
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_GPRS_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
    }
    // return unprocessed events
    return (events ^ PARKINGSYSTEM_GPRS_EVT);
  }

  /* GPRSģ���¼������Ͷ��� */
  if ( events & PARKINGSYSTEM_SEMDMSG_EVT )
  { 
    /* ÿ5�뷢��һ��ATָ�� */
    switch(SENDMSG_FLAG)
    {
      
      case 0:             //ATָ����ö��ų��ȣ���AT+CMGS=49
        HalUARTWrite(0,"AT+CMGS=49\r\n",12);
        SENDMSG_FLAG++;   //ATָ��ִ����ż�1���´�ִ����һ��ATָ��
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_SEMDMSG_EVT,
        (200 + (osal_rand() & 0x00FF)) );
      break;
      case 1:             //ATָ��������ݣ���0011000D9168+PHONE+MSG
        HalUARTWrite(0,"0011000D9168",12);
        HalUARTWrite(0,PHONE,12);     //�û��ֻ�����
        strcat(MSG,"\x1A");           //��������
        HalUARTWrite(0,MSG,sizeof(MSG));
        SENDMSG_FLAG=0;   //ATָ��ִ�������0
      break;
      default:
        osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_SEMDMSG_EVT,
        (PARKINGSYSTEM_GPRS_TIMEOUT + (osal_rand() & 0x00FF)) );
      break;
    } 
    // return unprocessed events
    return (events ^ PARKINGSYSTEM_SEMDMSG_EVT);
  }

  // Discard unknown events
  return 0;
}

void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  switch ( pkt->clusterId )
  {
    /* ��λ��Ϣ���� */
    case SAMPLEAPP_SEND_DATA_CAR_CLUSTERID: 
      HalUARTWrite(0,IPSEND,sizeof(IPSEND));  //ATָ���TCP��������������
      if(pkt->cmd.Data[0]=='A')               //�����A��
      {
        //��Ϊ����Ŀ��ÿ������ֻѡȡ������λ���ô���������������λ��0��1ģ��
        HalUARTWrite(0,"A1",2);               //��һ���ַ�ָʾ���ĸ�����������A��
        HalUARTWrite(0,&pkt->cmd.Data[1],2);  //A002��A003��λ
        HalUARTWrite(0,"010000000000000",15); //������λ�ü�����ģ��
        HalUARTWrite(0,&pkt->cmd.Data[3],1);  //A019��λ
        HalUARTWrite(0,"000010000001",12);    //������λ�ü�����ģ��
      }
      else if(pkt->cmd.Data[0]=='B')          //�����B��
      {
        //��Ϊ����Ŀ��ÿ������ֻѡȡ������λ���ô���������������λ��0��1ģ��
        HalUARTWrite(0,"B10000100000000110",18);   //��һ���ַ�ָʾ���ĸ�����������B��
        HalUARTWrite(0,&pkt->cmd.Data[1],1);       //B018��λ
        HalUARTWrite(0,"0000",4);                  //������λ�ü�����ģ��
        HalUARTWrite(0,&pkt->cmd.Data[2],1);       //B023��λ
        HalUARTWrite(0,"1000000001",10);           //������λ�ü�����ģ��
        HalUARTWrite(0,&pkt->cmd.Data[3],1);       //B034��λ
        HalUARTWrite(0,"00001000",8);              //������λ�ü�����ģ��
      }
      HalUARTWrite(0,"\"\r\n",3);
     break;
   
   /* ������Ϣ���� */
   case SAMPLEAPP_SEND_DATA_LIGHT_CLUSTERID:
    //LStatus[0] = LED+48;
    //�����������������Ϣ��������״̬����LIGHT00079,Status:1
    HalUARTWrite(0,IPSEND,sizeof(IPSEND));  //ATָ���TCP��������������
    HalUARTWrite(0,"LIGHT",5);   
    HalUARTWrite(0,&pkt->cmd.Data[0],5);    //���ȣ�LXֵ
    HalUARTWrite(0,"LX,",3);
    HalUARTWrite(0,"Status:",7);
    HalUARTWrite(0,LStatus,1);              //������״̬
    HalUARTWrite(0,"\"\r\n",3);
    
    /* ���ֶ�ģʽ�£�����LXֵ�Զ����������� */
    if(MAU==0)
    {
      LX = (pkt->cmd.Data[2]-48)*100+(pkt->cmd.Data[3]-48)*10+(pkt->cmd.Data[4]-48);
      if(LX>=10)  //���ȴ���10LXʱ��������
      {
        LightYN(0);
        LStatus[0] = '0';
      }
      else
      {
        LightYN(1);
        LStatus[0] = '1';
      }
    } 
   break;
   
   /* �����ƿ��� */
   case SAMPLEAPP_LED_CONTROL_CLUSTERID:
    if(pkt->cmd.Data[0]==1)
      LED = 1;  //����
    else
      LED = 0;  //�ص�
   break;
  }
}

/******************************************************
** �������ն˷�������
** ��ID��SAMPLEAPP_SEND_DATA_CAR_CLUSTERID
** ���ܣ�����λ״̬��Ϣ���͸�ZigBeeЭ����
*/
void SampleApp_SendPointToPointMessage_Car( void )
{
  uint8 PD[4];
  /* ��λ���� */
  //PD[0]=0+65;   //A����λ
  PD[0]=0+66;     //B����λ
  /* ������λ״̬(ֻģ������������λ) */
  PD[1]=S001+48;
  PD[2]=S002+48;
  PD[3]=S003+48;
  
  if ( AF_DataRequest( &Point_To_Point_DstAddr,
                       &SampleApp_epDesc,
                       SAMPLEAPP_SEND_DATA_CAR_CLUSTERID,
                       4,
                       PD,  //��λ����
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

/******************************************************
** �������ն˷�������
** ��ID��SAMPLEAPP_SEND_DATA_LIGHT_CLUSTERID
** ���ܣ���������Ϣ���͸�ZigBeeЭ����
*/
void SampleApp_SendPointToPointMessage_Light( void )
{
  if ( AF_DataRequest( &Point_To_Point_DstAddr,
                       &SampleApp_epDesc,
                       SAMPLEAPP_SEND_DATA_LIGHT_CLUSTERID,
                       5,
                       Light, //��������
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

/*************************************
** �������ƹ�Զ�̿���
** ���ܣ�����ͣ�����������ƵĿ���
*/
void LightYN(uint n)
{
  uint8 L[1];
  L[0] = n;

  if ( AF_DataRequest( &SampleApp_Flash_DstAddr,
                       &SampleApp_epDesc,
                       SAMPLEAPP_LED_CONTROL_CLUSTERID,
                       1,
                       L,   //���ص�ָ��
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

/******************************************************
** ���������ڻص�����
** ���ܣ��Դ��ڷ��͹��������ݽ��д���
*/
static void UART_CallBack (uint8 port,uint8 event)
{
  uint8 usartbuf[30] = " ";
  HalUARTRead(0,usartbuf,30);   //�Ӵ����ж�ȡ���ݣ������usartbuf��
  
  if(osal_memcmp(&usartbuf[11],"MANUAL",6))   //������յ�MANUAL
  {
    MAU = 1;      //�ֶ�ģʽ
  }
  else if(osal_memcmp(&usartbuf[12],"SELFCONTROL",11))   //������յ�SELFCONTROL
  {
    MAU = 0;      //�Զ�ģʽ
  }
  else if(osal_memcmp(&usartbuf[11],"LIGHTON",7))        //������յ�LIGHTON
  {
    LightYN(1);   //����
    LStatus[0] = '1';
  }
  else if(osal_memcmp(&usartbuf[11],"LIGHTOFF",8))       //������յ�LIGHTOFF
  {
    LightYN(0);   //�ص�
    LStatus[0] = '0';
  }
  else if(osal_memcmp(&usartbuf[11],"PA",2))   //������յ�PA
  {
    PA = (usartbuf[13]-48)*10 + (usartbuf[14]-48);  //����A����λ����
    if(PA != 0)
    {
      BLOCKA = 1;         //A����λ���е���
      PnumA[0] = usartbuf[13]-48;
      PnumA[1] = usartbuf[14]-48;
      num_led(PnumA);     //���³�λ����LED�����
    }
  }
  else if(osal_memcmp(&usartbuf[11],"PB",2))   //������յ�PB
  {
    PB = (usartbuf[13]-48)*10 + (usartbuf[14]-48);  //����B����λ����
    if(PB != 0)
    {
      BLOCKA = 1;         //B����λ���е���
      PnumB[0] = usartbuf[13]-48;
      PnumB[1] = usartbuf[14]-48;
      //num_led(PnumB);   //���³�λ����LED�����
    }
  }
  else if(osal_memcmp(&usartbuf[12],"SENDMSG",7))   //������յ�SENDMSG+�ֻ���  
  {
    /* �����յ��ֻ��Ž��д�����ԭʼ����15767977442��ת����5167977744F2 */
    PHONE[0]=usartbuf[20];
    PHONE[1]=usartbuf[19];
    PHONE[2]=usartbuf[22];
    PHONE[3]=usartbuf[21];
    PHONE[4]=usartbuf[24];
    PHONE[5]=usartbuf[23];
    PHONE[6]=usartbuf[26];
    PHONE[7]=usartbuf[25];
    PHONE[8]=usartbuf[28];
    PHONE[9]=usartbuf[27];
    PHONE[10]= 'F';
    PHONE[11]=usartbuf[29];
    
    /* ���Ͷ��Ÿ�Ŀ���ֻ��� */
    osal_start_timerEx( SampleApp_TaskID, PARKINGSYSTEM_SEMDMSG_EVT,
        (0 + (osal_rand() & 0x00FF)) );              
  }
}
