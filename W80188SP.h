#pragma once

#define BASE_SPRT0 CTL_OFF+0x80
#define BASE_SPRT1 CTL_OFF+0x10

#define OFFS_SPRT_BDV     0x08  // serial port bauddiv reg
#define OFFS_SPRT_RX      0x06  // serial port receive reg
#define OFFS_SPRT_TX      0x04  // serial port transmit reg
#define OFFS_SPRT_STAT    0x02  // serial port status reg
#define OFFS_SPRT_CTL     0x00  // serial port control reg

/*
#define SPRT0_BDV          (BASE_SPRT0+OFFS_SPRT_BDV)
#define SPRT0_RX           (BASE_SPRT0+OFFS_SPRT_RX)
#define SPRT0_TX           (BASE_SPRT0+OFFS_SPRT_TX)
#define SPRT0_STAT         (BASE_SPRT0+OFFS_SPRT_STAT)
#define SPRT0_CTL          (BASE_SPRT0+OFFS_SPRT_CTL)

#define SPRT1_BDV          (BASE_SPRT1+OFFS_SPRT_BDV)
#define SPRT1_RX           (BASE_SPRT1+OFFS_SPRT_RX)
#define SPRT1_TX           (BASE_SPRT1+OFFS_SPRT_TX)
#define SPRT1_STAT         (BASE_SPRT1+OFFS_SPRT_STAT)
#define SPRT1_CTL          (BASE_SPRT1+OFFS_SPRT_CTL)
*/

#define SPRT_CTL_RSIE_ES      0x1000   //  enable Rx status interrupts
#define SPRT_CTL_BRK_ES       0x0800   //  send break
#define SPRT_CTL_TB8_ES       0x0400   //  Transmit data bit 8
#define SPRT_CTL_HS_ES        0x0200   //  hardware handshake enable
#define SPRT_CTL_TXIE_ES      0x0100   //  enable transmit interrupt
#define SPRT_CTL_RXIE_ES      0x0080   //  enable receive interrupt
#define SPRT_CTL_TMODE_ES     0x0040   //  enable transmitter
#define SPRT_CTL_RMODE_ES     0x0020   //  enable receiver
#define SPRT_CTL_EVN_ES       0x0010   //  even parity
#define SPRT_CTL_PE_ES        0x0008   //  enable parity checking
#define SPRT_CTL_MODE1_ES     0x0001   //  Async. mode A
#define SPRT_CTL_MODE2_ES     0x0002   //  Async. address recog. mode
#define SPRT_CTL_MODE3_ES     0x0003   //  Async. mode B
#define SPRT_CTL_MODE4_ES     0x0004   //  Async. mode C

// Asynchronous serial port status register (SPRT_STAT) ES only

#define SPRT_STAT_BRK1_ES     0x0400  //  Long break detected
#define SPRT_STAT_BRK0_ES     0x0200  //  Short break detected
#define SPRT_STAT_RB8_ES      0x0100  //  Receive data bit 9
#define SPRT_STAT_RDR_ES      0x0080  //  Receive data ready
#define SPRT_STAT_THRE_ES     0x0040  //  Tx holding reg. empty
#define SPRT_STAT_FRAME_ES    0x0020  //  Framing error detected
#define SPRT_STAT_OVERFLOW_ES 0x0010  //  Overrun error detected
#define SPRT_STAT_PARITY_ES   0x0008  //  Parity error detected
#define SPRT_STAT_TEMT_ES     0x0004  //  transmitter is empty
#define SPRT_STAT_HS0_ES      0x0002  //  *CTS signal asserted

//  all Sbreaks must set this bit
#define SPRT_STAT_BRK_ES    SPRT_STAT_BRK0_ES

#define ENABLE_RX_DX  SPRT_CTL_MODE1_ES | SPRT_CTL_TMODE_ES | SPRT_CTL_RMODE_ES | SPRT_CTL_RXIE_ES | SPRT_CTL_TXIE_ES
#define NoIntCtrlMask SPRT_CTL_MODE1_ES | SPRT_CTL_TMODE_ES | SPRT_CTL_RMODE_ES
#define WithIntCtrlmask ENABLE_RX_DX

class COM_80188 : public COMPORT {
  int Base;
  U8 IntVectNum;
  U16 DisIntMask;
  U32 ISR;
  int save[2];
  void enableTx(){
    outpw(Base+OFFS_SPRT_CTL,ENABLE_RX_DX);
  }
  void uninstall();
public:
  COM_80188(int Base,U8 IntVectNum,U16 DisIntMask,U32 ISR,U8 Flags):COMPORT(){
    this->Base=Base;
    this->IntVectNum=IntVectNum;
    this->DisIntMask=DisIntMask;
    this->ISR=ISR;
    this->Flags=Flags;
  }
  ~COM_80188(){
    if(OldIntVect) uninstall();
  }
  void setDataFormat(U8 nDataBits, PARITY_KIND parity, U8 nStopBits){}
  BOOL install(U32 Baud);
  BOOL setSpeed(U32 Baud);
  BOOL IsTxOver(){
    return (TxB.RdP==TxB.WrP) && (inpw(Base+OFFS_SPRT_STAT) & SPRT_STAT_TEMT_ES);
  }
  BOOL CarrierDetected(){return TRUE;}
  BOOL IsClearToSend(){return TRUE;}
  void setDtr(bool /*high*/){}
  void setRts(bool /*high*/){}
private:
  friend void interrupt SPI0_ISR(void);
  friend void interrupt SPI1_ISR(void);
};

BOOL COM_80188::install(U32 Baud){
  if(OldIntVect) uninstall();
  // install ISR
  _disable();
  OldIntVect=IntVect[IntVectNum];
  IntVect[IntVectNum]=ISR;
  outpw(INT_IMASK, inpw(INT_IMASK)&(~DisIntMask));  // enable COM INTERRUPT
  _enable();
  // initialization
  RxB.RdP=RxB.WrP=TxB.RdP=TxB.WrP=0;
  save[0]=inpw(Base+OFFS_SPRT_CTL);
  outpw(Base+OFFS_SPRT_CTL,WithIntCtrlmask); // N,8,1
  save[1]=inpw(Base+OFFS_SPRT_BDV);
  if(!setSpeed(Baud)) return FALSE;
  inpw(Base+OFFS_SPRT_RX);
  outpw(Base+OFFS_SPRT_STAT,0);
  return TRUE;
}

BOOL COM_80188::setSpeed(U32 Baud){
  int bdv=(int)(2500000L/Baud)+1;
  outpw(Base+OFFS_SPRT_BDV,bdv);
  this->Baud=Baud;
  return TRUE;
}

void COM_80188::uninstall(){
  if(!OldIntVect)return;
  _disable();
  IntVect[IntVectNum]=OldIntVect;
  OldIntVect=0;
//  outpw(INT_MASK, inpw(INT_MASK)|DisIntMask);  // disable COM INTERRUPT
//  outpw(Base+OFFS_SPRT_CTL,save[0]); // N,8,1
//  outpw(Base+OFFS_SPRT_BDV,save[1]);
  _enable();
}

#if defined(__7188XB)
#define COM_A_RTS_BIT 0x0010
#define COM_A_CTS_BIT 0x0020

class COM1_7188XB : public COM_80188
{
public:
  COM1_7188XB() : COM_80188(BASE_SPRT0,0x14,0x0400,(U32)SPI0_ISR) {}

  void setRts(bool high)
  {
    static bool needInit = true;
    if(needInit)
    {
      outpw(PIO_MODE_1, inpw(PIO_MODE_1) | COM_A_RTS_BIT); // output mode
      outpw(PIO_DIR_1, inpw(PIO_DIR_1) & ~COM_A_RTS_BIT);
      needInit=false;
    }
    U16 bits = inpw(PIO_DATA_1);
    if(high) bits &= ~COM_A_RTS_BIT;
    else bits |= COM_A_RTS_BIT;
    outpw(PIO_DATA_1, bits);
  }

  BOOL IsClearToSend()
  {
    static bool needInit = true;
    if(needInit)
    {
      outpw(PIO_MODE_1, inpw(PIO_MODE_1) & ~COM_A_CTS_BIT);
      outpw(PIO_DIR_1, inpw(PIO_DIR_1) & ~COM_A_CTS_BIT);
      needInit=false;
    }
    U16 bits = inpw(BASE_SPRT0 + OFFS_SPRT_STAT);
    return (bits & SPRT_STAT_HS0_ES) != 0;
  }
private:
  friend void interrupt SPI0_ISR(void);
  friend void interrupt SPI1_ISR(void);
} ComA;

#else

COM_80188 ComA(BASE_SPRT0,0x14,0x0400,(U32)SPI0_ISR,
  #if defined(__HALFDUPLEX_A)
    CF_HALFDUPLEX
  #else
    0
  #endif
  );

#endif

COM_80188 ComB(BASE_SPRT1,0x11,0x0200,(U32)SPI1_ISR,
  #if defined(__HALFDUPLEX_B)
    CF_HALFDUPLEX
  #else
    0
  #endif
  );

void interrupt far SPI0_ISR(void){
#define COM ComA
#define COMBASE BASE_SPRT0
#define INTTYPE INT_SP0INT
#include "W_EmbSPH.h"
#undef COM
#undef COMBASE
#undef INTTYPE
}

void interrupt far SPI1_ISR(void){
#define COM ComB
#define COMBASE BASE_SPRT1
#define INTTYPE INT_SP1INT
#include "W_EmbSPH.h"
#undef COM
#undef COMBASE
#undef INTTYPE
}

