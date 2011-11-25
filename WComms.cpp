//file WComms.cpp

#include <stdio.h>
#include <string.h>
#include "WComms.hpp"
#include "WKern.hpp"
#include "WHrdware.hpp"
#include "WCRCs.hpp"
#ifndef __BORLANDC__
#include <stdarg.h>
#endif

//class COMPORT

COMPORT::COMPORT(){
  RxB.WrP=RxB.RdP=TxB.WrP=TxB.RdP=0;
  RxB.ExpMask=0;
  RxB.ExpChar=1;
  RxCnt=0;
  OldIntVect=0;
  StrBuf=0;
//  SYS::enableThreadSwitching(FALSE);
//  ComNum=Num;
//  this->Baud=Baud;
//  InstallCom(ComNum,Baud);
//  HalfDuplex=(ComNum==2);
//  SYS::enableThreadSwitching(TRUE);
}

COMPORT::~COMPORT(){
  if(StrBuf) SYS::free(StrBuf);
//  if(OldIntVect) uninstall();
}

#ifdef __UsePerfCounters
void COMPORT::GetPerf(STRUCT_COMPERF *P){
  _disable();
  U32 Ticks = ISRTicks;  ISRTicks=0;
  P->RCalls=_RC; P->RBytes=_RB;
  P->TCalls=_TC; P->TBytes=_TB;
  _RC=_RB=_TC=_TB=0;
  _enable();
  P->ISRMs=(U16)(Ticks/SysTicksInMs);
}
#endif

void COMPORT::setExpectation(U8 Mask,U8 Char,U16 Count){
  SYS::cli();
  RxB.Event.reset();
  RxB.ExpMask=Mask;
  RxB.ExpChar=Char;
  RxCnt=Count;
  SYS::sti();
}

U8 _fast COMPORT::readChar(){
  while(!BytesInRxB()) SYS::sleep(1);
  SYS::cli();
  U8 Char=RxB.Data[RxB.RdP++];
  RxB.RdP&=COMBUF_WRAPMASK;
  SYS::sti();
  return Char;
}

U16 COMPORT::read(void* Buf,U16 Cnt,int Timeout){
  if(Timeout){
    if(Cnt>COMBUF_SIZE) Cnt=COMBUF_SIZE;
    SYS::cli();
    if(RxB.BytesOccupied()<Cnt) {
      setExpectation(0,0,Cnt-RxB.BytesOccupied()); //any char
      SYS::sti();
      RxB.Event.waitFor(Timeout);
    }
    else SYS::sti();
    U16 i=0;
    for(;i<Cnt;i++){
      _disable();
      if(RxB.RdP==RxB.WrP)
      {
          _enable();
          break;
      }
      ((U8*)Buf)[i]=RxB.Data[RxB.RdP++];
      RxB.RdP&=COMBUF_WRAPMASK;
      _enable();
    }
    Cnt=i;
  }
  else for(int i=0; i<Cnt; i++) ((U8*)Buf)[i]=readChar();
  return Cnt;
}

#ifdef __COM_STATUS_BUF
U16 COMPORT::read(void* Buf, void* Status, U16 Cnt, int Timeout)
{
    if(Timeout)
    {
        if(Cnt>COMBUF_SIZE) Cnt=COMBUF_SIZE;
        SYS::cli();
        if(BytesInRxB()<Cnt)
        {
            setExpectation(0,0,Cnt-BytesInRxB()); //any char
            SYS::sti();
            RxB.Event.waitFor(Timeout);
        }
        else SYS::sti();
        U16 i=0;
        for(;i<Cnt && BytesInRxB();i++)
        {
            _disable();
            ((U8*)Status)[i]=RxB.Status[RxB.RdP];
            ((U8*)Buf)[i]=RxB.Data[RxB.RdP++];
            RxB.RdP&=COMBUF_WRAPMASK;
            _enable();
        }
        Cnt=i;
    }
    else for(int i=0; i<Cnt; i++) ((U8*)Buf)[i]=readChar();
    return Cnt;
}
#endif

void _fast COMPORT::writeChar(U8 Char){
  int nextidx;
  do{
    _disable();
    if((nextidx=(TxB.WrP+1)&COMBUF_WRAPMASK) != TxB.RdP){
      TxB.Data[TxB.WrP]=Char;
      TxB.WrP=nextidx;
      _enable();
      break;
    }
    else{
      TxEvent().reset();
      _enable();
      TxEvent().waitFor();
    }
  } while(1);
  enableTx();
}

U16 COMPORT::write(const void *Buf,U16 Cnt){
  U8* Bytes = (U8*)Buf;
  while(1)
  {
    _disable();
    U16 nBF = TxB.BytesFree();
    U16 n = (nBF<Cnt) ? nBF : Cnt;
    Cnt -= n;
    while(n-- > 0){
      TxB.Data[TxB.WrP] = *(Bytes++);
      ++TxB.WrP &= COMBUF_WRAPMASK;
    }
    _enable();
    enableTx();
    if(Cnt>0)
    {
      TxEvent().waitFor();
      TxEvent().reset();
    }
    else
      break;
  }
//  for(;Cnt>0;Cnt--) writeChar(*(Bytes++));
  return Cnt;
}

void COMPORT::print(const char* Msg){
  write(Msg,strlen(Msg));
}

void COMPORT::printHex(void const * Buf,int Cnt){
  for(int i=0; i<Cnt; i++){
    U8 Byte=((U8*)Buf)[i];
    writeChar(hex_to_ascii[Byte>>4]);
    writeChar(hex_to_ascii[Byte&0xF]);
    writeChar(' ');
  }
}

void COMPORT::printf(const char* Fmt, ...){
  if(!StrBuf) StrBuf = (char*)SYS::malloc(1996);
#ifdef __BORLANDC__
  vsprintf(StrBuf,Fmt,...);
  print(StrBuf);
#else
  va_list args;
  va_start(args,Fmt);
  vsprintf(StrBuf,Fmt,args);
  va_end(args);
  print(StrBuf);
#endif
}

void COMPORT::sendCmdTo7000(U8 *Cmd,BOOL Checksum){
  U8 Buf[32];
  int i=0;
  if(Checksum){
    U8 Sum=0;
    while(Cmd[i]) {
      Buf[i] = Cmd[i];
      Sum += Cmd[i++];
    }
    Buf[i++] = hex_to_ascii[Sum>>4];
    Buf[i++] = hex_to_ascii[Sum&0xF];
  }
  else
    while(Cmd[i]) Buf[i] = Cmd[i++];
  Buf[i++] = '\r';
  write(Buf,i);
  setExpectation(0xFF,'\r');
}

int COMPORT::receiveLine(U8 *Buf,int Timeout,BOOL Checksum, U8 endChar){
  if(Timeout && !RxEvent().waitFor(Timeout)) return -1;
  int Pos=0, Res=0, Sum=0;
  while(1){
    if(!BytesInRxB()){
      Res=-1; break;
    }
    else {
      U8 c = readChar();
      if(c==endChar)
          break;
      Buf[Pos]=c;
      Sum+=c;
      if(++Pos==128){ Res=-2; break;}
    }
  }
  Buf[Pos]=0;
  if(Res==0 && Checksum){
    if(Pos<2) Res=-3;
    else{
      Pos-=2;
      Sum -= Buf[Pos]+Buf[Pos+1];
      U8 CS = (U8)FromHexStr(&(Buf[Pos]),2);
      if((Sum & 0xFF) != CS){
        Res=-4;
//        ConPrintf("Buf=%s Sum=%02X CS=%02X\n\r",Buf,Sum & 0xFF,CS);
      }
    }
  }
  Buf[Pos]=0;
  return Res;
}

void COMPORT::reset(){
  uninstall();
  install(Baud);
}

void COMPORT::waitTransmitOver(){
  int t=1000;
  while(!IsTxOver() && t--) SYS::sleep(0);
}

//class PRT_ABSTRACT
void PRT_ABSTRACT::TxStr(const char *psz)
{
  Tx( psz, strlen(psz) );
}

#if defined(__ARQ)

//class PRT_COMPORT
PRT_COMPORT::PRT_COMPORT(COMPORT *com){
  this->com=com;
  RxState=RX_FLAG;
  com->setExpectation(0xFF,'V');
}

int PRT_COMPORT::Rx(void *Data){
  if(RxState==RX_READY){
    RxState=RX_FLAG;
    return com->read(Data,DataSize,0);
  }
  else return 0;
}

void PRT_COMPORT::Tx(const void *Data, U16 Cnt){
  _HEADER hdr;
  hdr.DataSize=(U8)Cnt;
//  hdr.CRC=(U8)CRC16_get(&hdr,sizeof(hdr)-1);
  hdr.CRC=(U8)CRC16_get( &hdr, sizeof(hdr)-1, CRC16_ModBus );
  com->writeChar('W');
  com->write(&hdr,sizeof(hdr));
  com->write(Data,Cnt);
  com->writeChar('V');
  dump(&hdr,sizeof(hdr));
}

U16 PRT_COMPORT::ProcessIO(){
  U16 Result=0;
  // RX
  BOOL Process=TRUE;
  while(com->BytesInRxB() && Process){
    switch(RxState){
    case RX_FLAG:
      while(com->BytesInRxB()){
        if(com->readChar()=='W'){
          RxState=RX_HDR;
          break;
        }
      }
      break;
    case RX_HDR:
      _HEADER hdr;
      if(com->BytesInRxB() >= sizeof(hdr)){
        com->read(&(hdr),sizeof(hdr),0);
        //if((U8)(CRC16_get(&hdr,sizeof(hdr)-1))==hdr.CRC){
        if((U8)(CRC16_get(&hdr,sizeof(hdr)-1,CRC16_ModBus))==hdr.CRC){
          DataSize=hdr.DataSize;
          RxState=RX_DATA;
        }
        else {
//          com->unread(&hdr,sizeof(hdr));
          RxState=RX_FLAG;
        }
      }
      else Process=FALSE;
      break;
    case RX_DATA:
      if(com->BytesInRxB() >= DataSize+1){
        RxState=RX_READY;
        Result|=IO_RX;
      }
      Process=FALSE;
      break;
    }
  }
  if(!Process && RxState!=RX_READY) com->RxEvent().reset();
  // TX
  U16 bct=com->BytesCanTx();
  if(bct >= LL_PRTDataSize+256 ){
    Result|=IO_TX;
    if(bct==COMBUF_SIZE) Result|=IO_TXEMPTY;
  }
  return Result;
}


//class PRT_ARQ
PRT_ARQ::PRT_ARQ(PRT_ABSTRACT *prt){
  this->prt=prt;
  RxWin=new ARQRXWINITEM[ARQ_WINSIZE];
  TxWin=new ARQTXWINITEM[ARQ_WINSIZE];
  TimeClient=NULL;
  TxBuf.Addr=0;
  ARQState=ARQS_RSTQ;
  reset();
}

void PRT_ARQ::reset(){
  RxRd=RxWr=TxRd=TxWr=LastACKNum=0;
  LastSendTime=0;
  SYS::getSysTime(LastRecvTime);
  for(int i=0; i<ARQ_WINSIZE; i++) RxWin[i].DataSize=0;
}

int PRT_ARQ::Rx(void *Data){
  U16 i=RxRd & ARQ_WINWRAP;
  int Result=RxWin[i].DataSize;
  memcpy(Data,RxWin[i].Data,Result);
  RxWin[i].DataSize=0;
//  dbg3("\n\rPRT_ARQ::Rx : ¹=%02X DataSize=%d",RxRd,Result);
  ++RxRd&=ARQ_NUMWRAP;
//  dbg3("\n\rRxRd: RxRd=%02X RxWr=%02X ",RxRd,RxWr);
  return Result;
}

void PRT_ARQ::Tx(const void *Data, U16 DataSize){
  U16 i=TxWr & ARQ_WINWRAP;
  TxWin[i].TxTime=0;
  TxWin[i].DataSize=(U8)DataSize;
  memcpy(TxWin[i].Data,Data,DataSize);
//  dbg3("\n\rPRT_ARQ::Tx : ¹=%02X DataSize=%d",TxWr,DataSize);
  ++TxWr&=ARQ_NUMWRAP;
}

#define CODE_RR   0x0
#define CODE_RNR  0x2
#define CODE_SREJ 0x3
#define CODE_TIMQ 0x4
#define CODE_TIMA 0x5
#define CODE_RSTQ 0x6
#define CODE_RSTA 0x7

U16 PRT_ARQ::ProcessIO(){
  U16 EM = prt->ProcessIO();
  PACKET Buf;
  U16 Size;
  // RX
  if(EM & IO_RX){
    Size = prt->Rx(&Buf);
    // Frame corrupted?
    U16 CRC=CRC16_get((U8*)&Buf+2,Size-2, CRC16_ModBus);
//    ConPrintHex(&Buf,Size);
//    dbg3("Size=%d CRC=%04X ",Size,CRC);
    if(Buf.CRC!=CRC)
    { // Frame corrupted!
      dbg("\n\rframe corrupted");
      // Header is valid and frame is a i-frame?
      if(Buf.HdrCRC==(U8)CRC16_get(&Buf.Hdr,sizeof(Buf.Hdr), CRC16_ModBus) &&
        Buf.Hdr.A._.Type==0)
      { // corrupted i-frame received, prepare SREJ-frame
        TxBuf.Addr=MyAddr;
        TxBuf.A._.Type=1;
        TxBuf.A.S.Code=CODE_SREJ;
        TxBuf.B.NRx=Buf.Hdr.A.I.NTx;
        TxBuf.B.QA=1;
        dbg2(" Generate SREJ ¹%02X",TxBuf.B.NRx);
      }
    }
    else
    { // Frame valid
      SYS::getSysTime(LastRecvTime);
      if(Buf.Hdr.A._.Type==0)
      { // i-frame
        if(ARQState==ARQS_NORMAL)
        { // ARQ in "normal operation" state
          acknowledge(Buf.Hdr.B.NRx);
          int i=Buf.Hdr.A.I.NTx;
          if( ((i-RxRd) & ARQ_NUMWRAP) < ARQ_WINSIZE ){
            if( ((i-RxWr) & ARQ_NUMWRAP) <= ((i-RxRd) & ARQ_NUMWRAP) )
              RxWr=(i+1) & ARQ_NUMWRAP;
            i&=ARQ_WINWRAP;
            RxWin[i].DataSize=(U8)(Size-ARQ_PRTDataSize);
            memcpy(RxWin[i].Data,Buf.Data,sizeof(Buf.Data));
          }
        }
      }
      else
      { // s-frame
        switch(Buf.Hdr.A.S.Code){
        case CODE_SREJ:
          dbg2("\n\rSREJ %02X received",Buf.Hdr.B.NRx);
          // prepare requested i-frame
          TxBuf.Addr=MyAddr;
          TxBuf.A._.Type=0;
          TxBuf.A.I.NTx=Buf.Hdr.B.NRx;
          break;
        case CODE_RR:
//          dbg2(" Rec_RR %02X",Buf.Hdr.B.NRx);
          acknowledge(Buf.Hdr.B.NRx);
          break;
        case CODE_TIMA:
          if(TimeClient)
            TimeClient->receiveData(1,&(Buf.Data[0]),Size-ARQ_PRTDataSize);
          break;
        case CODE_RSTQ:
          if(ARQState==ARQS_NORMAL) reset();
          TxBuf.Addr=MyAddr;
          TxBuf.A._.Type=1;
          TxBuf.A.S.Code=CODE_RSTA;
          TxBuf.B.QA=0;
          TxBuf.B.NRx=0;
          ConPrint("recv(ARQS_RSTQ)\n\r");
          break;
        case CODE_RSTA:
          ConPrint("recv(ARQS_RSTA)\n\r");
          if(ARQState==ARQS_RSTQ) reset();
          ARQState=ARQS_NORMAL;
          break;
        default:;
          //now i don't know what to do
        }
      }
    }
  }
  // TX
  if(EM & IO_TX){
    Size=ARQ_PRTDataSize;
    TIME Now;
    SYS::getSysTime(Now);
    // We have no frame in TxBuf ?
    if(TxBuf.Addr==0){
      if(TimeClient && TimeClient->HaveDataToTransmit(1)){
        if(EM & IO_TXEMPTY){
          TxBuf.Addr=MyAddr;
          TxBuf.A._.Type=1;
          TxBuf.A.S.Code=CODE_TIMQ;
          TxBuf.B.QA=0;
          TxBuf.B.NRx=0;
          Size+=TimeClient->getDataToTransmit(1,&(Buf.Data[0]),MaxARQDataSize);
        }
      }
      else switch(ARQState){
        case ARQS_RSTQ:
          if((LastSendTime==TIME(0)) || ((LastSendTime+PACKET_TIMEOUT) < Now)){
            // generate RSTQ s-frame
            TxBuf.Addr=MyAddr;
            TxBuf.A._.Type=1;
            TxBuf.A.S.Code=CODE_RSTQ;
            TxBuf.B.QA=0;
            TxBuf.B.NRx=0;
            LastSendTime=Now;
            ConPrint("send(ARQS_RSTQ)\n\r");
          }
          break;
        case ARQS_NORMAL:
          // check i-frame in queue or ..
          if(!get_i_frame() && (
            (LastSendTime+(PACKET_TIMEOUT/2) < Now) ||
            ((LastACKNum-RxRd) & ARQ_NUMWRAP > ARQ_WINSIZE) &&
            ((RxRd-LastACKNum) & ARQ_NUMWRAP) > (ARQ_WINSIZE>>1)
          )){ // .. or generate RR s-frame
            TxBuf.Addr=MyAddr;
            TxBuf.A._.Type=1;
            TxBuf.A.S.Code=CODE_RR;
            TxBuf.B.NRx=GetNRx();
            TxBuf.B.QA=0;
//            dbg2(" Gen_RR %02X",TxBuf.B.NRx);
            break;
          }
        default:;
          // do nothing
      }
    }
    // we have anything to send?
    if(TxBuf.Addr!=0){
      if(TxBuf.A._.Type==0)
      { // for i-frame do some actions
        TxBuf.B.NRx=GetNRx();
        TxBuf.B.QA=0;
        int i=TxBuf.A.I.NTx & ARQ_WINWRAP;
        int DSize=TxWin[i].DataSize;
        memcpy(Buf.Data,TxWin[i].Data,DSize);
        Size+=DSize;
        TxWin[i].TxTime=Now;
      }
      memcpy(&Buf.Hdr,&TxBuf,sizeof(TxBuf));
      sendPacket(Buf,Size);
      TxBuf.Addr=0;
    }
  }
  return
    ( (RxWin[RxRd&ARQ_WINWRAP].DataSize>0) ? IO_RX : 0 ) |
    ( (((TxWr-TxRd)&ARQ_NUMWRAP) < ARQ_WINSIZE) ? IO_TX : 0 );
}

void PRT_ARQ::acknowledge(U16 R){
  if( TxRd!=R && ((R-TxRd) & ARQ_NUMWRAP) <= ((TxWr-TxRd) & ARQ_NUMWRAP) ){
    TxRd=(U8)R;
/*
    ConPrintf("\n\rACK: TxRd=%02X TxWr=%02X   RxRd=%02X RxWr=%02X",
      TxRd,TxWr,RxRd,RxWr);
//*/
  }
  Acknowledged=TRUE;
}

BOOL PRT_ARQ::get_i_frame(){
  TIME Now;
  SYS::getSysTime(Now);
  int i=TxRd;
  while(i!=TxWr){
    TIME &Tmp=TxWin[i & ARQ_WINWRAP].TxTime;
    //SYS::checkTime(Tmp);
    if(Tmp+PACKET_TIMEOUT < Now) break;
    ++i&=ARQ_NUMWRAP;
  }
  if(i!=TxWr){
    TxBuf.Addr=MyAddr;
    TxBuf.A._.Type=0;
    TxBuf.A.I.NTx=i;
    return TRUE;
  }
  else return FALSE;
}

U8 PRT_ARQ::GetNRx(){
  int i=RxRd;
  while(RxWin[i & ARQ_WINWRAP].DataSize!=0 && i!=RxWr) ++i&=ARQ_NUMWRAP;
  SYS::getSysTime(LastSendTime);
  LastACKNum=i;
//  dbg2(" GetNRx=%02X",i);
  return i;
}

void PRT_ARQ::sendPacket(PACKET &P,U16 Size){
  P.HdrCRC=(U8)CRC16_get(&P.Hdr,sizeof(P.Hdr), CRC16_ModBus);
  P.CRC=CRC16_get((U8*)&P+2,Size-2, CRC16_ModBus);
  prt->Tx(&P,Size);
}

BOOL PRT_ARQ::LinkTimeout(){
  TIME Now;
  SYS::getSysTime(Now);
  if(LastRecvTime+PACKET_TIMEOUT*4 < Now){
    LastRecvTime = Now;
    return TRUE;
  }
  else return FALSE;
}


PRT_ARQ::~PRT_ARQ(){
  delete RxWin;
  delete TxWin;
}

#endif
//*/
