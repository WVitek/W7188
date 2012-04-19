#if defined(_HLI_HDR)



#define MaxPacketDataSize (MaxARQDataSize-2)

#if defined(__RS485) && !defined(__HALFDUPLEX)
#define __HALFDUPLEX
#endif

#if defined(__HALFDUPLEX) || defined(__FULLDUPLEX)
#define __WIRE
#endif

#define setNeedConnection(x)

//#ifndef __GPS_TIME
    #include "TIMC_Svc.h"
    TIMECLIENT_SVC TimeSvc;
//#endif

#if defined(__7188XA) || defined(__7188XB)
void HLI_linkCheck();
#else
#define HLI_linkCheck()
#endif





#else // !defined(_HLI_HDR)





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

#define TOUT_LINK_AFTER_DATA_RX (toTypeSec | 600)
#define TOUT_LINK_AFTER_PWR_OFF (toTypeSec | 600)
#define TOUT_LINK_POWER_OFF_TIME (toTypeSec | 5)
static TIMEOUTOBJ toutLink;

#ifdef __7188X

#if !defined(POWERON_DO1)
#define POWERON_DO1 true
#endif

void HLI_linkCheck()
{
  static enum {modemPowerInit, modemPowerOn, modemPowerOff}
    modemPower = modemPowerInit;
  switch(modemPower)
  {
      case modemPowerOff:
        if(!toutLink.IsSignaled())
            return;
      case modemPowerInit:
        DIO::SetDO1(POWERON_DO1); // modem power up
        modemPower = modemPowerOn;
        toutLink.start(TOUT_LINK_AFTER_PWR_OFF);
        ConPrint("\n\rHLI: DO1=1 (mdm pwr ON)\n\r");
        break;
      case modemPowerOn:
        if(toutLink.IsSignaled())
        {
            DIO::SetDO1(!POWERON_DO1); // modem power down
            modemPower = modemPowerOff;
            toutLink.start(TOUT_LINK_POWER_OFF_TIME);
            ConPrint("\n\rHLI: DO1=0 (mdm pwr OFF)\n\r");
        }
        break;
  }
}
#endif

BOOL HLI_receive(void const * Buf, int BufSize)
{
  if( CRC16_is_OK(Buf,BufSize,CRC16_PPP) )
  {
    PACKET *p = (PACKET*)Buf;
    if(p->Hdr.To != MyAddr) return TRUE;
    U8 SvcID = p->Hdr.ServiceID;
    SERVICE **svc = Service;
    while(*svc && (**svc).ID!=SvcID)
        svc++;
    if(*svc)
        (**svc).receiveData( p->Hdr.From, p->Data, BufSize - HLI_SYSDATASIZE );
    toutLink.start(TOUT_LINK_AFTER_DATA_RX);
    return TRUE;
  }
  else {
    dbg2("\n\rHLI: CRC error:",BufSize);
    dump(Buf,BufSize);
    return FALSE; //dododo
  }
}

#define HLI_Addr 1

int HLI_totransmit(void* Buf, int BufSize)
{
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
        toutTx.start(toTypeSec | 40); // need some tx every 40 sec
        CRC16_write( Buf,  DataSize, CRC16_PPP );
        DataSize += CRC_SIZE;
    }
  return DataSize;
}

#if defined(__GPRS_SIM300)
  #include "hli_sim3.h"
#elif defined(__GPRS_GR47)
  #include "hli_gr47.h"
#elif defined(__GPRS_2238)
  #include "hli_2238.h"
#elif defined(__GPRS_TC65)
  #include "hli_tc65.h"
#elif defined(__WIRE)
  #include "hli_wire.h"
#else
  #error !!! Modem type not specified
#endif

#endif
