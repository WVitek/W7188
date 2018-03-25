
#define COMBUF_SIZE 512
#define COMBUF_WRAPMASK (COMBUF_SIZE-1)
#define COMBUF_RXLIMIT (COMBUF_SIZE-COMBUF_SIZE/16)
#define COMBUF_TXLIMIT (COMBUF_SIZE/16)

class __BUFFER {
public:
  S16  WrP;
  S16  RdP;
  EVENT Event;
  U8  ExpMask;
  U8  ExpChar;
  U8  Data[COMBUF_SIZE];
#ifdef __COM_STATUS_BUF
  U8  Status[COMBUF_SIZE];
#endif
  inline U16 BytesOccupied(){
    return (WrP-RdP)&COMBUF_WRAPMASK;
  }
  inline U16 BytesFree(){
    return COMBUF_SIZE-BytesOccupied();
  }
  inline BOOL TriggerRX(){
    return BytesOccupied() > COMBUF_RXLIMIT;
  }
  inline BOOL TriggerTX(){
    return BytesOccupied() < COMBUF_TXLIMIT;
  }
  inline void clear(){
      SYS::cli(); WrP=RdP=0; SYS::sti();
  }
};

#define CF_FULLDUPLEX 0x00
#define CF_HALFDUPLEX 0x80
#define CF_AUTODIR    0x40
#define CF_HWFLOWCTRL 0x20

#define OFS_TxB  0x040C
#define OFS_RxB  0x0000
#define OFS_WrP  0x0
#define OFS_RdP  0x2
#define OFS_Data 0x4

#ifdef __UsePerfCounters
struct STRUCT_COMPERF {
  U16 ISRMs;
  U16 RCalls,RBytes;
  U16 TCalls,TBytes;
};
#endif

enum PARITY_KIND { pkNone, pkEven, pkOdd, pkMark, pkSpace };

class COMPORT /*: public STREAM*/ {
protected:
  __BUFFER RxB;
  __BUFFER TxB;
  U16 RxCnt;
  U32 OldIntVect;
  U32 Baud;
#ifdef __UsePerfCounters
  U32 ISRTicks;
  U16 _RC,_RB;
  U16 _TC,_TB;
#endif
  char* StrBuf;
  virtual void enableTx()=0;
public:
  U16  virtual read(void *Buf,U16 Cnt,int Timeout);
#ifdef __COM_STATUS_BUF
  U16  virtual read(void *Buf,void *Status, U16 Cnt,int Timeout);
#endif
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

