#ifndef __SERVICE_H
#include "SERVICE.H"
#endif

//#include <i86.h>

class DI_SVC : public SERVICE {
public:
  DI_SVC():SERVICE(4){};
  BOOL HaveDataToTransmit(U8);
  int getDataToTransmit(U8,void* Data,int MaxSize);
  void receiveData(U8,const void* /*Data*/,int /*Size*/){};
};

BOOL DI_SVC::HaveDataToTransmit(U8){
  if(Events.SafeCount()>0)
    return SYS::TimeOk;
  else{
    setNeedConnection(FALSE);
    return FALSE;
  }
}

int DI_SVC::getDataToTransmit(U8,void* Data,int MaxSize){
  if(!Events.SafeCount()) return 0;
  Events.cs.enter();
  int Cnt=MaxSize/AlarmEventSize;
  if(Cnt>Events.Count()) Cnt=Events.Count();
  Events.getData(Data,Cnt);
  Events.cs.leave();
  return Cnt*AlarmEventSize;
}

