#ifndef __SERVICE_H
#include "SERVICE.H"
#endif

//#include <i86.h>

//MODULE I7063(0xFF);
//PU_DI puDI(&I7063);

class DI_SVC : public SERVICE {
public:
  DI_SVC():SERVICE(4){};
  BOOL HaveDataToTransmit(U8);
  int getDataToTransmit(U8,void* Data,int MaxSize);
  void receiveData(U8,const void* /*Data*/,int /*Size*/){};
};

BOOL DI_SVC::HaveDataToTransmit(U8){
  if(puDI.SafeCount()>0)
    return TimeSvc.TimeOk();
  else{
    setNeedConnection(FALSE);
    return FALSE;
  }
}

int DI_SVC::getDataToTransmit(U8,void* Data,int MaxSize){
  if(!puDI.SafeCount()) return 0;
  puDI.cs.enter();
  int Cnt=MaxSize/AlarmEventSize;
  if(Cnt>puDI.Count()) Cnt=puDI.Count();
  puDI.getData(Data,Cnt);
  puDI.cs.leave();
  return Cnt*AlarmEventSize;
}

