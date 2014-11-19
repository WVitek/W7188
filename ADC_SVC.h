#pragma once
#include "Service.h"

#include <i86.h>

class ADC_SVC : public SERVICE {
  BOOL Request;
  TIME FromTime;
  ADC_LIST *plADC;
public:

//#define plADC_(i) ((PU_ADC*)(*plADC)[i])

ADC_SVC(ADC_LIST *ADCs, U8 ID):SERVICE(ID)
{
    plADC = ADCs;
}

BOOL HaveDataToTransmit(U8 /*To*/)
{
  return SYS::TimeOk && Request;
}

int getDataToTransmit(U8 /*To*/,void* Data,int MaxSize)
{
  Request=FALSE;
  int nADC = plADC->Count();
  if(!nADC)
      return 0;
  TIME tmpTime = FromTime;
  // determine correct fromtime
  for(int i=0; i<nADC; i++)
  {
    PU_ADC* P = (*plADC)[i];
    P->cs.enter();
    P->readArchive(FromTime,NULL,0); // correct time value
  }
  if(FromTime<SYS::NetTimeOffset)
    return 0;
  // determine correct recsize
  U16 BytesPerCh = (MaxSize-sizeof(TIME)-1) / nADC;
  for(int i=0; i<nADC; i++)
  {
    PU_ADC* P = (*plADC)[i];
    BytesPerCh = P->readArchive(FromTime,NULL,BytesPerCh); // correct time value
  }
  // reading
  U8 *Dst = (U8*)Data;
  *(TIME*)Dst = FromTime; Dst+=sizeof(TIME);
  *Dst++ = (U8)nADC;
  for(int i=0; i<nADC; i++)
  {
    PU_ADC* P = (*plADC)[i];
    P->readArchive(FromTime,Dst,BytesPerCh);
    Dst+=BytesPerCh;
    P->cs.leave();
  }
  //ConPrintf("\n\r\ADC answer [Recs=%d FromTime: Q=%lld; A=%lld]\n\r", BytesPerCh>>1, tmpTime, FromTime);
  if(BytesPerCh)
    return sizeof(TIME)+1+BytesPerCh*nADC;
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
