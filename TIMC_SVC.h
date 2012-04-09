#pragma once

#include <stdlib.h>
#include "Service.h"
#include "wkern.hpp"

//***************************

#define TimeServerAddr 1

#ifdef __WIRE

#define TOUT_TimeRequest (toTypeSec | 5)

#ifdef __MTU
#define TOUT_ResyncInterval (toTypeSec | 10)
#else
#define TOUT_ResyncInterval (toTypeSec | 30)
#endif

#else

#define TOUT_TimeRequest (toTypeSec | 60)
#define TOUT_ResyncInterval (toTypeSec | 300)

#endif

#ifdef __GPRS_GR47
  #define __MaxTimeError 250
#else
  #define __MaxTimeError 125
#endif

#define MaxDataCnt 1

class TIMECLIENT_SVC : public SERVICE {

  struct _RESYNCPACKET{
    TIME TQTx,TQRx,TATx;
  };

  typedef TIME _RESYNCDATA[4];

  BOOL FTimeOk;
  TIME FLastReqNetTime;
  _RESYNCDATA* SD;
  int RDCnt;
  int IterCnt;
  int TryCnt;
  TIMEOUTOBJ toutResync;
  void processTimeData();
  void correctTime();
public:
  TIMECLIENT_SVC();
  ~TIMECLIENT_SVC(){SYS::free(SD);}
  BOOL HaveDataToTransmit(U8 To);
  int getDataToTransmit(U8 To,void* Data,int MaxSize);
  void receiveData(U8 From,const void* Data,int Size);
  inline BOOL TimeOk(){return FTimeOk;}
};

TIMECLIENT_SVC::TIMECLIENT_SVC():SERVICE(1){
  RDCnt=0;
  IterCnt=0;
  TryCnt=0;
  FTimeOk=FALSE;
  SD=(_RESYNCDATA*)SYS::malloc(MaxDataCnt*sizeof(_RESYNCDATA));
  toutResync.setSignaled();
}

BOOL TIMECLIENT_SVC::HaveDataToTransmit(U8 To)
{
  return To==TimeServerAddr && toutResync.IsSignaled();
}

#pragma argsused
int TIMECLIENT_SVC::getDataToTransmit(U8 To,void* Data,int/* MaxSize*/)
{
  if(To!=TimeServerAddr) return 0;
  toutResync.start(TOUT_TimeRequest);
  int Size = sizeof(_RESYNCPACKET);
  SYS::getNetTime(FLastReqNetTime);
#ifdef __GPRS_GR47
  FLastReqNetTime = FLastReqNetTime+100; // GR47 GPRS-terminal has a 100ms transmission delay (timeout)
#endif
  ((_RESYNCPACKET*)Data)->TQTx=FLastReqNetTime;
  //scramble data
  for(int i=0; i<Size; i++) ((U8*)Data)[i]^=i;
  return Size;
}

void TIMECLIENT_SVC::receiveData(U8 From,const void* Data,int Size){
  if(From!=TimeServerAddr) return;
  TIME TARx;
  SYS::getNetTime(TARx);
  //descramble data
  for(int i=0; i<Size; i++) ((U8*)Data)[i]^=i;
  _RESYNCPACKET &RP=*((_RESYNCPACKET*)Data);
  if(FLastReqNetTime!=RP.TQTx)
  {
    ConPrint("\n\rExtraordinary timepacket received");
    return;
  }
  TIME RQ=RP.TQRx-RP.TQTx;
  SD[IterCnt][RDCnt++]=RQ;
  TIME RS=TARx-RP.TATx;
  SD[IterCnt][RDCnt++]=RS;
  if(RDCnt==4) {
    RDCnt=0;
    if(++IterCnt==MaxDataCnt){
      IterCnt=0;
      processTimeData();
    }
  }
  else
    toutResync.setSignaled(); // urgent time request
}

void TIMECLIENT_SVC::processTimeData()
{
    static S16 StatOfs=0;
    TIME D1 = SD[0][0] + SD[0][1]; // (QRx-QTx)+(ARx-ATx) = (ARx-QTx)-(ATx-QRx) = total delay in channel
    TIME D2 = SD[0][2] + SD[0][3];
    TIME TOfs0 = (SD[0][0] - SD[0][1]); // ( (QRx-QTx)-(ARx-ATx) ) = 2x time offset
    TIME TOfs1 = (SD[0][2] - SD[0][3]);
    TIME Err = ( abs64(TOfs0-TOfs1)+abs64(D1-D2) ) >> 1;
    TIME TOfs = (TOfs0+TOfs1)>>2;
#ifdef __WATCOMC__
    ConPrintf("\n\rTime sync [D1=%dms D2=%dms Err=%d TOfs=%Ld] ",
        (int)D1,(int)D2,(int)Err,TOfs
    );
#else
    ConPrintf("\n\rD1=%dms D2=%dms Err=%d TOfs=%p:%p\n\r",
        (int)D1.Lo(),(int)D2.Lo(),(int)Err.Lo(),TOfs.Hi(),TOfs.Lo()
    );
#endif

#ifdef __GPS_TIME
    RDCnt=0;
    TryCnt=0;
    toutResync.start(TOUT_ResyncInterval);
#else
    if(Err < abs64(TOfs) && Err<TIME(__MaxTimeError))
    {
        RDCnt=0;
        TryCnt=0;
        SYS::setNetTimeOffset(SYS::NetTimeOffset+TOfs);
        if(abs64(TOfs)<TIME(18000))
            StatOfs=StatOfs+(S16)TOfs;
        if(!FTimeOk)
        {
            FTimeOk = TRUE;
            ConPrintf("+++Time sync OK. Err=%d",(U16)Err);
        }
        else
        {
            TIME CurTime;
            SYS::getSysTime(CurTime);
            #ifdef __WATCOMC__
            ConPrintf("+++Time sync OK. StatOfs=%d/%Ld\n\r", StatOfs,CurTime);
            #else
            ConPrintf("+++Time sync OK. StatOfs=%d/%p:%p\n\r", StatOfs,CurTime.Hi(),CurTime.Lo());
            #endif
        }
    }
    else
    {
        if(++TryCnt>=2 && FTimeOk)
        {
            ConPrint("---Ne v etot raz :-)");
            RDCnt=0;
            TryCnt=0;
        }
        else
        {
            ConPrint("---Time sync oblom");
            SD[0][0]=SD[0][2];
            SD[0][1]=SD[0][3];
            RDCnt=2;
        }
    }
    if(!FTimeOk)
        toutResync.setSignaled(); // urgent time request
    else
        toutResync.start(TOUT_ResyncInterval);
#endif
}
