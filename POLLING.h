#pragma once

#ifdef __MTU
#include "MTU05.h"
#endif

#ifdef __I7K
#include "POLL_I7K.h"
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
    //S(0x01);
    switchLed();
    //int C0=0,C1=0;
    SYS::sleep(toNextSecond);
#ifdef __I7K
    int I7K_Qry, I7K_Ans;
    I7K_StatRead(I7K_Qry,I7K_Ans);
#endif
#ifdef __MTU
    int MTU_Qry, MTU_Ans;
    MTU_StatRead(MTU_Qry,MTU_Ans);
#endif
    TIME Time;
#ifdef __MTU
    SYS::getNetTime(Time);
    SYS::sleep(1);
#endif
#ifdef q__I7K
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
    //S(0x04);
#define __UNDEFINED
#ifdef __UNDEFINED
    U16 year,month,day,hour,min,sec,msec;
    //SYS::getNetTime(Time);
    SYS::DecodeDate(Time, year, month, day);
    SYS::DecodeTime(Time, hour, min, sec, msec);

#if defined( __MTU )
  #ifdef __I7K
    #define fmt "\n\r%d%02d%02d_%02d%02d%02d.%03d  MTU:%03d/%03d I7K:%03d/%03d SYS:%03d %02d"
    #define arg MTU_Qry, MTU_Ans, I7K_Qry, I7K_Ans
  #else
    #define fmt "\n\r%d%02d%02d_%02d%02d%02d.%03d  MTU:%03d/%03d SYS:%03d %02d"
    #define arg MTU_Qry, MTU_Ans
  #endif
#elif defined( __I7K )
    #define fmt "\n\r%d%02d%02d_%02d%02d%02d.%03d  I7K:%03d/%03d SYS:%03d %02d"
    #define arg I7K_Qry, I7K_Ans
#else
    #define fmt "\n\r%d%02d%02d_%02d%02d%02d.%03d  SYS:%03d %02d"
    #define arg
#endif
    ConPrintf(fmt, year, month, day, hour, min, sec, msec, arg, SYS::GetCPUIdleMs(), SYS::GetTimerISRMs() );
#undef fmt
#undef arg

#ifdef __UsePerfCounters
    for(int i=1; i<=4; i++){
      STRUCT_COMPERF CP;
      GetCom(i).GetPerf(&CP);
      if(i<=2)
        ConPrintf(" %d:%02d R%04d T%04d",i,CP.ISRMs,CP.RBytes,CP.TBytes);
      else ConPrintf(" %d:%02d",i,CP.ISRMs);
//      ConPrintf(" %d:%02d R%04d/%02d T%04d/%02d",i,CP.ISRMs,CP.RBytes,CP.RCalls,CP.TBytes,CP.TCalls);
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
