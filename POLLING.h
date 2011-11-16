#pragma once

int __CntPollQuery=0, __CntPollAnswer=0;
inline void PollStatAdd(int Qry, int Ans) {
  SYS::cli();
  __CntPollQuery +=Qry;
  __CntPollAnswer+=Ans;
  SYS::sti();
}
inline void PollStatRead(int &Qry, int &Ans) {
  SYS::cli();
  Qry=__CntPollQuery;  __CntPollQuery =0;
  Ans=__CntPollAnswer; __CntPollAnswer=0;
  SYS::sti();
}

#ifdef __MTU
#include "MTU05.h"
#endif

#ifdef __I7K
#include "Module.h"
#include "WHrdware.hpp"

class THREAD_POLLING : public THREAD {
  U16 pollPort;
  U32 baudRate;
  MODULES *modules;
  PU_LIST *pollList;
public:
  THREAD_POLLING(U16 pollPort=0, U32 baudRate=38400,
    MODULES &m=Modules, PU_LIST &pl=PollList)
  {
    if(pollPort==0)
#ifdef __PollPort
      this->pollPort=__PollPort;
#else
      this->pollPort=2;
#endif
    else this->pollPort=pollPort;
    this->baudRate=baudRate;
    modules=&m;
    pollList=&pl;
  }
  void execute();
};

void THREAD_POLLING::execute(){
  COMPORT& RS485=GetCom(pollPort);
  RS485.install(baudRate);
  dbg("\n\rPOLLING started");
  //dbg3("\n\rPOLLING started COM%d @ %ld",pollPort,baudRate);
  U8 Query[16];
  U8 Resp[256];
  POLL_UNIT *pu,*pu_;
  int i0=0, i;
  Realtime=TRUE;
  // Configure modules
  while(TRUE){
    BOOL Quit=TRUE;
    for(i=modules->Count()-1; i>=0; i--){
      if((*modules)[i]->GetConfigCmd(Query)){
        RS485.sendCmdTo7000(Query,TRUE);
        RS485.receiveLine(Resp,100,TRUE);
        Quit=FALSE;
      }
    }
    if(Quit)break;
  }
  pu_=(*pollList)[i0];
  // send first command
  if(!pu_->GetPollCmd(Query)) if(--i0<0) i0=pollList->Count()-1;;
  RS485.sendCmdTo7000(Query,TRUE);
  // polling loop
  while(!Terminated){
    // prepare next command
    pu=(*pollList)[i0];
    if(!pu->GetPollCmd(Query)) if(--i0<0) i0=pollList->Count()-1;
    // wait response
    Resp[0]=0;
    RS485.RxEvent().waitFor(50);
    // send next commnad
    RS485.sendCmdTo7000(Query,TRUE);
    // process response string
    int R=RS485.receiveLine(Resp,0,TRUE);
    U8* Tmp = (R==0) ? Resp : NULL;
    int Ans = (pu_->response(Tmp)) ? 1: 0;
    PollStatAdd(1,Ans);
    S(0x04);
    pu_=pu;
  }
  S(0x00);
  dbg("\n\rPOLLING stopped");
  //dbg2("\n\rPOLLING stopped COM%d",pollPort);
}
#endif

#if !defined(__NO_TMR)
//class THREAD_TMR
class THREAD_TMR : public THREAD {
public:
  void execute();
};

#define TIME_RESYNC_INTERVAL 600

void THREAD_TMR::execute(){
  dbg("\n\rTIMER started");
  while(!Terminated){
    S(0x01);
    switchLed();
    TIME Time;
    //int C0=0,C1=0;
    SYS::sleep(toNextSecond);
#ifdef __MTU
    SYS::getNetTime(Time);
#else
    for(int i=ADC_Freq-1; i>=0; i--)
    {
      SYS::getNetTime(Time);
      int j;
      int n=PollListAll.Count();
      if(n>0){
        for(j=n-1; j>=0; j--)
          PollListAll[j]->latchPollData();
        for(j=n-1; j>=0; j--)
          PollListAll[j]->doSample(Time);
        if(i>0) for(int j=(10/ADC_Freq)-1; j>=0; j--) SYS::sleep(toNext100ms);
      }
      else SYS::sleep(100);
    }
#endif
    S(0x04);
#define __UNDEFINED
#ifdef __UNDEFINED
    int Qry,Ans;
    PollStatRead(Qry,Ans);
    U16 year,month,day,hour,min,sec,msec;
    //SYS::getNetTime(Time);
    SYS::DecodeDate(Time, year, month, day);
    SYS::DecodeTime(Time, hour, min, sec, msec);
    ConPrintf( "\n\r%d%02d%02d_%02d%02d%02d.%03d  Q:%03d A:%03d SYS:%03d %02d",
        year, month, day, hour, min, sec, msec,
        Qry, Ans, SYS::GetCPUIdleMs(), SYS::GetTimerISRMs() );
#ifdef __UsePerfCounters
    for(int i=1; i<=4; i++){
      STRUCT_COMPERF CP;
      GetCom(i).GetPerf(&CP);
      ConPrintf(" %d:%02d R%04d/%02d T%04d/%02d",
        i,CP.ISRMs,CP.RBytes,CP.RCalls,CP.TBytes,CP.TCalls
      );
    }
#endif
#else //UNDEFINED
    ConPrint(".");
#endif
    HLI_linkCheck();
  }
  S(0x00);
  dbg("\n\rTIMER stopped");
}

#endif // __NO_TMR
