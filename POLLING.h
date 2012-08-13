#pragma once

#ifdef __MTU
#include "MTU05.h"
#endif

#ifdef __I7K
#include "POLL_I7K.h"
#endif

#if !defined(__NO_STAT)
class THREAD_STAT : public THREAD {
public:
void execute()
{
  dbg("\n\rSTART Stat");
  U16 prevMin = 60;
  while(true)
  {
    S(0x01);
    switchLed();
    //int C0=0,C1=0;
    SYS::sleep(toNextSecond);
    S(0x02);
    TIME Time;
    SYS::getNetTime(Time);
#ifdef __I7K
    int I7K_Qry, I7K_Ans;
    I7K_StatRead(I7K_Qry,I7K_Ans);
#endif
#ifdef __MTU
    int MTU_Qry, MTU_Ans;
    MTU_StatRead(MTU_Qry,MTU_Ans);
#endif
    SYS::switchThread();
    if(Terminated)
        break;
    //SYS::sleep(1);
    S(0x03);
#define __UNDEFINED
#ifdef __UNDEFINED
    U16 year,month,day,hour,min,sec,msec;
    //SYS::getNetTime(Time);
    SYS::DecodeDate(Time, year, month, day);
    SYS::DecodeTime(Time, hour, min, sec, msec);

    char buf[100];
    int pos = 0;
#if defined( __MTU )
    if(prevMin!=min)
    {
        EVENT_DI Event;
        Event.Time = Time;
        S(0x04);
        for(int i=ctx_MTU.ADCsList->Count()-1; i>=0; i--)
        {
            PU_ADC_MTU* mtu = (PU_ADC_MTU*)((*ctx_MTU.ADCsList)[i]);
            int nErrs = mtu->nCrcErrors;
            if(nErrs==0) continue;
            prevMin=min;
            mtu->nCrcErrors = 0;
            ConPrintf("\n\rMTU[%d] #%d, %d CRC error(s)", i, mtu->BusNum, nErrs);
            Event.Channel=200+i;
            Event.ChangedTo=nErrs;
            Events.EventDigitalInput(Event);
            Event.Time++;
        }
    }
    S(0x05);
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
    pos+=sprintf(buf, fmt, year, month, day, hour, min, sec, msec, arg, SYS::GetCPUIdleMs(), SYS::GetTimerISRMs() );
#undef fmt
#undef arg

#ifdef __UsePerfCounters
    for(int i=1; i<=4; i++)
    {
        STRUCT_COMPERF CP;
        GetCom(i).GetPerf(&CP);
        if(i<=2)
            pos+=sprintf(buf+pos," %d:%02d R%04d T%04d",i,CP.ISRMs,CP.RBytes,CP.TBytes);
        else
            pos+=sprintf(buf+pos," %d:%02d",i,CP.ISRMs);
    }
#endif
    ConPrint(buf);
#else //UNDEFINED
    ConPrint(".");
#endif
    HLI_linkCheck();
#ifdef __DO2_PERIOD
    {   // DO2 control
        static TIMEOUTOBJ toutDO2;
        static enum {do2Init, do2On, do2Off} do2State = do2Init;
        switch(do2State)
        {
            case do2Off:
                if(!toutDO2.IsSignaled())
                    break;
            case do2Init:
                DIO::SetDO2(0);
                do2State = do2On;
                toutDO2.start(toTypeSec | (__DO2_PERIOD));
                ConPrint("\n\rDO2=0\n\r");
                break;
            case do2On:
                if(!toutDO2.IsSignaled())
                    break;
                DIO::SetDO2(1);
                do2State = do2Off;
                toutDO2.start(toTypeSec | 1);
                ConPrint("\n\rDO2=1\n\r");
        }
    }
#endif
  }
  S(0x00);
  dbg("\n\rSTOP Stat");
}


};

#endif // __NO_STAT
