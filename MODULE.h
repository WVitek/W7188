#pragma once

#ifndef __STRING_H
  #include <string.h>
#endif
#include "WTLIST.hpp"


#define dtChecksumBit 0x40
U16 BaudCodeBaudRate[0x09]={
      0, // 0x00
      0, // 0x01
      0, // 0x02
   1200, // 0x03
   2400, // 0x04
   4800, // 0x05
   9600, // 0x06
  19200, // 0x07
  38400  // 0x08
};

class MODULE;
class POLL_UNIT;

typedef TLIST<MODULE*> MODULES;
typedef TLIST<POLL_UNIT*> PU_LIST;

MODULES Modules;
PU_LIST PollList;
PU_LIST PollListAll;

class MODULE : public OBJECT {
protected:
  U8 Address;  //*
  U8 BaudCode; //*   (see ADAM or ICPCON documentation)
  U8 InputType;//*
  U8 DataType; //*
public:
  PU_LIST* pollList;
  CRITICALSECTION cs;
  MODULE(int Addr=1, MODULES& ms=Modules, PU_LIST& pl=PollList)
  {
    pollList=&pl;
    BaudCode=0x08; DataType=dtChecksumBit; Address=Addr;
    ms.addLast(this);
  }
  U8 inline GetAddress()const{ return Address; }
  U8 inline GetBaudCode()const{ return BaudCode; }
  BOOL virtual GetConfigCmd(U8*/*Buf*/){ return FALSE; }
};

class I7017 : public MODULE {
  U8 ChMask; // used channels mask
  U8 State;
public:
  I7017(int Addr=1, MODULES &ms=Modules, PU_LIST& pl=PollList):MODULE(Addr,ms,pl){}
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
public:
  CRITICALSECTION cs;
  POLL_UNIT(MODULE *M,int ItSz,U8 Lg2Cp):CYCL_BUF(ItSz,Lg2Cp){
    Module=M;
  }
  int inline SafeCount(){
    cs.enter(); int r=Count(); cs.leave(); return r;
  }
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
#define ADC_Period (1000/ADC_Freq) // milliseconds

#define plADC_(i) ((PU_ADC*)plADC[i])

#if !defined(LOG2_ADC_BUF_SIZE)
#define LOG2_ADC_BUF_SIZE 14
#endif

PU_LIST plADC;

class PU_ADC : public POLL_UNIT
{
public:
  TIME FirstTime;
public:
  PU_ADC(MODULE *M):POLL_UNIT(M,ASItemSize, LOG2_ADC_BUF_SIZE)
  {
    if(Items)
    {
      if(M) { M->pollList->addLast(this); PollListAll.addLast(this); }
      plADC.addLast(this);
    }
    else ConPrint("\n\rPU_ADC:Items==NULL!!!");
  }

  inline TIME GetLastTime()
  { return FirstTime+smul(Count(),ADC_Period); }

  int readArchive(TIME &FromTime, U8 *Buf, U16 BufSize)
  {
//    cs.enter();
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
        r=readn(Buf,Pos,Cnt)*ASItemSize;
      else
        r=0;
    }
//    cs.leave();
    return r;
  }
}; // PU_ADC

#ifdef __MTU

#include "WFloat32.h"
class MTU_Coeffs
{
public:
    F32 C[9];
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
        // Ñ[0] + C[1] * T + C[2] * T*T
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
    U8  BusNum;
    //static TLIST<PU_ADC_MTU*> all;
    MTU_Coeffs coeffs;

    PU_ADC_MTU(int busNum):PU_ADC(NULL)
    {
        //all.addLast(this);
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
        if(cnt>10)
        { cnt = 0; return FALSE; }
        Resp++;
        int n=cnt<<1;
        for(int i=0; i<n; i++, Resp+=2)
            buf[i] = swap2bytes(*(U16*)Resp);
        return FALSE;
    }

    void latchPollData(){}

    //const static F32 fMaxP = 0x41C80000;
    const static F32 fMaxPto32K = 0x44A3D5C3; // 32767/25

    static U8 quant;
    static const U16 noDataErrCode = flgEComm << 8;

    void doSample(TIME Time)
    {
        U16 ps[10];
        for(int i=0; i<cnt; i++)
        {
            U16 p = buf[i<<1];
            U16 t = buf[(i<<1)+1];
            F32 fP = coeffs.calc(p,t);
//            if((quant++ & 0x3F)==0)
//            {
//                S32 realP = F32_to_1000scaled(fP);
//                char sign;
//                if(realP<0)
//                { sign='-'; realP=-realP; }
//                else sign = '+';
//                U16 Pfrac;
//                U16 Pint = udivmod(realP,1000,Pfrac);
//                ConPrintf("\n\rP%X=%c%d.%03d (p=%04X, t=%04X); ",
//                    BusNum, sign, Pint, Pfrac, p, t);
//            }
            S32 raw = F32_to_S32(fmul(fP,fMaxPto32K));
            if(raw<32767)
                ps[i] = (raw<0) ? 0 :(U16)raw;
            else ps[i] = flgEADCRange<<8;
        }
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
    }

}; // PU_ADC_MTU
static U8 PU_ADC_MTU::quant;
#endif

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
  PU_ADC_7K(I7017 *M,int Ch):PU_ADC(M)
  {
    Channel=Ch; M->useChannel(Ch);
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
//    ConPrint(" AIERR");
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
        // V=2.5; R=124; k=P*252.024; b=P*1638400
      #if   __SHOWADCDATA == 100
        S16 Val = S16((smul(Sample,25202)-163840000L)>>16); // P=100
      #elif __SHOWADCDATA == 60
        S16 Val = S16((smul(Sample,15121)- 98304000L)>>16); // P= 60
      #elif __SHOWADCDATA == 40
        S16 Val = S16((smul(Sample,10081)- 65536000L)>>16); // P= 40
      #elif __SHOWADCDATA == 25
        S16 Val = S16((smul(Sample, 6301)- 40960000L)>>16); // P= 25
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
        SYS::dbgLed(0x6a6ec);
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

struct EVENT_DI {
  TIME Time; U8 Channel; U8 ChangedTo;
};

#define AlarmEventSize sizeof(EVENT_DI)
// Digital Input Channels 0-7 (from 7063)

class PU_DI : public POLL_UNIT  {
protected:
  U16 ChMask,PrevStatus,Status,ChangedTo0,ChangedTo1;
  TIME FLastEventTime;
  BOOL TimeOk;
public:
  PU_DI(MODULE *M, U16 ChMask):POLL_UNIT(M,AlarmEventSize,8){
    PrevStatus=Status=this->ChMask=ChMask;
    if(ChMask){
      M->pollList->addLast(this);
      PollListAll.addLast(this);
    }
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
    if(Resp && Resp[0]=='>' && (Resp[5]==0)){
      U16 NewStatus=ChMask & (U16)FromHexStr(&(Resp[1]),4);
      cs.enter();
      ChangedTo0|=Status & ~NewStatus;
      ChangedTo1|=~Status & NewStatus;
      Status=NewStatus;
      cs.leave();
      return TRUE;
    }
//    ConPrint(" DIERR");
    return FALSE;
  }
  void EventDigitalInput(const EVENT_DI &Event){
    cs.enter();
    if(IsFull()) get(NULL);
    put(&Event);
    FLastEventTime=Event.Time;
    cs.leave();
    ConPrintf("\n\rValue at IN%d changed to %d",Event.Channel,Event.ChangedTo);
    setNeedConnection(TRUE);
  }
  void doSample(TIME Time);
  void getData(void* Data,int Cnt);

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
};

#if defined(__ARQ)
void PU_DI::getData(void* Data,int Cnt){
  EVENT_DI* pED = (EVENT_DI*)Data;
  getn(Data,Cnt);
  while(Cnt-- > 0) {
    SYS::checkNetTime( (pED++)->Time );
  }
}
#endif

void PU_DI::doSample(TIME Time){
  cs.enter();
  U16 CT[2]={ChangedTo0, ChangedTo1};
  U16 St=Status;
  ChangedTo0=ChangedTo1=0;
  cs.leave();
  if((CT[0] | CT[1]) == 0) return;
  U8 bit=1;
  EVENT_DI Event;
  Event.Time=Time;
  for(int i=0; i<=15; i++,bit<<=1){
    Event.Channel=i;
    if((PrevStatus ^ St) & bit){
      // DI status changed
      Event.ChangedTo=(St & bit) ? 1 : 0;
      EventDigitalInput(Event);
    }
    else{
      // Previous DI status == current status, but changing
      // might occured 2n times between sampling
      int j=(CT[0] & St & bit) ? 0 : 1;
      int k=j;
      do{
        if(CT[k] & bit){
          Event.ChangedTo=k;
          EventDigitalInput(Event);
        }
        k^=1;
      }while(k!=j);
    }
  }
  PrevStatus=St;
}
