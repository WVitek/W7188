#pragma once
#include "SERVICE.h"

//#include <i86.h>

class DI_SVC : public SERVICE {
  TIME TimeOfLastSended;
  TIMEOUTOBJ toutAck;
//  BOOL NeedTx;
public:

DI_SVC():SERVICE(4)
{
  TimeOfLastSended=0;
//  NeedTx=FALSE;
}

BOOL HaveDataToTransmit(U8 /*To*/)
{
  return SYS::TimeOk && toutAck.IsSignaled() && Events.HaveNewEvents(TimeOfLastSended);// || NeedTx;
}

int getDataToTransmit(U8 /*To*/,void* Data,int MaxSize)
{
//  NeedTx = FALSE;
  TIME Time = TimeOfLastSended+1;
  int size = Events.readArchive(Time,(U8*)Data,MaxSize);
  if( size )
  {
    TimeOfLastSended = Time;
    if( size+10 >= MaxSize )
      toutAck.start(toTypeSec | 90); // maybe we have more events to send
    else
      size++; // one extra byte -> no more events
  }
  else size++; // send one byte -> no more events
  ConPrintf("\n\rALARMS: Tx %d\n\r",size/10);
  return size;
}

void receiveData(U8 /*From*/, const void* Data,int Size)
{
  if(Size!=sizeof(TIME)) return;
  ConPrint("\n\rALARMS: Rx ack\n\r");
//  NeedTx = TimeOfLastSended <= *(TIME*)Data;
  TimeOfLastSended = *(TIME*)Data;
  toutAck.setSignaled();
}

};

