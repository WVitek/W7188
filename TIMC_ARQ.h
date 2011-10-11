#include <stdlib.h>

#ifndef __SERVICE_H
#include "SERVICE.h"
#endif

#ifndef __WKERN_HPP
#include "wkern.hpp"
#endif

//#define __UseNewTimeSyncAlg
//***************************

#define TimeServerAddr 1

#define TOUT_TimeRequest (toTypeSec | 8)
#define TOUT_ResyncInterval (toTypeSec | 600)

#define TIMEREQUEST_TIMEOUT 8000

#ifdef __UseNewTimeSyncAlg

#define Log2MIC 1
#define MaxDataCnt (1<<Log2MIC)

#else

#define MaxDataCnt 1

#endif

class TIMECLIENT_SVC : public SIMPLESERVICE {

  struct _RESYNCPACKET{
    TIME TQTx,TQRx,TATx;
  };

  typedef TIME _RESYNCDATA[4];

  BOOL FTimeOk;
  TIME FLastReqTime;
  _RESYNCDATA SD[MaxDataCnt];
  BOOL FReplyOk;
  int RDCnt;
  int IterCnt;
  int TryCnt;
  TIMEOUTOBJ toutResync;
  void processTimeData();
  void correctTime();
public:
  TIMECLIENT_SVC();
  BOOL HaveDataToTransmit(U8 To);
  int getDataToTransmit(U8 To,void* Data,int MaxSize);
  void receiveData(U8 From,const void* Data,int Size);
  inline BOOL TimeOk(){return FTimeOk;};
};

TIMECLIENT_SVC::TIMECLIENT_SVC(){
  RDCnt=0;
  IterCnt=0;
  TryCnt=0;
  FTimeOk=FALSE;
  FLastReqTime=0;
  FReplyOk=TRUE;
  toutResync.setSignaled();
}

BOOL TIMECLIENT_SVC::HaveDataToTransmit(U8){
  return toutResync.IsSignaled();
/*
  if (FNeedResync) {
    if (FReplyOk) return TRUE;
    TIME Now;
    SYS::getNetTime(Now);
    return FLastReqTime+TIMEREQUEST_TIMEOUT < Now;
  }
  else return FALSE;
//*/
}

#define TPL5 100
#define TPL3 33

#pragma argsused
int TIMECLIENT_SVC::getDataToTransmit(U8,void* Data,int/* MaxSize*/){
  FReplyOk=FALSE;
  toutResync.start(TOUT_TimeRequest);
#ifdef __UseNewTimeSyncAlg
  int Size = ((RDCnt==0) ? TPL5 : TPL3) - PRTDataSize;
#else
  int Size = sizeof(_RESYNCPACKET);
#endif
  TIME Time;
  SYS::getNetTime(Time);
  ((_RESYNCPACKET*)Data)->TQTx=Time;
  FLastReqTime=Time;
  //scramble data
  for(int i=0; i<Size; i++) ((U8*)Data)[i]^=i;
  return Size;
}

void TIMECLIENT_SVC::receiveData(U8,const void* Data,int Size){
  TIME TARx;
  SYS::getNetTime(TARx);
  //descramble data
  for(int i=0; i<Size; i++) ((U8*)Data)[i]^=i;
  _RESYNCPACKET &RP=*((_RESYNCPACKET*)Data);
  if(FLastReqTime!=RP.TQTx){
    ConPrint("\n\rExtraordinary timepacket received");
    return;
  }
  //if(FTimeOk && !FNeedResync) return;
  FReplyOk=TRUE;
  TIME RQ=RP.TQRx-RP.TQTx;
  SD[IterCnt][RDCnt++]=RQ;
  TIME RS=TARx-RP.TATx;
  SD[IterCnt][RDCnt++]=RS;
//  ConPrintf("  %6d",S16(RS-RQ));
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

#ifndef __UseNewTimeSyncAlg

void TIMECLIENT_SVC::processTimeData(){
  static S16 StatOfs=0;
  TIME D1 = SD[0][1] + SD[0][0];
  TIME D2 = SD[0][3] + SD[0][2];
  TIME TOfs0 = (SD[0][0] - SD[0][1])>>1;
  TIME TOfs1 = (SD[0][2] - SD[0][3])>>1;
  TIME Err = abs64(TOfs0-TOfs1)+abs64(D1-D2);
  TIME TOfs = (TOfs0+TOfs1)>>1;
//*
#ifdef __WATCOMC__
  ConPrintf("\n\rD1=%dms D2=%dms Err=%d TOfs=%Ld\n\r",
    (int)D1,(int)D2,(int)Err,TOfs
  );
#else
  ConPrintf("\n\rD1=%dms D2=%dms Err=%d TOfs=%p:%p\n\r",
    (int)D1.Lo(),(int)D2.Lo(),(int)Err.Lo(),TOfs.Hi(),TOfs.Lo()
  );
#endif
//*/
  if(Err < abs64(TOfs)){
    TIME CurTime;
    SYS::getNetTime(CurTime);
    CurTime += TOfs;
    SYS::setNetTime(CurTime);
    if(abs64(TOfs)<TIME(18000)) StatOfs=StatOfs+(S16)TOfs;
    //FNeedResync=FALSE;
    RDCnt=0;
    TryCnt=0;
    if(!FTimeOk){
      //SYS::StartTime=TOfs;
      FTimeOk=TRUE;
      ConPrint("+++Time sync OK.");
    }
    else{
      //CurTime=CurTime-SYS::StartTime;
#ifdef __WATCOMC__
      ConPrintf("+++Time sync OK. StatOfs=%d/%Ld\n\r",
        StatOfs,CurTime
      );
#else
      ConPrintf("+++Time sync OK. StatOfs=%d/%p:%p\n\r",
        StatOfs,CurTime.Hi(),CurTime.Lo()
      );
#endif
    }
  }
  else{
    if(++TryCnt>=2){
      ConPrint("---Ne v etot raz :-)");
      //FNeedResync=FALSE;
      RDCnt=0;
      TryCnt=0;
    }
    else{
      ConPrint("---Time sync oblom");
      SD[0][0]=SD[0][2];
      SD[0][1]=SD[0][3];
      RDCnt=2;
    }
  }
  if(FTimeOk)
    toutResync.start(TOUT_ResyncInterval);
  else
    toutResync.setSignaled(); // urgent time request
}

#else

U16 Sqrt(U32 Y){
  U16 x,x1=0,x2=65535;
  U32 y,y1=0,y2=65535ul*65535;
  if(y2<Y) return x2;
  while(x1+1<x2){
    x=(x2+x1)>>1;
    y=umul(x,x);
    if (y<Y){ x1=x; y1=y; }
    else if (Y<y) { x2=x; y2=y; }
    else return x;
  }
  if( Y-y1 < y2-Y ) return x1; else return x2;
}

U16 Get_t0_512(int n){
  // Fixed-point represented values (9 bit fraction part)
  const U16 t0[21]={
    32592, 5082, 2991, // 1,2,3
     2357, 2064, 1898, // 4,5,6
     1791, 1718, 1664, // 7,8,9
     1623, 1564, 1524, // 10,12,14
     1496, 1474, 1457, // 16,18,20
     1443, 1432, 1423, // 22,24,26
     1415, 1408, 1319  // 28,30,inf
  };
  if (n>30) n=21;
  else if (n>10) n=10 + ((n-10) >> 1);
  else if (n<1) n=1;
  return t0[n-1];
}

int DataFilter(
  const S16 *Data, int DataCnt, int MinCnt,
  BOOL *Valid, S16 *Average, U16 *Sigma
){
  BOOL *V;
  if(Valid) V=Valid;
  else V=new BOOL[DataCnt];
  memset(V,0xFF,DataCnt);
  S16 Avg;
  U16 Sig;
  int Cnt=DataCnt;
  BOOL Outliers;
  do{
    // Calculate average
    S32 Tmp=0;
    int i;
    for(i=0; i<DataCnt; i++) if(V[i]) Tmp+=Data[i];
    Avg=sdiv(Tmp,Cnt);
    Tmp=0;
    for(i=0; i<DataCnt; i++) if(V[i]){
      S16 x=Data[i]-Avg;
      Tmp+=smul(x,x);
    }
    Sig=Sqrt(Tmp/Cnt);
    U16 Limit = 3*Sig>>1;
    // Filter outliers
    Outliers=FALSE;
    for(i=0; i<DataCnt; i++) if(V[i]){
      S16 x=Data[i]-Avg;
      if(abs(x) > Limit){ V[i]=FALSE; Cnt--; Outliers=TRUE; }
    }
  }while(Outliers && Cnt>MinCnt);
  if(Average) *Average=Avg;
  if(Sigma) *Sigma=Sig;
  if(!Valid) delete V;
  return Cnt;
}

void TIMECLIENT_SVC::processTimeData(){
  static TIME StatOfs=0;
  TIME TOfs;
  if(abs64(SD[0][0]) > TIME(TIMEREQUEST_TIMEOUT))
    TOfs=SD[0][0];
  else {
    S16 D[4][MaxDataCnt];
    // Prepare data
    int i,j;
    for(i=0; i<MaxDataCnt; i++){
      ConPrintf("\n\r");
      for(int j=0; j<4; j++){
        S16 Tmp=(S16)SD[i][j];
        D[j][i]=Tmp;
        ConPrintf("%5d",Tmp);
      }
    }
    // Filter data
    U16 Sigma[4];
    int Cnt[4]={MaxDataCnt,MaxDataCnt,MaxDataCnt,MaxDataCnt};
    S16 Avg[4];
    for(j=0; j<4; j++) {
      Cnt[j]=DataFilter(
        D[j], MaxDataCnt, MaxDataCnt>>1,
        NULL, &(Avg[j]), &(Sigma[j])
      );
      ConPrintf(
        "\n\rD[%d]: Cnt=%2d Avg=%4d Sigma=%3d",
        j, Cnt[j], Avg[j], Sigma[j]
      );
    }
    // Calculate "dispersion"
    S16 DD[2][MaxDataCnt*2];
    U16 DSig[2];
    S16 DAvg[2];
    for(i=0; i<MaxDataCnt; i++){
      DD[0][(i<<1)+0]=D[0][i]-Avg[0];
      DD[0][(i<<1)+1]=D[2][i]-Avg[2];
      DD[1][(i<<1)+0]=D[1][i]-Avg[1];
      DD[1][(i<<1)+1]=D[3][i]-Avg[3];
    }
    BOOL Oblom=FALSE;
    for(j=0; j<2; j++) {
      int Cnt=DataFilter(DD[j],MaxDataCnt*2,MaxDataCnt,NULL,&DAvg[j],&DSig[j]);
      ConPrintf("\n\rDD[%d]: Cnt=%2d DSig=%u",j,Cnt,DSig[j]);
      if(Cnt<MaxDataCnt) Oblom=TRUE;
    }
    // Solve equation
    U16 kA = DSig[0];
    U16 kB = DSig[1];
    S16 Denom=(TPL3-TPL5)*(kA+kB);
    if (Denom==0) Denom=TPL3-TPL5;
    S16 P = sdiv( S32(Avg[0]+Avg[3])*TPL3 - S32(Avg[1]+Avg[2])*TPL5, Denom);
    S16 d = sdiv(
      ( S32(Avg[0])*TPL3 - S32(Avg[2])*TPL5 ) * kB +
      ( S32(Avg[1])*TPL5 - S32(Avg[3])*TPL3 ) * kA,
      Denom);
    S16 A = Avg[0]-Avg[2];
    S16 B = Avg[3]-Avg[1];
    //
    ConPrintf("\n\rkA=%3d kB=%3d (A=%3d B=%3d)/%d P=%d",
      kA, kB, A, B, TPL5-TPL3, P);
    if(Oblom || P<0 || A<=0 || B<=0){
      ConPrint("\n\r--- Time sync oblom");
      return;
    }
    TOfs=d;
  }
  TIME CurTime;
  SYS::getTime(CurTime);
  SYS::setTime(CurTime+TOfs);
  if(abs64(TOfs) > TIME(3600000L)){
    SYS::StartTime=TOfs;
#ifdef __WATCOMC__
    ConPrintf("\n\r+++ Time sync. TOfs=%Ld", TOfs );
#else
    ConPrintf("\n\r+++ Time sync. TOfs=%p:%p", TOfs.HP(), TOfs.LP() );
#endif
  }
  else {
    FNeedResync=FALSE;
    if(!FTimeOk){
      SYS::StartTime+=TOfs;
      FTimeOk=TRUE;
      CurTime=0;
    }
    else {
      TOfs=TOfs>>1;
      CurTime=CurTime-SYS::StartTime;
      StatOfs=StatOfs+TOfs;
    }
#ifdef __WATCOMC__
    ConPrintf("\n\r+++ Time sync. StatOfs=%d/(%Ld) TOfs=%d",
      S16(StatOfs), CurTime, (S16)TOfs );
#else
    ConPrintf("\n\r+++ Time sync. StatOfs=%d/(%p:%p) TOfs=%d",
      S16(StatOfs), CurTime.HP(), CurTime.LP(), (S16)TOfs.LP() );
#endif
  }
}

#endif
