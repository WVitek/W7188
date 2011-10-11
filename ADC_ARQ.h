#ifndef __SERVICE_H
#include "SERVICE.H"
#endif

#include <i86.h>

class ADC_SVC : public SERVICE {
  TIME* lastSendTime;
public:
  ADC_SVC();
  BOOL HaveDataToTransmit(U8 To);
  int getDataToTransmit(U8 To,void* Data,int MaxSize);
  void receiveData(U8,const void* /*Data*/,int /*Size*/){}
};

ADC_SVC::ADC_SVC():SERVICE(2)
{
  lastSendTime = (TIME*)SYS::malloc(sizeof(TIME)*plADC.Count());
  for(int i=plADC.Count()-1; i>=0; i--)
    lastSendTime[i]=0;
}

BOOL ADC_SVC::HaveDataToTransmit(U8){
  BOOL Res=FALSE;
  if(TimeSvc.TimeOk()){
    beginRead();
    for(int i=plADC.Count()-1; i>=0 && !Res; i--){
      PU_ADC *PU;
      PU=plADC_(i);
      Res=Res || (PU->GetLastTime() > lastSendTime[i]);
    }
    endRead();
  }
  return Res;
}

int ADC_SVC::getDataToTransmit(U8,void* Data,int MaxSize){
  if(!plADC.Count()) return 0;
  int iP = 0;
  TIME minTime=lastSendTime[0];
  for(int i=plADC.Count()-1; i>0; i--)
  {
    if(lastSendTime[i] >= minTime)
      continue;
    iP = i;
    minTime = lastSendTime[i];
  }
  int Cnt=(MaxSize-1-sizeof(TIME))/ASItemSize;
  PU_ADC* P = plADC_(iP);
  P->cs.enter();
  int size = P->readArchive(minTime,(U8*)Data+1+sizeof(TIME),Cnt);
  P->cs.leave();
  U8* Buf = (U8*)Data;
  Buf[0] = (U8)iP;
  *(TIME*)&(Buf[1]) = minTime;
  Cnt = size / ASItemSize;
  lastSendTime[iP] = minTime + Cnt*ADC_Period;
  return 1+sizeof(TIME)+Cnt*ASItemSize;
}

