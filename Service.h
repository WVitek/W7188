#ifndef __SERVICE_H
#define __SERVICE_H
#pragma once;

#include "wkern.hpp"

class SIMPLESERVICE {
public:
  BOOL virtual HaveDataToTransmit(U8 To)=0;
  int virtual getDataToTransmit(U8 To,void* Data,int MaxSize)=0;
  void virtual receiveData(U8 From,const void* Data,int Size)=0;
};

class SERVICE : public SIMPLESERVICE {
  CRITICALSECTION CS;
public:
  U8 ID;
  inline SERVICE(U8 ID){this->ID=ID;}
  inline void beginRead(){CS.enter();}
  inline void endRead(){CS.leave();}
  inline void beginWrite(){CS.enter();}
  inline void endWrite(){CS.leave();}
};

#endif
