//file WComms.cpp

#include <stdio.h>
#include <string.h>
#include "Comms.hpp"
#include "WKern.hpp"
#include "Hardware.hpp"
#include "WCRCs.hpp"
#ifndef __BORLANDC__
#include <stdarg.h>
#endif

//class COMPORT

COMPORT::COMPORT(){
  StrBuf=0;
}

COMPORT::~COMPORT(){
  if(StrBuf) SYS::free(StrBuf);
}

void COMPORT::setExpectation(U8 Mask,U8 Char,U16 Count){
// TODO (w#1#): implement comm task
  SYS::cli();
  rxEvent.reset();
  RxB.ExpMask=Mask;
  RxB.ExpChar=Char;
  RxCnt=Count;
  SYS::sti();
}

U8 _fast COMPORT::readChar(){
  while(!IsCom(port)) SYS::sleep(1);
  return ReadCom(port);
}

U16 COMPORT::read(void* Buf,U16 Cnt,int Timeout){
  if(Timeout == 0) Timeout = 32767;
  int nLeft = Cnt;
  while(Timeout>0 && nLeft>0){
    int n = DataSizeInCom(port);
    if(n==0){
        SYS::sleep(1);
        Timeout--;
        continue;
    }
    if(n>nLeft) n = nLeft;
    ReadComn(port,Buf,n);
    nLeft -= n;
  }
  return Cnt - nLeft;
}

void _fast COMPORT::writeChar(U8 Char){
  ToCom(port, Char);
}

U16 COMPORT::write(const void *Buf,U16 Cnt){
  ToComBufn(port,Buf,Cnt);
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
  vsnprintf(StrBuf,1996,Fmt,args);
  va_end(args);
  print(StrBuf);
#endif
}

void COMPORT::sendCmdTo7000(U8 *Cmd, BOOL Checksum, BOOL ClearRxBuf){
  //U8 Buf[32];
  int i=0;
  if(Checksum)
  {
    U8 Sum=0;
    while(Cmd[i])
    {
      //Buf[i] = Cmd[i];
      Sum += Cmd[i++];
    }
    write(Cmd,i);
    U8 Buf[3];
    Buf[0] = hex_to_ascii[Sum>>4];
    Buf[1] = hex_to_ascii[Sum&0xF];
    Buf[2] = '\r';
    write(Buf,3);
  }
  else
  {
    while(Cmd[i]) i++;//Buf[i] = Cmd[i++];
    write(Cmd,i);
    writeChar('\r');
  }
  //Buf[i++] = '\r';
  //write(Buf,i);
  if(ClearRxBuf) clearRxBuf();
  setExpectation(0xFF,'\r');
}

int COMPORT::receiveLine(U8 *Buf,int Timeout,BOOL Checksum, U8 endChar){
    int Pos=0, Res;
    if(Timeout==0) Timeout=32767;
    while(--Timeout>0 && !IsCom(port))
        SYS::sleep(1);
    if(Timeout == 0)
        Res=-1;
    //else
    {
        int Sum=0;
        while(1)
        {
            if(!IsCom(port)) { Res=-2; break; }
            U8 c = readChar();
            if(c==endChar) { Res=0; break; }
            Buf[Pos]=c;
            Sum+=c;
            if(++Pos==128) { Res=-3; break;}
        }
        Buf[Pos]=0;
        if(Res==0 && Checksum)
        {
            if(Pos<2) Res=-4;
            else
            {
                Pos-=2;
                Sum -= Buf[Pos]+Buf[Pos+1];
                U8 CS = (U8)FromHexStr(&(Buf[Pos]),2);
                if((Sum & 0xFF) != CS)
                {
                    Res=-5;
                    //ConPrintf("Buf=%s Sum=%02X CS=%02X\n\r",Buf,Sum & 0xFF,CS);
                }
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

#if !defined(__7188XB) && !defined(__IP8000)

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

class COM_VENDOR : public COMPORT {
  TIME timeOfHiCTS;
  TIME timeOfLoCTS;
public:
  COM_VENDOR(int port):COMPORT(port)
  {
    this->Base=Base;
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
  // 7. 485 initial in C:\I8000\W7188\VendorHardware.cppreceive direction
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


#endif // !defined(__7188XB)

COMPORT& GetCom(int ComNum){
  switch(ComNum){
#if defined(__7188XB) || defined(__IP8000)
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
