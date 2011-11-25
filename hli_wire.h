#pragma once
#include "PrtLiner.h"

#define MaxTxSize 400
#define LinkTimeout (toTypeSec | 10)

class THREAD_HLI : public THREAD {
  int comNum;
public:

THREAD_HLI(U16 comNum):THREAD(4096+1024){
  this->comNum = comNum;
}

void execute(){
  COMPORT& HLI=GetCom(comNum);
//#if !defined(__HLI_BaudRate)
//  #define __HLI_BaudRate 19200
//#endif
  HLI.install(__HLI_BaudRate);
  PRT_LINER prt(&HLI);
  dbg3("\n\rHLI_WIRE started (COM%d, %ld)", comNum, (U32)__HLI_BaudRate);
  prt.Open();
  TIMEOUTOBJ toutLink, toutTx;
  U8 TxCnt=0, RxCnt=0;
  U8 TxBuf[MaxTxSize];
  while(!Terminated){
    U16 R = prt.ProcessIO();
#if defined(__HLIControlCD)
    if((R & IO_UP==0) || !HLI.CarrierDetected())
#else
    if(R & IO_UP==0)
#endif
    {
      SYS::sleep(73);
      continue;
    }
    BOOL NothingToDo = TRUE;
    if (R & IO_RX)
    {
      int RxSize;
      const void *Rx = prt.GetBuf( RxSize );
      if( RxSize && HLI_receive(Rx,RxSize) )
      {
        toutLink.start(LinkTimeout); // restart link timeout
        RxCnt++;
      }
      prt.Rx(NULL);
      NothingToDo = FALSE;
    }
    if( R & IO_TX )
    {
      int TxSize = HLI_totransmit(TxBuf,MaxTxSize);
      if( TxSize>0 )
      {
        prt.Tx(TxBuf,TxSize);
        toutTx.start(toTypeSec | 10);
        NothingToDo = FALSE;
        TxCnt++;
      }
      else prt.Tx(TxBuf,0);
    }
#ifndef __SHOWADCDATA
    SYS::dbgLed( (U32(RxCnt)<<12) | TxCnt );
#endif
    if( NothingToDo )
    {
      HLI.RxEvent().reset();
      HLI.RxEvent().waitFor(53);
    }
  }
  prt.Close();
  HLI.uninstall();
  S(0x00);
  dbg("\n\rHLI stopped");
}

};

