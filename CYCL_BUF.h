#pragma once
#ifndef __CYCL_BUF_INC
#define __CYCL_BUF_INC

#ifndef __WKERN_HPP
#include "wkern.hpp"
#endif

#define __CBUFCAP (BufMask+1)

class CYCL_BUF : public OBJECT {
protected:
  void* Items;
  int First,Last;
  int FItemSize;
  U16 BufMask;
  int FCount;
public:
  CYCL_BUF(int ItemSize,U8 Log2Capacity);
  ~CYCL_BUF();
  inline int Count() { return FCount; }
  inline int Capacity() { return __CBUFCAP; }
  inline int ItemSize() { return FItemSize; }
  inline BOOL IsEmpty() { return FCount==0; }
  inline BOOL IsFull() { return FCount==__CBUFCAP; }

  void put(const void* Data)
  {
    memcpy(ItemPtr(Last),Data,FItemSize);
    ++Last &= BufMask;
    if(FCount>BufMask)
      First = Last;
    else
      FCount++;
  }

  void* ItemPtr(int Ndx)
  {
    return (U8*)Items+Ndx*FItemSize;
  }

  inline void* at(int Ndx)
  {
    return ItemPtr(Ndx & BufMask);
  }

  void read(void* Data){
    memcpy(Data,ItemPtr(First),FItemSize);
  }

  int readn(void* Data, int pos, int n){
    if( pos+n > FCount ) n=FCount-pos;
    if(Data)
    {
      U16 i1 = First+pos, i2 = i1+n; i1 &= BufMask; i2 &= BufMask;
      if (i1<i2)
        memcpy(Data,ItemPtr(i1),n*FItemSize); // read one fragment of data
      else
      { // read two fragments
        U16 sz1 = (__CBUFCAP-i1)*FItemSize;
        memcpy(Data,ItemPtr(i1),sz1);
        memcpy((U8*)Data+sz1,Items,i2*FItemSize);
      }
    }
    return n;
  }

  void get(void* Data){
    if (Data!=NULL) read(Data);
    ++First &= BufMask; FCount--;
  }

  int getn(void* Data,int n){
    if (Data!=NULL)
    { 
      n=readn(Data,0,n);
      First=(First+n)&BufMask;
      FCount-=n;
      return n;
    }
    else
      return 0;
  }

}; // CYCL_BUF

CYCL_BUF::CYCL_BUF(int ItemSize,U8 Log2Capacity){
  FItemSize=ItemSize;
  BufMask=(1<<Log2Capacity);
  Items=SYS::malloc(BufMask*FItemSize);
  BufMask--;
}

CYCL_BUF::~CYCL_BUF(){
  SYS::free(Items);
}

#endif // __CYCL_BUF_INC
