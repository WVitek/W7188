#pragma once
#include "Service.h"

#include <i86.h>

class ADC_SVC : public SERVICE {
  BOOL Request;
  TIME FromTime;
public:

ADC_SVC():SERVICE(2)
{
}

BOOL HaveDataToTransmit(U8 /*To*/)
{
  return SYS::TimeOk && Request;
}


int getDataToTransmit(U8 /*To*/,void* Data,int MaxSize)
{
  Request=FALSE;
  int nADC = plADC.Count();
  if(!nADC) return 0;
  //TIME tmpTime = FromTime;

  // determine correct fromtime
  for(int i=0; i<nADC; i++)
  {
    PU_ADC* P = plADC_(i);
    P->cs.enter();
    P->readArchive(FromTime,NULL,0); // correct time value
  }
  // determine correct recsize
  U16 RecSize = (MaxSize-sizeof(TIME)-1) / plADC.Count();
  for(int i=0; i<nADC; i++)
  {
    PU_ADC* P = plADC_(i);
    RecSize = P->readArchive(FromTime,NULL,RecSize); // correct time value
  }
  // reading
  U8 *Dst = (U8*)Data;
  *(TIME*)Dst = FromTime; Dst+=sizeof(TIME);
  *Dst++ = (U8)nADC;
  for(int i=0; i<nADC; i++)
  {
    PU_ADC* P = plADC_(i);
    P->readArchive(FromTime,Dst,RecSize);
    Dst+=RecSize;
    P->cs.leave();
  }
  //ConPrintf("\n\r\ADC answer [Recs=%d FromTime: Q=%lld; A=%lld]\n\r", RecSize>>1, tmpTime, FromTime);
  if(RecSize)
    return sizeof(TIME)+1+RecSize*nADC;
  else
    return 0;
}

void receiveData(U8 /*From*/,const void* Data,int Size)
{
  if(Size == sizeof(TIME))
  {
    FromTime = *(TIME*)Data;
    Request = TRUE;
//    ConPrintf("\n\rADC request [FromTime=%lld]\n\r",FromTime);
  }
}

};
