#pragma once

int iOperator = 0;

#if defined(_HLI_IPaddr)

char* szMdmCmdConnParam = _HLI_IPaddr;
//char* szMdmCmdInit="at&f;+ipr=19200;e0;&d2;+cdnscfg=\"81.30.199.5\",\"195.85.238.65\";+cipsprt=0;+cdnsorip=0;+cipmode=0";
char* szMdmCmdInit="at&f;+ipr=19200;e0;&d2;+cdnscfg=\"195.239.153.25\";+cipsprt=0;+cdnsorip=0;+cipmode=0";

#elif defined(_HLI_hostname)

char* szMdmCmdConnParam = _HLI_hostname;
// ns.dyndns.org = 204.13.248.75
// ns3.ufanet.ru = 81.30.199.5
// ns.mts.ru     = 194.54.148.129
char* szMdmCmdInit0="at&f;+ipr=19200;e0;&d2;+cdnscfg=\"204.13.248.75\",\"81.30.199.5\";+cipsprt=0;+cdnsorip=1;+cipmode=0";
//char* szMdmCmdInit0="at&f;+ipr=19200;e0;&d2;+cdnscfg=\"195.239.153.25\",\"195.85.238.65\";+cipsprt=0;+cdnsorip=1;+cipmode=0";
char* szMdmCmdInit1="at&f;+ipr=19200;e0;&d2;+cdnscfg=\"217.118.66.243\",\"217.118.66.244\";+cipsprt=0;+cdnsorip=1;+cipmode=0";
#define szMdmCmdInit ((iOperator==0)?szMdmCmdInit0:szMdmCmdInit1)

#endif

char* szMdmCmdRset="at+cipshut";
#if defined(__BASHNEFT)
char* szMdmCmdConn0="at+cstt=\"bashneft.ufa\",\"mts\",\"mts\";+ciicr;+cifsr;+cipstart=\"udp\",\"%s\",\"19864\"";
#else
char* szMdmCmdConn0="at+cstt=\"internet.mts.ru\",\"mts\",\"mts\";+ciicr;+cifsr;+cipstart=\"udp\",\"%s\",\"19864\"";
#endif
char* szMdmCmdConn1="at+cstt=\"internet.beeline.ru\",\"beeline\",\"beeline\";+ciicr;+cifsr;+cipstart=\"udp\",\"%s\",\"19864\"";
#define szMdmCmdConn ((iOperator==0)?szMdmCmdConn0:szMdmCmdConn1)
//char* szMdmCmdConn="at+cgqmin=1,0,0,0,0,0;+cgqreq=1,1,1,5,3,12;+cstt=\"internet.mts.ru\",\"mts\",\"mts\";+ciicr;+cifsr;+cipstart=\"udp\",\"%s\",\"19864\"";

#include "PrtLiner.h"

#define MaxTxSize 400

#define RxIs(Resp) __rxequal(Rx,Resp,RxSize)
#define RxBeg(Resp) __rxBeg(Rx,Resp,RxSize)
#define RxEnd(Resp) __rxEnd(Rx,Resp,RxSize)
#define LinkTimeout (toTypeSec | 120)

class THREAD_HLI : public THREAD {
  COMPORT *HLI;
public:

THREAD_HLI(COMPORT &HLI):THREAD(4096+2048){
  this->HLI=&HLI;
}

void execute(){
  COMPORT& HLI=*(this->HLI);
  HLI.install(19200);
  PRT_LINER prt(&HLI);
  prt.Open();
  dbg("\n\rHLI started (SIM300-GPRS-UDP)");
  U8 RxCnt=0, TxCnt=0;
  TIMEOUTOBJ toutState, toutSend, toutTx;
  enum {msDetect,msInit,msConn,msOnline,msRset,msUnreachable} State = msUnreachable, NewState = msDetect;
  int RxSize, TxSize=0;
  int nTout = 0;
  BOOL Sending;
  U8 TxBuf[MaxTxSize];
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
        if(RxIs("RING"))
          NewState = msRset;
        else if(RxIs("Call Ready"))
        {
          EVENT_DI Event;
          SYS::getNetTime(Event.Time);
          Event.Channel=255;
          Event.ChangedTo=1;
          puDI.EventDigitalInput(Event);
        }
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
        if(StateChanged)
           nTout = 0;
        else if(nTout++ >= 5)
        {
           nTout = 0;
           NewState = msRset;
           break;
        }
        ConPrint("\n\rSIM300: detecting...\n\r");
        if(R & IO_TX)
           prt.TxCmd("at+cops?");
        toutState.start(toTypeSec | 10);
      }
      if(RxBeg("+COPS"))
      {
        if(RxEnd("\"MTS\""))
        { iOperator = 0; ConPrint("\n\rSIM300: MTS\n\r"); }
        else
        { iOperator = 1; ConPrint("\n\rSIM300: not MTS\n\r"); }
      }
      if(RxIs("OK"))
        NewState = msInit;
      break;

    case msInit:
      if( StateChanged )
      {
        ConPrint("\n\rSIM300: initializing...\n\r");
        prt.TxCmd(szMdmCmdInit);
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR"))
        NewState = msRset;
      else if(RxIs("OK"))
        NewState = msConn;
      break;

    case msConn:
      if(StateChanged)
      {
        ConPrintf("\n\rSIM300: connecting to '%s'...\n\r",szMdmCmdConnParam);
        sprintf((char*)TxBuf,szMdmCmdConn,szMdmCmdConnParam);
//        ConPrint((char*)TxBuf);
        prt.TxCmd((char*)TxBuf);
        toutState.start(toTypeSec | 30);
      }
      else if( toutState.IsSignaled() )
      {
        ConPrint("\n\rSIM300: connecting timeout\n\r");
        NewState = msRset;
      }
      else if(RxIs("ERROR"))
      {
        ConPrint("\n\rSIM300: connecting error\n\r");
        NewState = msRset;
      }
      else if(RxIs("CONNECT OK"))
        NewState = msOnline;
      break;

    case msOnline:
      if( StateChanged )
      {
        Sending = FALSE;
        ConPrint("\n\rSIM300: online\n\r");
        toutState.start(LinkTimeout); // restart link timeout
      }
      else if( toutState.IsSignaled() ) 
      {
        ConPrint("\n\rSIM300: link timeout\n\r");
        NewState = msRset;
      }
      else if( Sending && RxIs("SEND OK") )
        Sending = FALSE;
      else if( Sending && ( RxIs("ERROR") || toutSend.IsSignaled() ) )
      {
        ConPrint("\n\rSIM300: sending failure\n\r");
        NewState = msRset;
      }
      else if( RxIs("ERROR") )
      {
        ConPrint("\n\rSIM300: unexpected error\n\r");
        NewState = msRset;
      }
      else
      {
        if( RxSize && HLI_receive(Rx,RxSize) )
        {
          toutState.start(LinkTimeout); // restart link timeout
          RxCnt++;
        }
        if( !Sending && TxSize==0 )
        {
          TxSize = HLI_totransmit(TxBuf,MaxTxSize);
          if(TxSize==0 && StateChanged) TxSize = HLI_totransmit(TxBuf,0);
        }
      }
      if( NewState==msOnline && TxSize>0 && (R & IO_TX) )
      {
        Sending = TRUE;
        if(TxSize) ConPrintf(" [Tx=%d] ",TxSize);
        char cmd[32];
        sprintf(cmd,"at+cipsend=%d", prt.GetTxRealSize(TxBuf,TxSize) );
        prt.TxCmd(cmd);
        prt.Tx(TxBuf,TxSize);
        TxSize=0;
        toutSend.start(toTypeSec | 10);
        TxCnt++;
      }
      break;

    case msRset:
      if(StateChanged)
      {
        ConPrint("\n\rSIM300: resetting\n\r");
        prt.TxCmd(szMdmCmdRset);
        toutState.start(toTypeSec | 10);
      }
      else if( toutState.IsSignaled() || RxIs("SHUT OK") || RxIs("ERROR") )
      {
#ifdef __7188XB          
        HLI.setRts(false); SYS::sleep(333); HLI.setRts(true);
#else
        HLI.setDtr(false); SYS::sleep(333); HLI.setDtr(true);
#endif
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

