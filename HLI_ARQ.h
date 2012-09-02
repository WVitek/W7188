#if defined(_HLI_HDR)

#define MaxPacketDataSize (MaxARQDataSize-2)
#define __HLIControlCD

#if   defined(__GSM_GR47)
  #define _ComSpeedHLI 19200
  char* szModemInitCmd="AT &F &D2 &S0 E0 S0=1; +IPR=19200; &W\r";
#elif   defined(__GSM_SIM300)
  #define _ComSpeedHLI 19200
  #if !defined(__ModemInitCmd)
  char* szModemInitCmd="AT &C1 &D2 E0 S0=1 &W\r";
  #endif
#elif defined(__GSM_TC65)
  #define _ComSpeedHLI 19200
  char* szModemInitCmd="AT&D2;&S0;E0;S0=1;+IPR=19200\r";
#elif defined(__CDMA2000_DTG450)
  #define _ComSpeedHLI 19200
  char* szModemInitCmd="AT &F &D2 &C1 E0 S0=1 +IPR=19200\r";
#elif defined(__LEASEDLINE)
  #undef __HLIControlCD
  #define _ComSpeedHLI 38400
  char* szModemInitCmd="";
#else
  #error !!! Modem type not specified
#endif

char* szDialCmd="ATS0=1;D+79625271206\r"; // UPO BeeLine
//char* szDialCmd="AT S0=1 D+79048169201\r"; // VPO Utel
//char* szDialCmd="AT S0=1 D+79128921286\r"; // VPO MTS
//char* szDialCmd="AT S0=1 D+79136013079\r"; // Isilkul
//char* szDialCmd="AT S0=1 D+79173784236\r"; // Chudaev home
//char* szDialCmd="AT S0=1 D+79173784236\r"; // UPO
//char* szDialCmd="AT S0=1 D+79177922184\r"; // Simka UPO
//char* szDialCmd="AT S0=1 D+79174706642\r"; // Home A
//char* szDialCmd="AT S0=1 D+79174836137\r"; // Home B
//char* szDialCmd="AT S0=1 D89277558041\r"; // Georgievka
//char* szDialCmd="AT S0=1 D4406378\r"; // UPO Sotel-Video
#define IntervalNextDial 90
//#define IntervalNeedConnect
#define IntervalRetryDial 45
S16 TimerDial=0;
TIME LastConnectTime=0;
BOOL NeedConnection=TRUE;
CRITICALSECTION _csConn;

void setNeedConnection(BOOL Need){
  _csConn.enter();
  TIME Now;
  if(Need && !NeedConnection){
    SYS::getSysTime(Now);
    if(LastConnectTime!=0 && LastConnectTime+90000l < Now)
      TimerDial = (S16)((90000l - (S32)(Now-LastConnectTime))/1000);
  }
  else if (!Need) SYS::getSysTime(LastConnectTime);
  NeedConnection=Need;
  _csConn.leave();
}

#include "TIMC_ARQ.h"
TIMECLIENT_SVC TimeSvc;

#define HLI_linkCheck()

#else // !defined(_HLI_HDR)



#include "ADC_ARQ.h"
ADC_SVC ADC_Svc(ctx_I7K.ADCsList);
#include "DI_ARQ.h"
DI_SVC DI_Svc;
#include "PROG_Svc.h"
PROG_SVC PROG_Svc;
#include "DUMP_Svc.h"
DUMP_SVC DUMP_Svc;

#define ServicesCount 4
SERVICE* Service[ServicesCount]={&DI_Svc,&ADC_Svc,&PROG_Svc,&DUMP_Svc};


struct PACKETHEADER {
  U8 Addr;
  U8 ServiceID;
};

//class THREAD_HLI
class THREAD_HLI : public THREAD {
  U16 comNum;
public:
  THREAD_HLI(U16 comNum):THREAD(4096+1024){
    this->comNum = comNum;
  }
  void execute();
};

#if defined(__7188XB)
#define HLI_CarrierDetected() HLI.IsClearToSend()
#define HLI_setDtr(x) HLI.setRts(x)
#else
#define HLI_CarrierDetected() HLI.CarrierDetected()
#define HLI_setDtr(x) HLI.setDtr(x)
#endif

void THREAD_HLI::execute(){
  COMPORT& HLI = GetCom(comNum);
#if defined(__GSM_GR47)
  HLI.install(9600);
#elif defined(__GSM_TC65)
  HLI.install(115200);
#elif defined(__CDMA2000_DTG450)
  HLI.install(115200);
#endif
  HLI.print(szModemInitCmd);
  SYS::sleep(500);
  HLI.install(_ComSpeedHLI);
  PRT_COMPORT prtcom(&HLI);
  PRT_ARQ prtarq(&prtcom);
  prtarq.TimeClient=&TimeSvc;
  prtarq.Acknowledged=FALSE;
  dbg3("\n\rSTART HLI_ARQ (#%d, COM%d)",MyAddr,comNum);
  U8 iNextSvc=0;
  U8 RxCnt=0, TxCnt=0;
#ifdef __HLIControlCD
//  int NoDataCnt=0;
  int TimerInit=0;
#endif
  while(1){
    U16 R;
    while(HLI_CarrierDetected() && !Terminated)
    {
      R=prtarq.ProcessIO();
#ifdef __HLIControlCD
/*
      if(!(R & IO_RX) && prtarq.LinkTimeout()){
        ConPrint("\n\rHLI link timeout\n\r");
        //HLI.reset();
        //HLI.setDtrLow();
        //SYS::sleep(300);
        //HLI.setDtrHigh();
        SYS::sleep(100);
        HLI.print(szModemInitCmd);
      } //*/
#endif
      if(R!=0) break;
      HLI.RxEvent().reset();
      HLI.RxEvent().waitFor(2);
    }
#ifdef __HLIControlCD
    if( ! HLI_CarrierDetected() )
    {
        prtarq.Acknowledged=FALSE;
      #if defined(__USE_CALLBACK_MODE)
        int iPos=0;
        char* sRING="RING";
      #endif
        do
        {
          while(HLI.BytesInRxB())
          {
              char c=HLI.readChar();
              ConWriteChar(c);
        #if defined(__USE_CALLBACK_MODE)
              if(c!=sRING[iPos++])
                  iPos=0;
              else if(sRING[iPos]==0)
              {// incoming "RING" detected, perform delayed callback
                  HLI.print("ath\r"); // reset incoming call
                  iPos=0;
                  setNeedConnection(TRUE);
                  TimerDial=3;
              }
        #endif
          }
          char *szCmd=NULL;
          if(NeedConnection)
          {
            _csConn.enter();
            TimerDial--;
            if(TimerDial<=0){
              szCmd=szDialCmd;
              TimerDial=IntervalRetryDial;
            }
            _csConn.leave();
          }
          else
          {
            TimerInit--;
            if(TimerInit<=0){
              ConPrint("! RESET !");
              HLI_setDtr(false);
              SYS::sleep(300);
              HLI_setDtr(true);
              //SYS::sleep(100);
              //HLI.print("atz\r");
              SYS::sleep(1000);
              szCmd=szModemInitCmd;
              TimerInit=120; // Modem reinit period
            }
          }
          if(szCmd){
            HLI.reset();
            ConPrintf("\n\r%s\n\r",szCmd);
            HLI.print("\n\n");
            SYS::sleep(100);
            HLI.print(szCmd);
          }
          ConPrintf(" CD??? %ds",TimerDial);
          SYS::sleep(1000);
        }while(!(HLI_CarrierDetected() || Terminated));
        TimerDial=0;
        TimerInit=0;
    }
#endif
/*
    SYS::sleep(5);
*/
    if(Terminated) break;

#ifndef __SHOWADCDATA
    SYS::dbgLed( (U32(RxCnt)<<12) | TxCnt );
#endif

    struct PACKET {
      PACKETHEADER Hdr;
      U8 Data[MaxARQDataSize-2];
    } PackI,PackO;
    U16 Size;
    int i;
    if(R & IO_RX){
      RxCnt++;
#ifdef __HLIControlCD
//      NoDataCnt=0;
#endif
#ifdef __HLI_RX_SWITCH_LED
      switchLed();
#endif
      Size=prtarq.Rx(&PackI);
      for(i=0; i<ServicesCount && Service[i]->ID!=PackI.Hdr.ServiceID; i++);
      if(i<ServicesCount)
        Service[i]->receiveData(1,PackI.Data,Size-sizeof(PACKETHEADER));
    }
    if((R & IO_TX) && prtarq.Acknowledged){
      PackO.Hdr.Addr=MyAddr;
      i=iNextSvc;
      BOOL HaveData=FALSE;
      do{
        if(Service[i]->HaveDataToTransmit(1)){
          HaveData=TRUE;
          PackO.Hdr.ServiceID=Service[i]->ID;
          Size=Service[i]->getDataToTransmit(1,PackO.Data,MaxPacketDataSize);
        }
        if(++i>=ServicesCount) i=0;
      }while(i!=iNextSvc && !HaveData);
      iNextSvc=i;
      if(!HaveData){
        HLI.RxEvent().reset();
        HLI.RxEvent().waitFor(2);
      }
      else {
        TxCnt++;
        prtarq.Tx(&PackO,sizeof(PACKETHEADER)+Size);
      }
    }
  }
  HLI.uninstall();
  S(0x00);
  dbg("\n\rSTOP HLI_ARQ");
}


#endif
