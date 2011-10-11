#pragma once
// Enfora GSM2238

#if defined(__BASHNEFT)
char* szMdmCmdConn="ate0;+ifc=0,0;$padblk=500;$padcmd=3;+cgdcont=1,\"IP\",\"bashneft.ufa\";$paddst=\"%s\",19864;$padsrc=19864;$hostif=1;d*99***1#";
#else
char* szMdmCmdConn="ate0;+ifc=0,0;$padblk=500;$padcmd=3;+cgdcont=1,\"IP\",\"internet.mts.ru\";$paddst=\"%s\",19864;$padsrc=19864;$hostif=1;d*99***1#";
#endif

#include "PrtLiner.h"

#define MaxTxSize 400

#define RxIs(Resp) __rxequal(Rx,Resp,RxSize)
#define LinkTimeout (toTypeSec | 120)

class THREAD_HLI : public THREAD {
  COMPORT *HLI;
public:

THREAD_HLI(COMPORT &HLI):THREAD(4096+2048){
  this->HLI=&HLI;
}

BOOL setComRate(BOOL factory)
{
  U32 rate = factory? 115200ul : 19200ul;
  HLI->uninstall();
  HLI->install(rate);
  ConPrintf("\n\rHLI baudrate %ld\n\r",rate);
  return factory;
}

void execute(){
  COMPORT& HLI=*(this->HLI);
  PRT_LINER prt(&HLI);
  prt.Open();
  dbg("\n\rHLI started (Enfora GSM2238-UDP)");
  U8 RxCnt=0, TxCnt=0;
  TIMEOUTOBJ toutState, toutSend, toutTx;
  enum {msDetect,msInit,msSetRate,msConn,msOnline,msRset}
    State = msDetect, NewState = msRset;
  int RxSize, TxSize=0;
  U8 TxBuf[MaxTxSize];
  BOOL IsFactoryRate = setComRate(TRUE);
  
  while(!Terminated){
    U16 R = prt.ProcessIO();
    if(R & IO_UP==0)
    {
      SYS::sleep(20);
      continue;
    }
    const void *Rx;
    RxSize = 0;
    if (R & IO_RX)
    {
      Rx = prt.GetBuf( RxSize );
      if(RxSize)
      {
        if(State!=msOnline) 
          ConPrintf(" <Rx=%d:%.*s> ",RxSize,RxSize,Rx);
        else
          ConPrintf(" [Rx=%d] ",RxSize);
      }
    }
    else
      Rx = NULL;
    BOOL StateChanged = State != NewState;
    State = NewState;
    switch( State )
    {

    case msDetect:
      if( StateChanged || toutState.IsSignaled() )
      {
        ConPrint("\n\rGSM2238: detecting...\n\r");
        if(R & IO_TX){
          if(!StateChanged)
            IsFactoryRate = setComRate(!IsFactoryRate);
          SYS::sleep(1500);
          HLI.print("+++");
          SYS::sleep(1500);
          prt.TxCmd("");
          prt.TxCmd("at+csq");
        }
        toutState.start(toTypeSec | 7);
      }
      if(RxIs("OK"))
        NewState = msInit;
      break;

    case msInit:
      if( StateChanged )
      {
        ConPrint("\n\rGSM2238: set factory defaults\n\r");
        prt.TxCmd("at&f");
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR"))
        NewState = msRset;
      else if(RxIs("OK")){
        NewState = msSetRate;
        IsFactoryRate = setComRate(true);
      }
      break;
//*
    case msSetRate:
      if(StateChanged)
      {
        SYS::sleep(500);
        ConPrint("\n\rGSM2238: +IPR=19200\n\r");
        prt.TxCmd("at+ipr=19200");
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR"))
        NewState = msRset;
      else if(RxIs("OK")){
        NewState = msConn;
        IsFactoryRate = setComRate(FALSE);
      }
      break;
//*/
    case msConn:
      if(StateChanged)
      {
        SYS::sleep(1000);
        ConPrintf("\n\rGSM2238: connect to '%s'\n\r", _HLI_hostname);
        sprintf((char*)TxBuf,szMdmCmdConn, _HLI_hostname);
        prt.TxCmd((char*)TxBuf);
        toutState.start(toTypeSec | 30);
      }
      else if( toutState.IsSignaled() )
      {
        ConPrint("\n\rGSM2238: connecting timeout\n\r");
        NewState = msRset;
      }
      else if(RxIs("ERROR"))
      {
        ConPrint("\n\rGSM2238: connecting error\n\r");
//        NewState = msRset;
      }
      else if(RxIs("CONNECT"))
        NewState = msOnline;
      break;

    case msOnline:
      if( StateChanged )
      {
        ConPrint("\n\rGSM2238: online\n\r");
        toutState.start(LinkTimeout); // restart link timeout
        toutSend.setSignaled();
      }
      else if( toutState.IsSignaled() ) 
      {
        ConPrint("\n\rGSM2238: link timeout\n\r");
        NewState = msRset;
      }
      else if( RxIs("NO_CARRIER") )
      {
        ConPrint("\n\rGSM2238: carrier loss\n\r");
        NewState = msRset;
      }
      else if( RxIs("ERROR") )
      {
        ConPrint("\n\rGSM2238: unexpected error\n\r");
        NewState = msRset;
      }
      else
      {
        if( RxSize && HLI_receive(Rx,RxSize) )
        {
          toutState.start(LinkTimeout); // restart link timeout
          RxCnt++;
        }
        if( TxSize==0 && toutSend.IsSignaled() )
        {
          TxSize = HLI_totransmit(TxBuf,MaxTxSize);
          if(TxSize==0 && StateChanged) TxSize = HLI_totransmit(TxBuf,0);
        }
      }
      if( NewState==msOnline && TxSize>0 && (R & IO_TX) )
      {
        if(TxSize) ConPrintf(" [Tx=%d] ",TxSize);
        prt.Tx(TxBuf,TxSize);
        TxSize=0;
        toutSend.start(100);
        TxCnt++;
      }
      break;

    case msRset:
      NewState = msDetect;
      break;
      
    } // end of switch(State)
    
#ifndef __SHOWADCDATA
    SYS::dbgLed( (U32(NewState)<<16) | (U32(RxCnt)<<8) | TxCnt );
#endif
    if(Rx!=NULL)
      prt.Rx(NULL);
    else
    {
      HLI.RxEvent().reset();
      HLI.RxEvent().waitFor(50);
    }
  }
  prt.Close();
  HLI.uninstall();
  S(0x00);
  dbg("\n\rHLI stopped");
}

};

