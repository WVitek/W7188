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

COMPORT::COMPORT(int port){
  this->port = port;
  StrBuf=0;
}

COMPORT::~COMPORT(){
  if(StrBuf) SYS::free(StrBuf);
}

void COMPORT::setExpectation(U8 Mask,U8 Char,U16 Count){
// TODO (w#1#): implement comm task
//  SYS::cli();
//  rxEvent.reset();
//  RxB.ExpMask=Mask;
//  RxB.ExpChar=Char;
//  RxCnt=Count;
//  SYS::sti();
}

U16 COMPORT::BytesInRxB() { return DataSizeInCom(port); }
U16 COMPORT::BytesCanTx() { return GetTxBufferFreeSize(port); }
void COMPORT::clearRxBuf() { ClearCom(port); }

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
    ReadComn(port,(unsigned char*)Buf,n);
    nLeft -= n;
  }
  return Cnt - nLeft;
}

void _fast COMPORT::writeChar(U8 Char){
  ToCom(port, Char);
}

U16 COMPORT::write(const void *Buf,U16 Cnt){
  ToComBufn(port,(char*)Buf,(int)Cnt);
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

//void COMPORT::reset(){
//  uninstall();
//  install(Baud);
//}

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
  COM_NULL():COMPORT(-1){}
  BOOL install(U32 /*Baud*/){return TRUE;}
  void uninstall(){}
  void setDataFormat(U8 nDataBits, PARITY_KIND parity, U8 nStopBits){}
  BOOL setSpeed(U32 /*Baud*/){return TRUE;}
  BOOL IsTxOver(){return TRUE;}
  BOOL CarrierDetected(){return FALSE;}
  BOOL virtual IsClearToSend(){return FALSE;}
  void setDtr(bool /*high*/){}
  void setRts(bool /*high*/){}
} ComDummy;

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
protected:
  TIME timeOfHiCTS;
  TIME timeOfLoCTS;
public:
  COM_VENDOR(int port, U8 Flags):COMPORT(port) {
    this->Flags=Flags;
  }
  BOOL install(U32 Baud){
    return InstallCom(port,Baud,8,0,1) == 0;
  }
  void uninstall(){
    RestoreCom(port);
  }
  BOOL setSpeed(U32 Baud){
    return SetBaudrate(port,Baud) == 0;
  }

  BOOL TimeOfCTS(TIME &ToHi, TIME &ToLo)
  {
    // TODO (w#1#): add comm task
    ToHi = timeOfHiCTS;
    ToLo = timeOfLoCTS;
    return TRUE;
  }

  void setDataFormat(U8 nDataBits, PARITY_KIND parity, U8 nStopBits)
  {
    SetDataFormat(port,nDataBits,parity,nStopBits);
  }

  BOOL IsTxOver(){
    if(!IsTxBufEmpty(port))
      return false;
    WaitTransmitOver(port);
    return true;
  }
  BOOL CarrierDetected(){
      return GetCurMsr(port) & _MSR_DCD;
  }
  BOOL IsClearToSend(){
      return GetCurMsr(port) & _MSR_CTS;
  }
  void setDtr(bool high)
  {
    if(high)
      SetMCR_Bit(port, _MCR_DTR);
    else
      ClearMCR_Bit(port, _MCR_DTR);
  }
  void setRts(bool high)
  {
    if(high)
      SetMCR_Bit(port, _MCR_RTS);
    else
      ClearMCR_Bit(port, _MCR_RTS);
  }
};

//#ifdef __IP8000

COM_VENDOR com0(0,CF_FULLDUPLEX);
COM_VENDOR com1(1,CF_FULLDUPLEX);
COM_VENDOR com2(2,CF_HALFDUPLEX);
COM_VENDOR com3(3,CF_FULLDUPLEX);
COM_VENDOR com4(4,CF_FULLDUPLEX);

COMPORT& GetCom(int ComNum){
  switch(ComNum){
    case 0: return com0;
    case 1: return com1;
    case 2: return com2;
    case 3: return com3;
    case 4: return com4;
    default: return ComDummy;
  }
}

//#endif // __IP8000

