#if defined(_HLI_HDR)



#define MaxPacketDataSize (MaxARQDataSize-2)

#if defined(__RS485) && !defined(__HALFDUPLEX)
#define __HALFDUPLEX
#endif

#if defined(__HALFDUPLEX) || defined(__FULLDUPLEX)
#define __WIRE
#endif

#define setNeedConnection(x)

#ifndef __SendTimeEvents
#include "TIMC_Svc.h"
TIMECLIENT_SVC TimeSvc;
#endif

#if defined(__7188XA) || defined(__7188XB)
void HLI_linkCheck();
#else
#define HLI_linkCheck()
#endif



#else // !defined(_HLI_HDR)



#ifdef __SendTimeEvents
#include "TIMC_Svc.h"
TIMECLIENT_SVC TimeSvc;
#endif

#include "ADC_Svc.h"
#include "DI_Svc.h"
DI_SVC DI_Svc;
#include "PROG_Svc.h"
PROG_SVC PROG_Svc;
//#include "DUMP_Svc.h"
//DUMP_SVC DUMP_Svc;

SERVICE* Service[]=
{
//#ifndef __GPS_TIME
    &TimeSvc,
//#endif
    &DI_Svc,
#ifdef __MTU
    new ADC_SVC(ctx_MTU.ADCsList, 2),
    #ifdef __I7K
    new ADC_SVC(ctx_I7K.ADCsList, 6),
    #endif
#else
    new ADC_SVC(ctx_I7K.ADCsList, 2),
#endif
    &PROG_Svc,
//    &DUMP_Svc,
    NULL
};

struct PACKETHEADER
{
  U8 To,From;
  U8 ServiceID;
};

struct PACKET
{
  PACKETHEADER Hdr;
  U8 Data[1];
};

#define CRC_SIZE 2
#define HLI_SYSDATASIZE (sizeof(PACKETHEADER)+CRC_SIZE)

#ifdef __RESET_IF_NO_LINK

#define TOUT_LINK_AFTER_DATA_RX (toTypeSec | 240)
#define TOUT_LINK_AFTER_PWR_OFF (toTypeSec | 900)

#else

#if defined(__ARQ)

#define TOUT_LINK_AFTER_DATA_RX (toTypeSec | 1200)
#define TOUT_LINK_AFTER_PWR_OFF (toTypeSec | 1200)
#define TOUT_LINK_POWER_OFF_TIME (toTypeSec | 5)

#else

#define TOUT_LINK_AFTER_DATA_RX (toTypeSec | 600)
#define TOUT_LINK_AFTER_PWR_OFF (toTypeSec | 600)
#define TOUT_LINK_POWER_OFF_TIME (toTypeSec | 5)

#endif

#endif

static TIMEOUTOBJ toutLink;

#ifdef __7188X

#if !defined(POWERON_DO1)
#define POWERON_DO1 1
#endif

void HLI_linkCheck()
{
  //ConPrint("\n\r   Debug Hli link check");
  static enum {modemPowerInit, modemPowerOn, modemPowerOff}
    modemPower = modemPowerInit;
  switch(modemPower)
  {
      case modemPowerOff:
        if(!toutLink.IsSignaled())
            return;
      case modemPowerInit:
        //SYS::sleep(1500); // for relay // not working!
        DIO::SetDO1(POWERON_DO1); // modem power up
        modemPower = modemPowerOn;
        toutLink.start(TOUT_LINK_AFTER_PWR_OFF);
        ConPrintf("\n\rHLI: DO1=%d (mdm pwr ON)\n\r",POWERON_DO1);
        break;
      case modemPowerOn:
        if(toutLink.IsSignaled())
        {
#ifdef __RESET_IF_NO_LINK
            ConPrint("\n\rRESET by NO LINK timeout");
            SYS::sleep(100);
            SYS::reset(TRUE);
#else
            DIO::SetDO1(!POWERON_DO1); // modem power down
            modemPower = modemPowerOff;
            toutLink.start(TOUT_LINK_POWER_OFF_TIME);
            ConPrintf("\n\rHLI: DO1=%d (mdm pwr OFF)\n\r",!POWERON_DO1);
#endif
        }
        break;
  }
}

#endif // __7188X


BOOL HLI_receive(void const * Buf, int BufSize)
{
  //ConPrint("\n\r   Debug Hli recieve");
  if( CRC16_is_OK(Buf,BufSize,CRC16_PPP) )
  {
#ifdef __HLI_RX_SWITCH_LED
    switchLed();
#endif
    PACKET *p = (PACKET*)Buf;
    if(p->Hdr.To != MyAddr) return TRUE;
    U8 SvcID = p->Hdr.ServiceID;
    SERVICE **svc = Service;
    while(*svc && (**svc).ID!=SvcID)
        svc++;
    if(*svc){
        //ConPrintf("\n\r\n\r HLI_receive:\n\r p->Hdr.To = %d  p->Hdr.From = %d p->Hdr.ServiceID = %d DataSize = %d\n\r",
        //           p->Hdr.To, p->Hdr.From, p->Hdr.ServiceID, BufSize - HLI_SYSDATASIZE);

        (**svc).receiveData( p->Hdr.From, p->Data, BufSize - HLI_SYSDATASIZE );
    }
    toutLink.start(TOUT_LINK_AFTER_DATA_RX);
    return TRUE;
  }
  else {
    dbg2("\n\rHLI: CRC error:",BufSize);
    //dump(Buf,BufSize);
    return FALSE; //dododo
  }
}

#define HLI_Addr 1

int HLI_totransmit(void* Buf, int BufSize)
{
    //ConPrint("\n\r   Debug totransmit");
    static TIMEOUTOBJ toutTx;
    int DataSize = 0;

    SERVICE **svc = Service;
    while(*svc && !(**svc).HaveDataToTransmit(HLI_Addr))
        svc++;
    if( *svc || toutTx.IsSignaled() || BufSize==0)
    {
        PACKET *p = (PACKET*)Buf;
        p->Hdr.To = HLI_Addr;
        p->Hdr.From = MyAddr;
        if( *svc && BufSize>0 )
        {
            p->Hdr.ServiceID = (**svc).ID;
            DataSize = sizeof(PACKETHEADER) + (**svc).getDataToTransmit( HLI_Addr, p->Data, BufSize - HLI_SYSDATASIZE );
        }
        else
        { // ping
            p->Hdr.To = 0;
            p->Hdr.From = MyAddr;
            p->Hdr.ServiceID = 0;
            DataSize = sizeof(PACKETHEADER);
        }

        //ConPrintf("\n\r\n\r HLI_totransmit:\n\r p->Hdr.To = %d  p->Hdr.From = %d p->Hdr.ServiceID = %d DataSize = %d\n\r",
        //           p->Hdr.To, p->Hdr.From, p->Hdr.ServiceID, DataSize);

        toutTx.start(toTypeSec | 40); // need some tx every 40 sec
        CRC16_write( Buf,  DataSize, CRC16_PPP );
        DataSize += CRC_SIZE;
    }
  return DataSize;
}

#ifdef __UseAvgTOfs
void HLI_chechForEmergencyResync()
{
    //ConPrint("\n\r   Debug HLI_chechForEmergencyResync");

    TimeSvc.chechForEmergencyResync();

    //SERVICE **svc = Service;
    //while(*svc && (**svc).ID!=1)
    //    svc++;
    //if(*svc){
    //    //ConPrintf("\n\r\n\r HLI_receive:\n\r p->Hdr.To = %d  p->Hdr.From = %d p->Hdr.ServiceID = %d DataSize = %d\n\r",
    //    //           p->Hdr.To, p->Hdr.From, p->Hdr.ServiceID, BufSize - HLI_SYSDATASIZE);
    //    (**svc).receiveData( p->Hdr.From, p->Data, BufSize - HLI_SYSDATASIZE );
    //}
}
#endif

#if defined(__GPRS_SIM300)
  #include "hli_sim3.h"
#elif defined(__GPRS_GR47)
  #include "hli_gr47.h"
#elif defined(__GPRS_2238)
  #include "hli_2238.h"
#elif defined(__GPRS_TC65)
  #include "hli_tc65.h"
#elif defined(__GPRS_MC52)
  #include "hli_mc52.h"
#elif defined(__WIRE)
  #include "hli_wire.h"
#else
  #error !!! Modem type not specified
#endif

#endif
