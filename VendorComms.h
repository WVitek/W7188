#ifndef __COMMS_HPP
#define __COMMS_HPP

#pragma once
#include "wkern.hpp"
#include "service.h"

enum PARITY_KIND { pkNone, pkEven, pkOdd, pkMark, pkSpace };

class COMPORT /*: public STREAM*/ {
public:
  U16  virtual read(void *Buf,U16 Cnt,int Timeout);
  U16  virtual write(const void *Buf,U16 Cnt);
public:
  U8   Flags;
  U8   err;
  COMPORT();
  ~COMPORT();
  BOOL virtual install(U32 Baud)=0;
  virtual void setDataFormat(U8 nDataBits, PARITY_KIND parity, U8 nStopBits)=0;
  virtual void uninstall()=0;
  BOOL virtual setSpeed(U32 Baud)=0;
  BOOL virtual IsTxOver()=0;
  BOOL virtual CarrierDetected()=0;
  BOOL virtual IsClearToSend()=0;
  BOOL virtual TimeOfCTS(TIME &ToHi, TIME &ToLo){ return FALSE; }
  //***
  void  inline clearRxBuf() {RxB.clear();}
  U16   inline BytesInRxB() {
    SYS::cli(); U16 R=RxB.BytesOccupied(); SYS::sti(); return R;
  }
  U16   inline BytesCanTx() {
    SYS::cli(); U16 R=TxB.BytesFree(); SYS::sti(); return R;
  }
  EVENT inline &RxEvent() {return RxB.Event;}
  EVENT inline &TxEvent() {return TxB.Event;}
  //EVENT inline &EventLoCTS() {return EvtLoCTS;}
  //EVENT inline &EventHiCTS() {return EvtHiCTS;}
  void         setExpectation(U8 Mask,U8 Char,U16 Count=0);
  U8   _fast   readChar();
  void _fast   writeChar(U8 Char);
  void         print(const char* Msg);
  void         printHex(void const * Buf,int Cnt);
  void         printf(const char* Fmt, ...);
  void         reset();
  void         sendCmdTo7000(U8 *Cmd,BOOL Checksum,BOOL ClearRxBuf=FALSE);
  int          receiveLine(U8 *Buf,int Timeout,BOOL Checksum, U8 endChar='\r');
  void         waitTransmitOver();
  virtual void setDtr(bool high)=0;
  virtual void setRts(bool high)=0;
#ifdef __UsePerfCounters
  void GetPerf(STRUCT_COMPERF *P);
#endif
};

extern COMPORT& GetCom(int ComNum);

#define IO_RX       0x0001
#define IO_TX       0x0002
#define IO_TXEMPTY  0x0004
#define IO_UP       0x0008

enum PRT_STATE {
  psInitial,
  psStarting,
  psClosed,
  psStopped,
  psClosing,
  pStopping,
  psReqSend,
  psAckRcvd,
  psAckSent,
  psOpened
};

class PRT_ABSTRACT { // Packet Receiver-Transmitter (Abstract)
protected:
  PRT_STATE State;
public:
  virtual void Open(){}
  virtual void Close(){}
  virtual int RxSize(){return -1;}
  virtual int Rx(void *Data)=0;
  virtual void Tx(const void *Data, U16 DataSize)=0;
  virtual U16 ProcessIO()=0;
public:
  void TxStr(const char *psz);
};

#if defined(__ARQ)

class PRT_COMPORT: public PRT_ABSTRACT {
  struct _HEADER {
    U8 DataSize;
    U8 CRC;
  };
  COMPORT *com;
  enum RX_STATE { RX_FLAG, RX_HDR, RX_DATA, RX_READY } RxState;
  U8 DataSize;
public:
  PRT_COMPORT(COMPORT *com);
  int Rx(void *Data);
  void Tx(const void *Data, U16 DataSize);
  U16 ProcessIO();
};

#define MaxARQDataSize 128

typedef U8 ARQDATA[MaxARQDataSize];

#define PACKET_TIMEOUT 2000
#define ARQ_WINSIZE 8
#define ARQ_WINWRAP (ARQ_WINSIZE-1)
#define ARQ_NUMWRAP 0x7F
class PRT_ARQ: public PRT_ABSTRACT {

  struct ARQTXWINITEM {
    TIME TxTime;
//    U8 TryCnt;
    U8 DataSize;
    ARQDATA Data;
  };

  struct ARQRXWINITEM {
    U8 DataSize;
    ARQDATA Data;
  };

  struct PACKET {
    U16 CRC;
    U8 HdrCRC;
    struct HEADER {
      U8 Addr;
      union {
        struct {
          U8 Data:7;
          U8 Type:1;
        } _;
        struct {  // Type = 0 - I-format
          U8 NTx:7;
          U8 Fill:1;
        } I;
        struct {  // Type = 1 - S-format
          U8 Code:7;
          U8 Fill:1;
        } S;
      } A;
      struct {
        U8 NRx:7;
        U8 QA:1;
      } B;
    } Hdr;
    ARQDATA Data;
  };

  PRT_ABSTRACT *prt;
  ARQRXWINITEM *RxWin;
  ARQTXWINITEM *TxWin;
  U8 RxRd,RxWr;
  U8 TxRd,TxWr;
  PACKET::HEADER TxBuf;
  TIME LastSendTime;
  TIME LastRecvTime;
  U8 LastACKNum;
  enum {ARQS_RSTQ,ARQS_RSTA,ARQS_NORMAL} ARQState;
protected:
  void sendPacket(PACKET &P,U16 Size);
  void acknowledge(U16 R);
  void reset();
  U8 GetNRx();
  BOOL get_i_frame();
public:
  SIMPLESERVICE *TimeClient;
  BOOL Acknowledged;
  PRT_ARQ(PRT_ABSTRACT *prt);
  int Rx(void *Data);
  void Tx(const void *Data, U16 DataSize);
  U16 ProcessIO();
  BOOL LinkTimeout();
  ~PRT_ARQ();
};

#define LL_PRTDataSize 2
#define ARQ_PRTDataSize (sizeof(PRT_ARQ::PACKET)-sizeof(ARQDATA))
#define PRTDataSize (LL_PRTDataSize+ARQ_PRTDataSize)

#endif

#endif // __COMMS_HPP
