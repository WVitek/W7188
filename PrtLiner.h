#pragma once

#include "Comms.hpp"

#define LINER_BUFSIZE 1024
#define RX_TIMEOUT (toTypeSec | 3)

int __rxCompare(const void *Rx, const char *Resp, int RxSize)
{
    const U8 *buf = (const U8*)Rx;
    const U8 *txt = (const U8*)Resp;
    int i = RxSize;
    while(TRUE){
        U8 b = *txt;
        if(b == 0)
            return i; // (i>0) ? (Resp is begin of Rx) : (full match)
        if(i-- <= 0)
            return -1; // not matched (not enough data rec'vd)
        if(b != *buf)
            return -2; // not matched (no response match)
        txt++; buf++;
    }
}

inline bool __rxequal(const void *Rx, const char *Resp, int RxSize)
{
    return __rxCompare(Rx,Resp,RxSize) == 0;
//  return (RxSize == strlen(Resp)) && (_strnicmp((const char*)Rx, Resp, RxSize) == 0);
}

bool __rxEnd(const void *Rx, const char *Resp, int RxSize)
{
  int Len = strlen(Resp);
  return (RxSize >= Len) && (_strnicmp((const char*)Rx+RxSize-Len, Resp, Len) == 0);
}

bool __rxBeg(const void *Rx, const char *Resp, int RxSize)
{
    return __rxCompare(Rx,Resp,RxSize) >= 0;
//  int Len = strlen(Resp);
//  return (RxSize >= Len) && (_strnicmp((const char*)Rx, Resp, Len) == 0);
}

class PRT_LINER: public PRT_ABSTRACT {
  COMPORT *com;
  int BufPos;
  BOOL RxReady,SavedNeedXor;
  U8 Buf[LINER_BUFSIZE];
  // Halfduplex transmission objects
  BOOL PauseTx, WasNullRx;
  TIMEOUTOBJ TxTimeout, RxTimeout;
public:

static U16 GetTxRealSize(const void *Data, U16 Cnt)
{
  const U8 *Src = (const U8 *)Data;
  U16 Res = 1;
  while( Cnt-- > 0)
  {
    switch( *Src++ )
    {
    case 0x0D:
    case 0x7D:
    case 0x0A:
      Res+=2;
      break;
    default:
      Res++;
    }
  }
  return Res;
}

inline const void* GetBuf(int &BufPos) const { BufPos = this->BufPos; return Buf; }
inline BOOL HalfDuplex() { return com->Flags & CF_HALFDUPLEX; }
void clearRxBuf()
{
  com->clearRxBuf();
  RxReady = FALSE;
  BufPos = 0;
}
void TxCmd(const char* pszCmd)
{
  clearRxBuf();
  TxStr(pszCmd);
}
/*
void TxCtrlZ()
{
  com->writeChar(0x1A);
}
//*/
public:

PRT_LINER(COMPORT *com)
{
  this->com=com;
  State = psInitial;
}

void Open()
{
  com->setExpectation(0xFF,0x0D);
  BufPos=0;
  RxReady=SavedNeedXor=FALSE;
  PauseTx=WasNullRx=FALSE;
  RxTimeout.start( RX_TIMEOUT );
  State = psOpened;
}

int RxSize()
{
  return (RxReady)? BufPos : 0;
}

void Close()
{
  State = psInitial;
}

int Rx(void *Data){
  if(RxReady){
    RxReady = FALSE;
    int R=BufPos; BufPos=0;
    if(Data != NULL) memcpy(Data,&(Buf[0]),R);
    //ConPrintf("\n\r Debug Prt 0 R=%d ", R);
    return R;
  }
  else{
    //ConPrint("\n\r   Debug Prt 0 ");
    return 0;
  }
}

#define _TxBufSize 32
void Tx(const void *Data, U16 Cnt){
  BOOL TxData = FALSE, TxEOL = TRUE;
  if(HalfDuplex()) {
    // if (have data) or (last rx is not null) or (urgent null tx)
    if( Cnt>0 || !WasNullRx || Data==NULL ) {
      TxData = Cnt>0;
      PauseTx = TRUE;
      TxTimeout.stop();
//      if(Cnt) ConPrintf(".:Tx=%d:.",Cnt);
    }
    else TxEOL = FALSE;
  }
  else
    TxData = Cnt>0;
  if(TxData)
  {
    //dbg2("\n\rTX %d byte(s):",Cnt);
    U8 TxBuf[_TxBufSize];
    const U8 *Src = (const U8*)Data;
    U16 TxPos = 0;
    for(; Cnt>0; Cnt--, Src++){
      U8 c = *Src;
      if(c==0x0D || c==0x7D || c==0x0A)// || c==0x1A) // 0x1A is CtrlZ - special for SIM300's "at+cipsend"
      {
        TxBuf[TxPos++] = 0x7D;
        if(TxPos==_TxBufSize){
          com->write(TxBuf,_TxBufSize);
          //dump(TxBuf,_TxBufSize);
          TxPos=0;
        }
        c^=0x20;
      }
      TxBuf[TxPos++] = c;
      if(TxPos==_TxBufSize){
        com->write(TxBuf,_TxBufSize);
        //dump(TxBuf,_TxBufSize);
        TxPos=0;
      }
    }
    if(TxPos>0){
      com->write(TxBuf,TxPos);
      //dump(TxBuf,TxPos);
    }
  }
  if(TxEOL)
    com->writeChar(0x0D);
}

U16 ProcessIO(){
  U16 Result=0;
  //********** RX
  int i=com->BytesInRxB();
  //ConPrintf("\n\r Debug Prt 1 i=%d ", i);
  if(!RxReady && i>0 )
  {
    //ConPrintf("\n\r   Debug PrtLiner 1 2 BufPos=(%d)", BufPos);
    int Pos=BufPos;
    BOOL NeedXor = SavedNeedXor;
    while(i>0)
    {
      U8 c=com->readChar(); i--;
      switch(c)
      {
      case 0x0D: // Carriage Return
        RxReady = TRUE;
        i=0; // break cycle
        break;
      case 0x7D: // ESC prefix '}'
        NeedXor=TRUE;
        break;
      case 0x0A: // Line Feed
        break;
      default:
        if(Pos<LINER_BUFSIZE){
          U8 b;
          if(NeedXor){ b=c^0x20; NeedXor=FALSE; } else b=c;
          Buf[Pos++]=b;
        }
      }
    }
    if(RxReady)
    {
      NeedXor=FALSE;
      if(HalfDuplex())
      {
        PauseTx=FALSE;
        RxTimeout.start( RX_TIMEOUT );
        TxTimeout.start(50); // 50 ms timeout
        WasNullRx = Pos==0;
//        if(Pos) ConPrintf(".:Rx=%d:.",Pos);
      }
    }
    SavedNeedXor=NeedXor;
    BufPos=Pos;
  }
  if(RxReady) Result|=IO_RX;
  //ConPrintf("\n\r   Debug PrtLiner 1 3 BufPos=(%d)", i);

  //********** TX
  if(HalfDuplex())
  {
    BOOL SomeTx = !PauseTx && TxTimeout.IsSignaled();
    if( !SomeTx && RxTimeout.IsSignaled() ){
      SomeTx = TRUE;
      RxTimeout.start( RX_TIMEOUT );
    }
    if( SomeTx ) Tx( NULL, 0 );
  }
  //
  U16 bct;
  if(!PauseTx && /*com->IsClearToSend() &&*/ (bct = com->BytesCanTx()) > 0 )
  {
    Result|=IO_TX;
    if(bct==COMBUF_SIZE) Result|=IO_TXEMPTY;
  }
  //********** State
  if(State == psOpened)
    Result |= IO_UP;
  return Result;
}

};

