#pragma once
#ifndef __WTLIST_HPP
#define __WTLIST_HPP

#ifndef __WKERN_HPP
#include "WKern.hpp"
#endif

template <class T> class TLIST : public OBJECT {
protected:
  T *v;
  int FCount,FCapacity;
public:
  inline T&   operator[](int i) {return v[i];}
  void        addLast(const T Value);
  inline T&   First() {return v[0];}
  inline int  Capacity(){return FCapacity;}
  inline int  Count(){return FCount;}
  void        insert(int Pos,const T Value);
  inline T&   Last() {return v[FCount-1];}
  inline T*   PtrAt(int i){ return &(v[i]); }
  void        remove(int Pos);
  inline void removeFirst(){ remove(0); }
  inline void removeLast(){ setCount(FCount-1); }
  void        setCapacity(int NewCap);
  void        setCount(int NewCnt);
  ~TLIST(){delete v;}
};


template <class T> void TLIST<T>::addLast(const T Value) {
  setCount(FCount+1);
  v[FCount-1]=Value;
}

template <class T> void TLIST<T>::insert(int Pos,const T Value) {
  setCount(FCount+1);
  if(Pos<FCount-1) memmove(PtrAt(Pos+1),PtrAt(Pos),sizeof(T)*(FCount-1-Pos));
  v[Pos]=Value;
}

template <class T> void TLIST<T>::remove(int Pos) {
  FCount--;
  if(Pos<FCount) memcpy(PtrAt(Pos),PtrAt(Pos+1),sizeof(T)*(FCount-Pos));
}

template <class T> void TLIST<T>::setCapacity(int NewCap)
{
  if(NewCap==FCapacity) return;
  if(NewCap<8) NewCap=8;
  T *nv=NULL;
  if(NewCap) nv=(T*)SYS::realloc(v,sizeof(T)*NewCap);
  v=nv;
  FCapacity=NewCap;
}

template <class T> void TLIST<T>::setCount(int NewCnt) {
  if(NewCnt>FCapacity) setCapacity(NewCnt);
  else if(NewCnt<FCount) memset(PtrAt(NewCnt),sizeof(T)*(FCount-NewCnt),0);
  FCount=NewCnt;
}

#endif // __TEMPLATE_HPP
