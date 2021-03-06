#pragma once

//#define I7K_Period 1000
//#define I7K_Log2BufSize 13
//
//#define MTU_Period 40
//#define MTU_Log2BufSize 14

#ifndef __STRING_H
  #include <string.h>
#endif
#include "WTLIST.hpp"


//#define dtChecksumBit 0x40
//U16 BaudCodeBaudRate[0x09]={
//      0, // 0x00
//      0, // 0x01
//      0, // 0x02
//   1200, // 0x03
//   2400, // 0x04
//   4800, // 0x05
//   9600, // 0x06
//  19200, // 0x07
//  38400  // 0x08
//};

class MODULE;
class POLL_UNIT;
class PU_ADC;

typedef TLIST<MODULE*> MODULES;
typedef TLIST<POLL_UNIT*> PU_LIST;
typedef TLIST<PU_ADC*> ADC_LIST;

struct
{
    MODULE* Module;
    MODULES* Modules;
    PU_LIST* PollList;
    ADC_LIST* ADCsList;
    int PeriodADC;
    int Log2BufSize;
} _ctx;

class CONTEXT_CREATOR
{
public:
    CONTEXT_CREATOR(int PeriodADC, int Log2BufSize)
    {
        _ctx.PeriodADC = PeriodADC;
        _ctx.Log2BufSize = Log2BufSize;
        _ctx.Modules = new MODULES();
        _ctx.PollList = new PU_LIST();
        _ctx.ADCsList = new ADC_LIST();
    }
};

class CONTEXT
{
public:
    MODULES* Modules;
    PU_LIST* PollList;
    ADC_LIST* ADCsList;
    int Period;

    CONTEXT()
    {
        Modules = _ctx.Modules;
        PollList = _ctx.PollList;
        ADCsList = _ctx.ADCsList;
        Period = _ctx.PeriodADC;
        //dbg3("\n\rnMods=%d; nPoll=%d",Modules->Count(),PollList->Count());
    }
};


class MODULE : public OBJECT {
protected:
  U8 Address;  //*
public:
  CRITICALSECTION cs;
  MODULE(int Addr=1)
  {
    Address=Addr;
    if(Addr!=0) _ctx.Modules->addLast(this);
    _ctx.Module = this;
  }
  U8 inline GetAddress()const{ return Address; }
  BOOL virtual GetConfigCmd(U8*/*Buf*/){ return FALSE; }
};

class I7017 : public MODULE {
  U8 ChMask; // used channels mask
  U8 State;
public:
  I7017(int Addr=1):MODULE(Addr) { }
  inline void useChannel(U8 Num){
    ChMask |= U8(1)<<Num;
  }
  BOOL GetConfigCmd(U8 *Buf){
    U8 A = Address;
    switch(State){
      case 0:
        strcpy((char*)Buf,"%AANN0D08C2\0"); // ADC mode "0D": 20 mA @ 125 Ohm (=2.5V), Comm. mode "08" = 38400 baud + checksum
        Buf[1]=hex_to_ascii[A >> 4];
        Buf[2]=hex_to_ascii[A & 0xF];
        Buf[3]=Buf[1];
        Buf[4]=Buf[2];
        break;
      case 1:
        strcpy((char*)Buf,"$AA5VV\0");
        Buf[1]=hex_to_ascii[A >> 4];
        Buf[2]=hex_to_ascii[A & 0xF];
        Buf[4]=hex_to_ascii[ChMask >> 4];;
        Buf[5]=hex_to_ascii[ChMask & 0xF];
        break;
      default:
        return FALSE;
    }
    State++;
    return TRUE;
  }
};

#ifndef __CYCL_BUF_INC
#include "CYCL_BUF.h"
#endif

class POLL_UNIT : public CYCL_BUF {
protected:
  MODULE *Module;
  POLL_UNIT():CYCL_BUF(0,0){}
public:
  CRITICALSECTION cs;
  POLL_UNIT(int ItSz,U8 Lg2Cp):CYCL_BUF(ItSz,Lg2Cp)
  {
      Module = _ctx.Module;
      if(Module->GetAddress()!=0)
        _ctx.PollList->addLast(this);
  }
  int inline SafeCount()
  { cs.enter(); int r=Count(); cs.leave(); return r; }
  BOOL virtual GetPollCmd(U8 *Buf)=0;
  BOOL virtual response(const U8 *Resp)=0;
  void virtual latchPollData(){}
  void virtual doSample(TIME Time)=0;
  int virtual readArchive(TIME &FromTime, U8 *Buf, U16 BufSize)=0;
};

#define flgError       0x80
#define flgEComm       (flgError | 0x40)
#define flgEADCRange   (flgError | 0x20)
#define flgEDataFormat (flgError | 0x10)
#define flgEResponse   (flgError | 0x08)
#define ASItemSize 2

PU_LIST plADC;

#ifdef __EVALUATION
static TIME __GetEndTime()
{
    TIME res;
    SYS::TryEncodeTime(2012,12,MyAddr-10,0,0,0,res);
    //SYS::TryEncodeTime(2013,3,MyAddr-89,0,0,0,res);
    return res;
}
static TIME evalEndTime = __GetEndTime();
static bool EvalExceeded = FALSE;
#endif

class PU_ADC : public POLL_UNIT
{
public:
  TIME FirstTime;
  int ADC_Period;

  PU_ADC():POLL_UNIT(2, _ctx.Log2BufSize)
  {
    ADC_Period = _ctx.PeriodADC;
    if(Items)
        _ctx.ADCsList->addLast(this);
    else ConPrint("\n\rPU_ADC:Items==NULL!!!");
  }

  inline TIME GetLastTime()
  { return FirstTime+smul(Count(),ADC_Period); }

  int readArchive(TIME &FromTime, U8 *Buf, U16 BufSize)
  {
    int r;
    TIME LastTime = GetLastTime();
    if ( LastTime <= FromTime )
    {
      FromTime = LastTime;
      r=0;
    }
    else
    {
      U16 Pos;
      if( FromTime < FirstTime )
      {
        Pos = 0;
        FromTime = FirstTime;
      }
      else
        Pos = U16( U32(FromTime - FirstTime) / ADC_Period);
      U16 Cnt = BufSize / ASItemSize;
      if( Pos + Cnt > Count() ) Cnt = Count()-Pos;
      if( Cnt )
    #ifndef __EVALUATION
        r=readn(Buf,Pos,Cnt)*ASItemSize;
    #else
      {
        if(EvalExceeded || FromTime > evalEndTime)
        {
          if(!EvalExceeded)
          {
            ConPrint("\n\rEvaluation period exceeded\n\r");
            EvalExceeded = TRUE;
          }
          r=readn(Buf,Pos,Cnt)*ASItemSize;
          int k=(r>ASItemSize)? r-ASItemSize : r;
          while(k>0)
          {
            k-=ASItemSize;
            Buf[k] = 0;
            Buf[k+1] = flgEResponse;
          }
        }
        else r=readn(Buf,Pos,Cnt)*ASItemSize;
      }
    #endif
      else
        r=0;
    }
    return r;
  }
}; // PU_ADC

#ifdef __I7K

// Analog Input Channel (from I7017)
class PU_ADC_7K : public PU_ADC
{
  S32 ChSum;
  U16 ChCnt;
  U8  ChFlg;
  U8  ChCount;
  U8  Channel;
public:
  S32 LatchedSum;
  U16 LatchedCnt;
  U8  LatchedFlg;
#ifdef ADCDOWNSAMPLE
  U8 FilterCnt;
#endif

public:
  PU_ADC_7K(int Ch):PU_ADC()
  {
    Channel=Ch;
    ((I7017*)_ctx.Module)->useChannel(Ch);
  }

  BOOL GetPollCmd(U8 *Buf)
  {
    U8 A = Module->GetAddress();
    Buf[0]='#';
    Buf[1]=hex_to_ascii[A >> 4];
    Buf[2]=hex_to_ascii[A & 0xF];
    Buf[3]=hex_to_ascii[Channel];
    Buf[4]=0;
    return FALSE;
  }

  BOOL response(const U8 *Resp){
    if(Resp){
      if(Resp[0]=='>'){
        if(Resp[5]==0){
          S16 Value=(S16)FromHexStr(&(Resp[1]),4);
          if(Value!=32767 && Value>=0){
            cs.enter();
            ChSum+=Value;
            ChCnt++;
            cs.leave();
          }
          else ChFlg|=flgEADCRange;
          return TRUE;
        }
        else ChFlg|=flgEDataFormat;
      }
      else ChFlg|=flgEResponse;
    }
    else ChFlg|=flgEComm;
    return FALSE;
  }

  void latchPollData(){
    cs.enter();
    LatchedSum=ChSum; ChSum=0;
    LatchedCnt=ChCnt; ChCnt=0;
    LatchedFlg=ChFlg; ChFlg=0;
    cs.leave();
  }
  void doSample(TIME Time){
  #ifdef ADCDOWNSAMPLE
    if(++FilterCnt < ADCDOWNSAMPLE) return;
  #endif
    U16 Sample;
    if (LatchedCnt>0){
      Sample = udiv(LatchedSum, LatchedCnt);
    #ifdef __SHOWADCDATA
      if(Channel==0){
        // p*100 = (Sample*k-b)>>16
        // k = P*V*6250*65536/(32767*R)
        // b = P*25*65536
        // V=2.5; R=125; k=P*252.024; b=P*1638400
      #if   __SHOWADCDATA == 100
        S16 Val = S16((smul(Sample,25000)-163840000L)>>16); // P=100
      #elif __SHOWADCDATA == 60
        S16 Val = S16((smul(Sample,15000)- 98304000L)>>16); // P= 60
      #elif __SHOWADCDATA == 40
        S16 Val = S16((smul(Sample,10000)- 65536000L)>>16); // P= 40
      #elif __SHOWADCDATA == 25
        S16 Val = S16((smul(Sample, 6250)- 40960000L)>>16); // P= 25
      #elif __SHOWADCDATA == 10
        S16 Val = S16((smul(Sample, 2500)- 16384000L)>>16); // P= 10
      #elif __SHOWADCDATA == 6
        S16 Val = S16((smul(Sample, 1500)- 9830400L)>>16); // P= 6
      #else
        S16 Val = S16(smul(Sample,500)>>16); // V=2.5
      #endif
        SYS::showDecimal(Val);
      }
    #endif
    }
    else {
      Sample=(flgError | LatchedFlg)<<8;
    #ifdef __SHOWADCDATA
      if(Channel==0){
        SYS::dbgLed(0xBAD00ul | LatchedFlg);
      }
    #endif
    }
    cs.enter();
//    TimeOk=TimeSvc.TimeOk();
  #ifdef ADCDOWNSAMPLE
    do{
  #endif
    if(IsFull()) get(NULL);
    put(&Sample);
  #ifdef ADCDOWNSAMPLE
    }while(--FilterCnt!=0);
  #endif
    FirstTime=Time-S32(Count()-1)*ADC_Period;
    cs.leave();
  }
}; // PU_ADC_7K
#endif // __I7K

#ifdef __MTU

#include "WFloat32.h"
class MTU_Coeffs
{
public:
    F32 C[9];

#ifdef __MTU_VAL_EMUL
    MTU_Coeffs()
    {
        C[0] = 0x095207C0ul;
        C[1] = 0xAA5FC5B7ul;
        C[2] = 0xC57DD0B0ul;
        C[3] = 0xE4A3D639ul;
        C[4] = 0xF18D0330ul;
        C[5] = 0xEAB69D29ul;
        C[6] = 0x7A3D4BB0ul;
        C[7] = 0x75811029ul;
        C[8] = 0x0606A5A0ul;
    }
#endif


    void setCoeff(int i, U32 srcFloat)
    {
        C[i] = srcFloat;
    }

    F32 calc(U16 p, U16 t)
    {
        F32 P = S32_to_F32(p);
        F32 T = S32_to_F32(t);
        F32 P2 = fmul(P,P);
        F32 T2 = fmul(T,T);
        // C[0] + C[1] * T + C[2] * T*T
        F32 r0 = fadd(C[0], fadd(fmul(C[2],T2),fmul(C[1],T)));
        // (C[3] + C[4] * T + C[5] * T*T) * P;
        F32 r1 = fmul( fadd( C[3], fadd( fmul(C[4],T), fmul(C[5],T2) ) ), P);
        // (C[6] + C[7] * T + C[8] * T*T) * P * P;
        F32 r2 = fmul( fadd( C[6], fadd( fmul(C[7],T), fmul(C[8],T2) ) ), P2);
        return fadd(fadd(r2,r1),r0);
    }

};

class PU_ADC_MTU : public PU_ADC
{
    U16 cnt;
    U16 buf[20];
public:
    U16 nCrcErrors;
    U8  BusNum;
    //static TLIST<PU_ADC_MTU*> all;
    MTU_Coeffs coeffs;

    PU_ADC_MTU(int busNum):PU_ADC()
    {
        BusNum=busNum;
    }

    BOOL GetPollCmd(U8 *Buf)
    {
        Buf[0]=0;
        return FALSE;
    }

    BOOL response(const U8 *Resp)
    {
        cnt = Resp[0];
        if(cnt>10 || cnt==0)
        { cnt = 0; return FALSE; }
        int n=cnt<<1;
        int size=1+(n<<1);
        if(CRC8_CCITT_get(Resp,size)!=Resp[size])
            nCrcErrors++;
        Resp++;
        for(int i=0; i<n; i++, Resp+=2)
            buf[i] = swap2bytes(*(U16*)Resp);
        return FALSE;
    }

    void latchPollData(){}

    //const static F32 fMaxPto32K = 0x44A3D5C3; // 32767/25
    const static F32 fMaxPto32K = 0x449D889E; // 32767/(25+1)
    const static S32 RawOffsetOfP0 = 32767 / (25+1);
    const static F32 fSampleValue = 0x41C40000; // 24.5

    static const U16 noDataErrCode = flgEComm << 8;

    void doSample(TIME Time)
    {
        S(0x41);
        U16 ps[10];
#ifdef __MTU_VAL_EMUL
        static U8 mix;
        cnt=7;
#endif
        for(int i=0; i<cnt; i++)
        {
#ifdef __MTU_VAL_EMUL
            U16 bits = mix++ & 0x0F;
            U16 p = 0x2D31 ^ bits;
            U16 t = 0xC2F2 ^ bits;
#else
            U16 p = buf[i<<1];
            U16 t = buf[(i<<1)+1];
#endif
            F32 fP = coeffs.calc(p,t);
            //F32 fP = fSampleValue;
            S32 raw = F32_to_S32(fmul(fP,fMaxPto32K)) + RawOffsetOfP0;
#ifdef __MTU_VAL_EMUL
            ps[i] = (U16)(RawOffsetOfP0 + mix);
#else
            if(raw<32767)
                ps[i] = (raw<0) ? 0 :(U16)raw;
            else ps[i] = flgEADCRange<<8;
#endif
        }
        S(0x42);
        TIME timeA = GetLastTime();
        TIME timeB = Time - smul(ADC_Period, cnt);
        cs.enter();
        for(int i=10; i>0 && timeA<timeB; i--)
        {
            if(IsFull()) get(NULL);
            put(&noDataErrCode);
            timeA += ADC_Period;
        }
        for(int i=0; i<cnt; i++)
        {
            if(IsFull()) get(NULL);
            put(&(ps[i]));
        }
        FirstTime=Time-smul(Count()-1,ADC_Period);
        cnt=0;
        cs.leave();
        S(0x43);
    }

}; // PU_ADC_MTU
#endif // __MTU

struct EVENT_DI {
  TIME Time; U8 Channel; U8 ChangedTo;
};

#define AlarmEventSize sizeof(EVENT_DI)
// Digital Input Channels 0-7 (from 7063)

#define Events (*PU_DI::Instance)

class PU_DI : public POLL_UNIT  {
protected:
  U16 ChMask,PrevStatus,Status,ChangedTo0,ChangedTo1;
  TIME FLastEventTime;
  BOOL TimeOk;
  CRITICALSECTION csPoll;
public:
  static PU_DI* Instance;

  PU_DI(U16 ChMask):POLL_UNIT(AlarmEventSize,10)
  {
    PrevStatus=Status=this->ChMask=ChMask;
    if(!Instance)
        Instance = this;
    FLastEventTime=0;
  }
  BOOL GetPollCmd(U8 *Buf){
    U8 A = Module->GetAddress();
    Buf[0]='@';
    Buf[1]=hex_to_ascii[A >> 4];
    Buf[2]=hex_to_ascii[A & 0xF];
    Buf[3]=0;
    return FALSE;
  }
  BOOL response(const U8 *Resp){
    if(Resp && Resp[0]=='>' && Resp[5]==0)
    {
      U16 NewStatus=ChMask & (U16)FromHexStr(&(Resp[1]),4);
      csPoll.enter();
      ChangedTo0|=Status & ~NewStatus;
      ChangedTo1|=~Status & NewStatus;
      Status=NewStatus;
      csPoll.leave();
      //dbg((const char*)Resp);
      return TRUE;
    }
    return FALSE;
  }

  void EventDigitalInput(const EVENT_DI &Event)
  {
    cs.enter();
    if(IsFull()) get(NULL);
    put(&Event);
    FLastEventTime=Event.Time;
    cs.leave();
    ConPrintf("\n\rEVENTS: Channel[%d]=%d",Event.Channel,Event.ChangedTo);
    setNeedConnection(TRUE);
  }

  int readArchive(TIME &FromTime, U8 *Buf, U16 BufSize)
  {
    cs.enter();
    int i=0;
    EVENT_DI *pEv;
    while( i<Count() && (pEv=(EVENT_DI*)at(i))->Time < FromTime ) i++;
    if(i==Count()) return 0;
    int size = 0;
    EVENT_DI *pDst = (EVENT_DI*)Buf;
    while( i<Count() && size+sizeof(EVENT_DI)<=BufSize )
    {
      *pDst++ = *(EVENT_DI*)at(i);
      i++; size+=sizeof(EVENT_DI);
    }
    if(size) FromTime = (--pDst)->Time;
    cs.leave();
    return size;
  }

  BOOL HaveNewEvents(const TIME& Time)
  {
    cs.enter();
    if(!TimeOk && SYS::TimeOk)
    {
      SYS::checkNetTime(FLastEventTime);
      for(int i=Count()-1; i>=0; i--)
        SYS::checkNetTime( ((EVENT_DI*)at(i))->Time );
      TimeOk = TRUE;
    }
    BOOL r = Time < FLastEventTime;
    cs.leave();
    return r;
  }
    #if defined(__ARQ)
    void getData(void* Data,int Cnt){
      EVENT_DI* pED = (EVENT_DI*)Data;
      getn(Data,Cnt);
      while(Cnt-- > 0) {
        SYS::checkNetTime( (pED++)->Time );
      }
    }
    #endif

    void doSample(TIME Time)
    {
        csPoll.enter();
        U16 CT[2]={ChangedTo0, ChangedTo1};
        U16 St=Status;
        ChangedTo0=ChangedTo1=0;
        csPoll.leave();
        if((CT[0] | CT[1]) == 0) return;
        U16 bit=1;
        EVENT_DI Event;
        Event.Time=Time;
        for(int i=0; i<=15; i++,bit<<=1)
        {
            Event.Channel=i;
            if((PrevStatus ^ St) & bit)
            {
                // DI status changed
                Event.ChangedTo=(St & bit) ? 1 : 0;
                EventDigitalInput(Event);
                Event.Time++;
            }
            else
            {
                // Previous DI status == current status, but changing
                // might occured 2n times between sampling
                int j=(CT[0] & St & bit) ? 0 : 1;
                int k=j;
                do
                {
                    if(CT[k] & bit)
                    {
                        Event.ChangedTo=k;
                        EventDigitalInput(Event);
                        Event.Time++;
                    }
                    k^=1;
                }while(k!=j);
            }
        }
        PrevStatus=St;
    }
};
static PU_DI* PU_DI::Instance = NULL;


#ifdef __GPS_TIME_GPS721
// GPS-721 module
class PU_GPS_721 : public PU_ADC
{
    enum {updateDate, getDate, getTime, getNSats} state;
    U16 ChSum;
    U8  ChCnt;
    U8  ChFlg;
    U16 year,month,day,prevHour,prevSec;
    U16 prevPPS;
    U16 tmp;
public:
    U16 LatchedSum;
    U8  LatchedCnt;
    U8  LatchedFlg;
public:
    PU_GPS_721(U16 dummy):PU_ADC()
    {
        state = updateDate;
    }

    BOOL GetPollCmd(U8 *Buf)
    {
        U8 A = Module->GetAddress();
        Buf[1]=hex_to_ascii[A >> 4];
        Buf[2]=hex_to_ascii[A & 0xF];
        switch(state)
        {
            case updateDate: Buf[0]='$'; Buf[3]='D'; break;
            case getDate:    Buf[0]='#'; Buf[3]='4'; break;
            case getTime:    Buf[0]='#'; Buf[3]='1'; break;
            case getNSats:   Buf[0]='#'; Buf[3]='3'; break;
        }
        Buf[4]=0;
        return FALSE;
    }

  BOOL response(const U8 *Resp)
  {
        S(0x51);
        if(!Resp)
        {
            ChFlg|=flgEComm;
            return FALSE;
        }
        if(Resp[0]!='!')
        {
            ChFlg|=flgEResponse;
            return FALSE;
        }
        switch(state)
        {
            case updateDate:
                S(0x52);
                tmp = (U16)SYS::SystemTime;
                state = getDate;
                //dbg((const char*)Resp);
                break;
            case getDate:
                S(0x53);
                if((U16)SYS::SystemTime - tmp > 2000u) // wait 2s after updateDate
                {
                    day = (U16)FromDecStr(Resp+3,2);
                    month = (U16)FromDecStr(Resp+5,2);
                    year = (U16)FromDecStr(Resp+7,2) + 2000u;
                    if(1<=day && day<=31 && 1<=month && month<=12)
                        state = getTime;
                    else
                    {
                        state = updateDate;
                        ChFlg|=flgEDataFormat;
                    }
                }
                //ConPrintf("\n\r%s = %d-%d-%d",Resp,year,month,day);
                break;
            case getTime:
                S(0x54);
                {
                    U16 sec = (U16)FromDecStr(Resp+7,2);
                    if(prevSec==sec)
                    {
                        state = getNSats;
                        break;
                    }
                    prevSec = sec;

                    TIME sysTimeNow, timeGPS;
                    SYS::getSysTime(sysTimeNow);

                    TIME sysTimeOfHiPPS, sysTimeOfLoPPS;
                    GetCom(1).TimeOfCTS(sysTimeOfHiPPS, sysTimeOfLoPPS);

                    S16 betweenPPS = (S16)(sysTimeOfHiPPS - prevPPS);
                    S16 delta = (S16)(sysTimeNow-sysTimeOfHiPPS);
                    S(0x55);
                    S16 pulseLength = (S16)(sysTimeOfLoPPS-sysTimeOfHiPPS);
                    ConPrintf( "    GPS> %04d, %03d, %03d", betweenPPS, pulseLength, delta);

                    S(0x57);
                    BOOL NoPPS = (U16)sysTimeOfHiPPS == prevPPS;
                    prevPPS = (U16)sysTimeOfHiPPS;

                    if(NoPPS)
                    {
                        if(++tmp>32)
                        {
                            S(0x58);
                            tmp = 0;
                            ConPrint("\n\rGPS: No PPS pulse detected");
                            ChFlg|=flgEComm;
                        }
                    }
                    else
                    {
                        S(0x59);
                        U16 hour = (U16)FromDecStr(Resp+3,2);
                        U16 ph = prevHour;
                        prevHour = hour;
                        if(ph > hour) // start of new day
                        {
                            // update date
                            TIME time;
                            SYS::getNetTime(time);
                            SYS::DecodeDate(time,year,month,day);
                            //state = updateDate;
                            //break;
                        }
                        U16 min = (U16)FromDecStr(Resp+5,2);
                        //ConPrintf("\n\r%s = %d:%d:%d",Resp,hour,min,sec);
                        S(0x56);
                        if(!SYS::TryEncodeTime(year,month,day,hour,min,sec,timeGPS))
                        {
                            ChFlg|=flgEADCRange;
                            state = updateDate;
                            break;
                        }
                        TIME offs = timeGPS - sysTimeOfHiPPS;
                        if(offs>0 || !SYS::TimeOk)
                        {
                            S(0x5A);
                            if(betweenPPS<900 || betweenPPS>1100 || delta<300 || delta>900 || pulseLength<98 || pulseLength>102)
                            {
                                state = getNSats;
                                break;
                            }
                            S(0x5B);
                            TIME change = abs64(offs - SYS::NetTimeOffset);
                            if(900<change && change<1100)
                            {
                                S(0x5C);
                                if(++tmp<45)
                                {
                                    state = getNSats;
                                    break;
                                }
                                EVENT_DI Event;
                                SYS::getNetTime(Event.Time);
                                Event.Channel=255;
                                Event.ChangedTo=5;
                                PU_DI::Instance->EventDigitalInput(Event);
                                ConPrintf( "\n\rGPS: second jitter (%dms)", (U16)change);
                            }
                            S(0x5D);
                            SYS::setNetTimeOffset(offs);
                            state = getNSats;
                            tmp = 0;
                        }
                        else state = updateDate;
                    }
                }
                break;
            case getNSats:
                S(0x5E);
                U16 value = Resp[3]-'0';
                state = getTime;
                cs.enter();
                ChSum+=value;
                ChCnt++;
                cs.leave();
                break;
        }
        S(0x5F);
        return TRUE;
    }

  void latchPollData(){
    cs.enter();
    LatchedSum=ChSum; ChSum=0;
    LatchedCnt=ChCnt; ChCnt=0;
    LatchedFlg=ChFlg; ChFlg=0;
    cs.leave();
  }
  void doSample(TIME Time){
    U16 Sample;
    if (LatchedCnt>0){
      Sample = udiv(LatchedSum, LatchedCnt);
    }
    else {
      Sample=(flgError | LatchedFlg)<<8;
    }
    cs.enter();
    if(IsFull()) get(NULL);
    put(&Sample);
    FirstTime=Time-S32(Count()-1)*ADC_Period;
    cs.leave();
  }

}; // PU_GPS_721
#endif
