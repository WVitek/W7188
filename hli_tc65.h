#pragma once

char* szMdmCmdConnParam = _HLI_hostname;
char* szMdmCmdInit="AT&F;E0;&D2;^SICS=0,conType,GPRS0;^SICS=0,alphabet,0;^SICS=0,user,mts;^SICS=0,passwd,mts;^SISS=1,srvType,Socket;^SISS=1,conId,0;^SISS=1,alphabet,1";

char* szMdmCmdDisc="AT^SISC=1";
char* szMdmCmdRset="AT^SMSO";
#if defined(__TNGP)
char* szMdmCmdConn="AT^SICS=0,apn,tngp.kazan;^SISS=1,address,sockudp://%s:19864;^SISO=1";
#elif defined(__BASHNEFT)
char* szMdmCmdConn="AT^SICS=0,apn,bashneft.ufa;^SISS=1,address,sockudp://%s:19864;^SISO=1";
#else
char* szMdmCmdConn="AT^SICS=0,apn,internet.mts.ru;^SISS=1,address,sockudp://%s:19864;^SISO=1";
#endif

#include "PrtLiner.h"

#define MaxTxSize 400
//#define MaxNoDataExchangeTime 600

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
  dbg("\n\rHLI started (TC65-GPRS-UDP)");
  U8 RxCnt=0, TxCnt=0;
  TIMEOUTOBJ toutState, toutTx;//, toutReset;
  enum {msDetect,msInit,msConn,msOnline,msDisc,/*msRset,*/msUnreachable}
    State = msUnreachable, NewState = msDetect;
  enum {osNone, osTx, osRx } OnlineState = osNone;
  int RxSize, TxSize, TxRealSize;
  int nTout = 0;
  BOOL IsTextMode = TRUE;
  int nSISR = 0;
  U8 TxBuf[MaxTxSize];
  //toutReset.start(toTypeSec | MaxNoDataExchangeTime);
  //toutReset.setSignaled();
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
        if(IsTextMode) 
          ConPrintf(" <Rx=%d:%.*s> ",RxSize,RxSize,Rx);
        else
          ConPrintf(" [Rx=%d] ",RxSize);
        if(RxIs("RING"))
          NewState = msDisc;
        else if(RxIs("^SYSSTART"))
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
      //if(toutReset.IsSignaled() )
      //{
      //  NewState = msRset;
      //  break;
      //}
      if( StateChanged || toutState.IsSignaled() )
      {
        if(StateChanged)
           nTout = 0;
        else if(nTout++ >= 10)
        {
           nTout = 0;
           NewState = msDisc;//msRset;
           break;
        }
        ConPrint("\n\rTC65: detecting...\n\r");
#ifdef __7188XB          
        HLI.setRts(false); SYS::sleep(1000); HLI.setRts(true);
#else
        HLI.setDtr(false); SYS::sleep(1000); HLI.setDtr(true);
#endif
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
        nTout = 0;
        ConPrint("\n\rTC65: initializing...\n\r");
        SYS::sleep(1000);
        prt.TxCmd(szMdmCmdInit);
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR")){
        if(nTout++ >= 5)
           NewState = msDisc;
        break;
      }
      else if(RxIs("OK"))
        NewState = msConn;
      break;

    case msConn:
      if(StateChanged)
      {
        ConPrintf("\n\rTC65: connecting to '%s'...\n\r",szMdmCmdConnParam);
        SYS::sleep(1000);
        sprintf((char*)TxBuf,szMdmCmdConn,szMdmCmdConnParam);
        //ConPrint((char*)TxBuf);
        prt.TxCmd((char*)TxBuf);
        toutState.start(toTypeSec | 30);
      }
      else if( toutState.IsSignaled() )
      {
        ConPrint("\n\rTC65: connecting timeout\n\r");
        NewState = msDisc;
      }
      else if(RxIs("ERROR"))
      {
        ConPrint("\n\rTC65: connecting error\n\r");
        NewState = msDisc;
      }
      else if(RxIs("^SISW: 1, 1"))
        NewState = msOnline;
      break;

    case msOnline:
      if( StateChanged )
      {
        OnlineState = osNone;
        nTout = 0; nSISR = 1;
        ConPrint("\n\rTC65: online\n\r");
        SYS::sleep(5000);
        toutState.start(LinkTimeout); // restart link timeout
      }
      else if( toutState.IsSignaled() ) 
      {
        ConPrint("\n\rTC65: link timeout\n\r");
        NewState = msDisc;
        break;
      }
      if(RxIs("^SISR: 1, 1"))
        nSISR++;
        
      switch(OnlineState) {

      case osNone:
        if(nSISR > 0){
          nSISR--;
          // we have data in TC65's rx buffer
          prt.TxCmd("at^sisr=1,800");
          OnlineState = osRx;
          ConPrint("\n\rRx state\n\r");
        }
        else{
          if(nTout==0) // previous sending is successfull?
            TxSize = HLI_totransmit(TxBuf,MaxTxSize);
          if(TxSize==0 && StateChanged)
            TxSize = HLI_totransmit(TxBuf,0); // send some first packet
          if(TxSize>0) {
            char cmd[32];
            TxRealSize = prt.GetTxRealSize(TxBuf,TxSize);
            sprintf(cmd,"at^sisw=1,%d", TxRealSize );
            prt.TxCmd(cmd);
            OnlineState = osTx;
            ConPrint("\n\rTx state\n\r");
            toutTx.start(toTypeSec | 10);
          }
        }
        break;

      case osTx:
        if(RxBeg("^SISW: 1,")){
          // permission granted, do transmission
          prt.Tx(TxBuf,TxSize);
          ConPrintf(" [Tx=%d (%d)] ",TxSize,TxRealSize);
          OnlineState = osTx;
          toutTx.start(toTypeSec | 10);
          TxCnt++;
        }
        else if(RxIs("OK")){
          nTout = 0;
          OnlineState = osNone;
        }
        else if(RxIs("ERROR") || toutTx.IsSignaled()){
          ConPrint("\n\rTC65: sending failure\n\r");
          //SYS::sleep(1000);
          //if (nTout++ > 5) {
            NewState = msDisc;
          //}
          //else OnlineState = osNone;
        }
        break;

      case osRx:
        if(RxBeg("^SISR: 1,")){
          // can read data
          IsTextMode = FALSE;
        }
        else if(RxIs("OK")){
          // all data readed
          OnlineState = osNone;
          IsTextMode = TRUE;
        }
        else if(RxIs("ERROR"))
          NewState = msDisc;
        else if( RxSize && HLI_receive(Rx,RxSize) )
        {
          //toutReset.start(toTypeSec | MaxNoDataExchangeTime);
          toutState.start(LinkTimeout); // restart link timeout
          RxCnt++;
        }
        break;        
      }
      
      break; // case msOnline

    case msDisc:
      if(StateChanged)
      {
        IsTextMode = TRUE;
        ConPrint("\n\rTC65: disconnecting\n\r");
        prt.TxCmd(szMdmCmdDisc);
        SYS::sleep(5000);
        toutState.start(toTypeSec | 10);
      }
      else if( toutState.IsSignaled() || RxIs("OR") || RxIs("ERROR") )
      {
        NewState = msDetect;
      }
      break;
/*
    case msRset:
      if(StateChanged)
      {
        EVENT_DI Event;
        SYS::getNetTime(Event.Time);
        Event.Channel=255; Event.ChangedTo=2; // 'modem restart' event
        puDI.EventDigitalInput(Event);
        
        IsTextMode = TRUE;
        ConPrint("\n\rTC65: resetting\n\r");
        prt.TxCmd(szMdmCmdRset);
        DIO::SetDO1(false); // modem power down
        SYS::sleep(5000);
        DIO::SetDO1(true); // modem power up
        toutState.start(toTypeSec | 10);
        toutReset.start(toTypeSec | MaxNoDataExchangeTime);
      }
      else if(RxIs("^SHUTDOWN")){
        ConPrint("\n\rTC65: ^SHUTDOWN\n\r");
        NewState = msDetect;
      }
      else if( toutState.IsSignaled() || RxIs("ERROR") )
      {
        NewState = msDetect;
      }
      break;
//*/
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

