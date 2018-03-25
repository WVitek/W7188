#pragma once

#include <stdlib.h>
#include "SERVICE.h"
#include "Hardware.hpp"

class DUMP_SVC : public SERVICE
{
  U32 Offset;
  BOOL Active;
public:

DUMP_SVC():SERVICE(5)
{
}

BOOL HaveDataToTransmit(U8 /*To*/)
{
  return Active;
}

#define MAX_DUMP_ADDR 0x100000

int getDataToTransmit(U8 /*To*/,void* Data,int MaxSize)
{
  U32 Ofs = Offset;
  U16 Size = MaxSize;
  if(Ofs == MAX_DUMP_ADDR){
    Active = FALSE;
    Size = 0;
  }
  else {
    if (Ofs+Size > MAX_DUMP_ADDR) Size = U16(MAX_DUMP_ADDR - Ofs);
    memcpy(Data,MK_FP(Ofs>>4,Ofs & 0xF),Size);
    Offset = Ofs+Size;
  }
  return Size;
}

#pragma argsused
void receiveData(U8 /*From*/,const void* Data,int/* Size*/)
{
  if(*(BOOL*)Data) {
    Offset=0x80000; Active=TRUE;
    ConPrint("\n\rDUMP START");
  }
  else {
    Active=FALSE;
    ConPrint("\n\rDUMP STOP");
  }
}

};
