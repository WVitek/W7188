#pragma once

char* szMdmCmdInit="at&f;+ipr=19200;e0;&d2;+cgdcont=1,\"IP\",\"internet.mts.ru\";*enad=1,\"name\",\"mts\",\"mts\",1,1;+cgqmin=1,0,0,0,0,0;+cgqreq=1,1,1,5,3,12;*e2ips=1,3,1,1020;*e2ipa=1,1";
char* szMdmCmdRset="at*e2ipa=0,1;*e2ipc";
char* szMdmCmdConn="at*e2ipo=0,\"%s\",19864";

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

void execute(){
  COMPORT& HLI=*(this->HLI);
  PRT_LINER prt(&HLI);
  HLI.install(9600);
  prt.Open();
  prt.TxCmd("at+ipr=19200;&w");
  SYS::sleep(200);
  HLI.install(19200);
  dbg("\n\rHLI started (GR47-GPRS-UDP)");
  U8 RxCnt=0, TxCnt=0;
  TIMEOUTOBJ toutState, toutSend, toutTx;
  enum {msDetect,msInit,msResolve,msConn,msOnline,msRset,msUnreachable} State = msUnreachable, NewState = msRset;
  int RxSize, TxSize=0;
  U8 TxBuf[MaxTxSize];
#if defined(_HLI_IPaddr)
  char szIPaddr[16]=_HLI_IPaddr;
#elif defined(_HLI_hostname)
  char szIPaddr[16];
#endif
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
        ConPrint("\n\rGR47: detecting...\n\r");
        if(R & IO_TX)
          prt.TxCmd("at+csq");
        toutState.start(toTypeSec | 10);
      }
      if(RxIs("OK"))
        NewState = msInit;
      break;

    case msInit:
      if( StateChanged )
      {
        ConPrint("\n\rGR47: initializing...\n\r");
        prt.TxCmd(szMdmCmdInit);
        toutState.start(toTypeSec | 15);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR"))
        NewState = msRset;
      else if(RxIs("OK"))
      #if defined(_HLI_IPaddr)
        NewState = msConn;
      #elif defined(_HLI_hostname)
        NewState = msResolve;
      #endif
      break;

  #ifdef _HLI_hostname
    case msResolve:
      if(StateChanged)
      {
        ConPrintf("\n\rGR47: Resolving host '%s'...\n\r",_HLI_hostname);
        sprintf((char*)TxBuf,"at*e2iprh=\"%s\"",_HLI_hostname);
        prt.TxCmd((char*)TxBuf);
        toutState.start(toTypeSec | 15);
      }
      else if( toutState.IsSignaled() )
      {
        ConPrint("\n\rGR47: RH timeout\n\r");
        NewState = msRset;
      }
      else if(RxIs("ERROR"))
      {
        ConPrint("\n\rGR47: RH error\n\r");
        NewState = msRset;
      }
      else if( RxSize>=16 && memcmp( Rx, "*E2IPRH: ", 9 ) == 0 )
      {
        size_t n = RxSize-9; if (n>15) n=15;
        memcpy( szIPaddr, (const char*)Rx + 9, n ); // copy IP address of "hostname" to szIPaddr
        szIPaddr[n]='\0';
//        ConPrintf("IP=%s\n\r",szIPaddr);
      }
      else if(RxIs("OK"))
        NewState = msConn;
      break;
  #endif
    
    case msConn:
      if(StateChanged)
      {
        ConPrintf("\n\rGR47: connecting to '%s'\n\r",szIPaddr);
        sprintf((char*)TxBuf,szMdmCmdConn,szIPaddr);
        prt.TxCmd((char*)TxBuf);
        toutState.start(toTypeSec | 15);
      }
      else if( toutState.IsSignaled() )
      {
        ConPrint("\n\rGR47: connecting timeout\n\r");
        NewState = msRset;
      }
      else if(RxIs("ERROR"))
      {
        ConPrint("\n\rGR47: connecting error\n\r");
//        NewState = msRset;
      }
      else if(RxIs("CONNECT"))
        NewState = msOnline;
      break;

    case msOnline:
      if( StateChanged )
      {
        ConPrint("\n\rGR47: online\n\r");
        toutState.start(LinkTimeout); // restart link timeout
        toutSend.setSignaled();
      }
      else if( toutState.IsSignaled() ) 
      {
        ConPrint("\n\rGR47: link timeout\n\r");
        HLI.setDtrLow();
        SYS::sleep(500);
        HLI.setDtrHigh();
        NewState = msRset;
      }
      else if( RxIs("NO_CARRIER") )
      {
        ConPrint("\n\rGR47: carrier loss\n\r");
        NewState = msRset;
      }
      else if( RxIs("ERROR") )
      {
        ConPrint("\n\rGR47: unexpected error\n\r");
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
      if(StateChanged)
      {
        ConPrint("\n\rGR47: resetting\n\r");
        prt.TxCmd(szMdmCmdRset);
        toutState.start(toTypeSec | 10);
      }
      else if( toutState.IsSignaled() || RxIs("OK") || RxIs("ERROR") )
      {
        if(toutState.IsSignaled())
        {
          HLI.setDtrLow();
          SYS::sleep(500);
          HLI.setDtrHigh();
        }
        NewState = msDetect;
      }
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

