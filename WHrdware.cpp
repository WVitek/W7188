#include "WHrdware.hpp"
#include "WComms.hpp"

#include "W80188SP.h"

class COM_NULL : public COMPORT {
protected:
  void enableTx(){}
public:
  U16  read(void* /*Buf*/,U16 /*Cnt*/,int /*Timeout*/){return 0;}
  U16  write(const void* /*Buf*/,U16 /*Cnt*/){return 0;}
public:
  COM_NULL(){}
  BOOL install(U32 /*Baud*/){return TRUE;}
  void uninstall(){}
  void setDataFormat(U8 nDataBits, PARITY_KIND parity, U8 nStopBits){}
  BOOL setSpeed(U32 /*Baud*/){return TRUE;}
  BOOL IsTxOver(){return TRUE;}
  BOOL CarrierDetected(){return FALSE;}
  BOOL virtual IsClearToSend(){return FALSE;}
  void setDtr(bool /*high*/){}
  void setRts(bool /*high*/){}
} ComZero;

#if !defined(__7188XB)

//#define Txbuf 0x00     // tx buffer
//#define Rxbuf 0x00     // rx buffer
#define Dll   0x00     // baud lsb
#define Dlh   0x01     // baud msb
#define Ier   0x01     // int enable reg
#define Fcr   0x02     // FIFO control register
#define Iir   0x02     // Interrupt Identification Register
#define Lcr   0x03     // line control reg
//#define Dfr   0x03     // Data format  reg
#define Mcr   0x04     // modem control reg
#define Lsr   0x05     // line status reg
#define Msr   0x06     // modem status reg
//#define Scr   0x07     // Scratch reg

#define NoError         0
#define PortError      -1
#define DataError      -2
#define ParityError    -3
#define StopError      -4
#define TimeOut        -5
#define QueueEmpty     -6
#define QueueOverflow  -7
#define BaudRateError  -13
#define CheckSumError  -14

#ifdef __SoftAutoDir
  #define set485DirTx(__CDB) outpw(ComDirPort,inpw(ComDirPort) | __CDB)
  #define set485DirRx(__CDB) outpw(ComDirPort,inpw(ComDirPort) & (~__CDB))
#endif

int GetFormatData(U8 nDataBits,PARITY_KIND parity,U8 nStopBits,U8 &Format)
{
  U8 format=0;
  switch(nDataBits) {
    case 5: break; // format+=0;
    case 6: format+=1; break;
    case 7: format+=2; break;
    case 8: format+=3; break;
    default: return(DataError);
  }

  switch(parity) {
    case pkNone : break;  // format+=0; No Parity
    case pkEven : format+=0x18; break; // Even parity
    case pkOdd  : format+=0x08; break; // Odd parity
    case pkMark : format+=0x28; break; // Mark parity
    case pkSpace: format+=0x38; break; // Space parity
    default: return(ParityError);
  }

  switch(nStopBits) {
    case 1: break; // format+=0;
    case 2: format+=4;    break;
    default: return(StopError);
  }
  Format=format;
  return NoError;
}

class COM_16550 : public COMPORT {
  int save[4];
  U16 Base;
#ifdef __SoftAutoDir
  U8 ComDirBit;
#endif
  U8 IntVectNum;
  U16 IntMask;
  U32 ISR;
  BOOL TxPaused;
  TIME timeOfHiCTS;
  TIME timeOfLoCTS;
  void enableTx(){
//    outp(Base+Lcr,inp(Base+Lcr)&0x7f);
    if(!TxPaused) outp(Base+Ier,0x0B); // enable UART interrupts: Rx, Tx, Modem Status change
  }
public:
  COM_16550(U16 Base,
#ifdef __SoftAutoDir
    U8 ComDirBit,
#endif
    U8 IntVectNum,U16 IntMask,
    U32 ISR,BOOL Flags):COMPORT()
  {
    this->Base=Base;
#ifdef __SoftAutoDir
    this->ComDirBit=ComDirBit;
#endif
    this->IntVectNum=IntVectNum;
    this->IntMask=IntMask;
    this->ISR=ISR;
    this->Flags=Flags;
  }
  BOOL install(U32 Baud);
  void uninstall();
  BOOL setSpeed(U32 Baud);

  BOOL TimeOfCTS(TIME &ToHi, TIME &ToLo)
  {
      _disable();
      ToHi = timeOfHiCTS;
      ToLo = timeOfLoCTS;
      _enable();
      return TRUE;
  }

  void setDataFormat(U8 nDataBits, PARITY_KIND parity, U8 nStopBits)
  {
      U8 format;
      GetFormatData(nDataBits,parity,nStopBits,format);
      SYS::cli();
      outp(Base+Lcr,format);
      SYS::sti();
  }

  BOOL IsTxOver(){
    SYS::cli();
    BOOL Res=(TxB.WrP==TxB.RdP) && (inp(Base+Lsr) & 0x40);
    SYS::sti();
    return Res;
  }
  BOOL CarrierDetected(){
      return (BOOL)(inp(Base+Msr)&0x80);
  }
  BOOL IsClearToSend(){
      return (BOOL)(inp(Base+Msr)&0x10);
  }
  void setDtr(bool high)
  {
      if(high) outp(Base+Mcr,inp(Base+Mcr)|1);
      else outp(Base+Mcr,inp(Base+Mcr)&~1);
  }
  void setRts(bool high)
  {
      if(high) outp(Base+Mcr,inp(Base+Mcr)|2);
      else outp(Base+Mcr,inp(Base+Mcr)&~2);
  }
private:
  friend void interrupt Com1_ISR(void);
  friend void interrupt Com2_ISR(void);
};

COM_16550 Com1(Com1Base,
#ifdef __SoftAutoDir
  Com1DirBit,
#endif
  Com1Int,Com1Msk,(U32)Com1_ISR,
#ifdef __HALFDUPLEX
  CF_HALFDUPLEX
#else
  CF_FULLDUPLEX // | CF_HWFLOWCTRL
#endif
);
COM_16550 Com2(Com2Base,
#ifdef __SoftAutoDir
  Com2DirBit,
#endif
  Com2Int,Com2Msk,(U32)Com2_ISR,
  CF_HALFDUPLEX|CF_AUTODIR
);

U16 GetBaudRateDivider(U32 baud){
#if defined(__7188) || defined(__I7188)
  switch(baud){
    case 1200ul  : return 96;
    case 2400ul  : return 48;
    case 4800ul  : return 24;
    case 9600ul  : return 12;
    case 19200ul : return  6;
    case 38400ul : return  3;
    case 57600ul : return  2;
    case 115200ul: return  1;
    default : return 0; // baud rate error
  }
#elif defined(__7188XA)
  switch(baud){
    case 1200ul  : return 768;
    case 2400ul  : return 384;
    case 4800ul  : return 192;
    case 9600ul  : return  96;
    case 19200ul : return  48;
    case 38400ul : return  24;
    case 57600ul : return  16;
    case 115200ul: return   8;
    default : return 0; // baud rate error
  }
#endif
}

BOOL COM_16550::install(U32 Baud){
  if(OldIntVect) uninstall();
  err=0;
  U8 format;
  if(GetFormatData(8,pkNone,1,format)!=NoError) return FALSE;
  SYS::cli();
  // save interrupt status
  save[3]=inp(Base+Ier);
  // save old format & speed
  save[2]=inp(Base+Lcr);
  // 1. set DLAB (baud rate)
  outp(Base+Lcr,0x80);
  //    save old baud rate
  save[0]=inp(Base+Dll);
  save[1]=inp(Base+Dlh);
  if(!setSpeed(Baud)){ SYS::sti(); return FALSE; }
  // 2. set data format
  outp(Base+Lcr,format);
  // 3. enable & clear FIFO
  outp(Base+Fcr,0xC1); // 01-1,41-4,81-8,C1-14 (Rx FIFO trigger level)
  outp(Base+Ier,0x09); // enable UART interrupts: Rx, Modem Status change

  //outp(Base+Mcr,0x0b); // set DTR line active
  // 5. init QUEUE
  RxB.WrP=RxB.RdP=TxB.WrP=TxB.RdP=0;
  RxB.ExpMask=0;
  RxB.ExpChar=1;
  // save old ISR
  OldIntVect=IntVect[IntVectNum];
  // install new ISR
  IntVect[IntVectNum]=ISR;
  // 6. unmask interrupt
  outpw(INT_IMASK,inpw(INT_IMASK)&(~IntMask));
#ifdef __SoftAutoDir
  // 7. 485 initial in receive direction
  set485DirRx(ComDirBit);
#endif
  TxPaused=FALSE;
  SYS::sti();
  setDtr(true);
  setRts(true);
  return TRUE;
}

BOOL COM_16550::setSpeed(U32 Baud){
  U16 Divider = GetBaudRateDivider(Baud);
  if (Divider==0) return FALSE;
  SYS::cli();
  // set DLAB (baud rate)
  outp(Base+Lcr,0x80);
  outp(Base+Dll,LoByte(Divider));
  outp(Base+Dlh,HiByte(Divider));
  SYS::sti();
  this->Baud=Baud;
  return TRUE;
}

void COM_16550::uninstall(void){
  if(!OldIntVect) return;
  _disable();
  outp(Base+Mcr,0x00); // set DTR line inactive?
  // 1. restore OLD ISR
  IntVect[IntVectNum]=OldIntVect;
  OldIntVect=0L;
  // 2. restore baud rate
  outp(Base+Lcr,0x80);
  outp(Base+Dll,save[0]);
  outp(Base+Dlh,save[1]);
  // 3. restore data format
  outp(Base+Lcr,save[2]&0x7f);
  // 4. restore enable COM's interrupt
  outp(Base+Ier,save[3]);
  // 6. mask interrupt
  outpw(INT_IMASK,inpw(INT_IMASK)|IntMask);
  _enable();
}

//***** Com1 *****

void interrupt Com1_ISR(void){
#define COM Com1
#define COMBASE Com1Base
#define COMDIRBIT Com1DirBit
#define INTTYPE Com1Int
#include "W16550IH.h"
#undef COM
#undef COMBASE
#undef INTTYPE
#undef COMDIRBIT
}

//***** Com2 *****
void interrupt Com2_ISR(void){
#define COM Com2
#define COMBASE Com2Base
#define COMDIRBIT Com2DirBit
#define INTTYPE Com2Int
#include "W16550IH.h"
#undef COM
#undef COMBASE
#undef INTTYPE
#undef COMDIRBIT
}

#endif // !defined(__7188XB)

COMPORT& GetCom(int ComNum){
  switch(ComNum){
#if defined(__7188XB)
  case 1: return ComA;
  case 2: return ComB;
  default: return ComZero;
#else
  case 1: return Com1;
  case 2: return Com2;
  case 3: return ComA;
  case 4: return ComB;
  default: return ComZero;
#endif
  }
}
