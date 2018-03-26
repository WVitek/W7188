
enum PARITY_KIND { pkNone=0, pkEven=1, pkOdd=2, pkMark=3, pkSpace=4 };

class COMPORT {
protected:
  int port;
  EVENT rxEvent, txEvent;
  char* StrBuf;
public:
  U16  virtual read(void *Buf,U16 Cnt,int Timeout);
  U16  virtual write(const void *Buf,U16 Cnt);
public:
  U8   Flags;
  COMPORT(int port);
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
  void         clearRxBuf();
  U16          BytesInRxB();
  U16          BytesCanTx();
  EVENT inline &RxEvent() {return rxEvent;}
  EVENT inline &TxEvent() {return txEvent;}
  void         setExpectation(U8 Mask,U8 Char,U16 Count=0);
  U8   _fast   readChar();
  void _fast   writeChar(U8 Char);
  void         print(const char* Msg);
  void         printHex(void const * Buf,int Cnt);
  void         printf(const char* Fmt, ...);
//  void         reset();
  void         sendCmdTo7000(U8 *Cmd,BOOL Checksum,BOOL ClearRxBuf=FALSE);
  int          receiveLine(U8 *Buf,int Timeout,BOOL Checksum, U8 endChar='\r');
  void         waitTransmitOver();
  virtual void setDtr(bool high)=0;
  virtual void setRts(bool high)=0;
};
