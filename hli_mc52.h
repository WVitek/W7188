#pragma once

char* szMdmCmdConnParam = _HLI_hostname;

char* szMdmCmdDisc="AT^SISC=1";
char* szMdmCmdRset="AT^SMSO";
#if defined(__MYGPRS)
char* szMdmCmdInit1 = "AT&FE0&D2";
char* szMdmCmdInit2 = "AT^SICS=0,conType,GPRS0";
char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"mts\";^SICS=0,passwd,mts";
char* szMdmCmdInit4 = "AT^SISS=1,srvType,Socket";
char* szMdmCmdInit5 = "AT^SICS=0,inactTO,0;^SISS=1,conId,0;^SISS=1,alphabet,1";
char* szMdmCmdInit6 = "AT"; // Буферная
char* szMdmCmdConn = "AT^SICS=0,apn,internet.mts.ru;^SISS=1,address,\"sockudp://%s:19864;port=60978\";^SISO=1";
#elif defined(__UTNP_VPO)
char* szMdmCmdInit1 = "AT&FE0&D2";
char* szMdmCmdInit2 = "AT^SICS=0,conType,GPRS0";
char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"mts\";^SICS=0,passwd,mts";
char* szMdmCmdInit4 = "AT^SISS=1,srvType,Socket";
char* szMdmCmdInit5 = "AT^SICS=0,inactTO,0;^SISS=1,conId,0;^SISS=1,alphabet,1";
char* szMdmCmdInit6 = "AT"; // Буферная
char* szMdmCmdConn = "AT^SICS=0,apn,param65.ural;^SISS=1,address,\"sockudp://%s:19864;port=60978\";^SISO=1";
#elif defined(__UTNP_UPO)
char* szMdmCmdInit1 = "AT&FE0&D2";
char* szMdmCmdInit2 = "AT^SICS=0,conType,GPRS0";
// Test1
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625307170\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,fOaeRjwJ2CJU";
// 10.226.136.1 Test2
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625297716\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,JTTvfX3QGfeB";
// 10.226.136.2 Yazikovo 87km
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625297896\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,1XX8uhepOJiK";
// 10.226.136.3 Yazikovo 91km
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625297964\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,Ut7JzCm1ebEZ";
// 10.226.136.4 Yazikovo 108km
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625297967\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,7AcoVJPqKPEE";
// 10.226.136.5 Subhankulovo 241km
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298086\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,WMwqmUwiuNOS";
// 10.226.136.6 Turino 284
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298143\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,59RZ4FNFNe6A";
// 10.226.136.7 Turino 329
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298160\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,byUcKJPnLs10";
// 10.226.136.8 Turino 382
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298164\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,5tUejs3D5ith";
// 10.226.136.9 Turino 400
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298207\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,jtxMkJviDfPh";
// 10.226.136.10 Turino 422
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298304\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,seZ1JkjWwrkp";
// 10.226.136.11 Test3
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298407\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,rfwvUb9abr7Y";
// 10.226.136.12 Test4
char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625298414\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,ih4zQzFTRBOz";

// 10.226.110.130-190 510 ( from TM 491km/465km )
//char* szMdmCmdInit3 = "AT^SICS=0,alphabet,0;^SICS=0,user,\"9625307249\\00upouraltnp.gldn.gprs\";^SICS=0,passwd,SCjSPBaFpCKn";

char* szMdmCmdInit4 = "AT^SISS=1,srvType,Socket";
char* szMdmCmdInit5 = "AT^SICS=0,inactTO,0;^SISS=1,conId,0;^SISS=1,alphabet,1";
char* szMdmCmdInit6 = "AT"; // Буферная
char* szMdmCmdConn = "AT^SICS=0,apn,gt.msk;^SISS=1,address,\"sockudp://%s:19864;port=60978\";^SISO=1";

#elif defined(__TNGP)
char* aszMdmCmdInit[] = {"AT&F;E0;&D2;^SICS=0,conType,GPRS0;^SICS=0,alphabet,0;^SICS=0,user,mts;^SICS=0,passwd,mts;^SISS=1,srvType,Socket;^SISS=1,conId,0;^SISS=1,alphabet,1", ""};
int aMdmCmdInitPause[] =
{
50,
0
};
char* szMdmCmdConn="AT^SICS=0,apn,tngp.kazan;^SISS=1,address,sockudp://%s:19864;^SISO=1";
#elif defined(__BASHNEFT)
char* aszMdmCmdInit[] = {"AT&F;E0;&D2;^SICS=0,conType,GPRS0;^SICS=0,alphabet,0;^SICS=0,user,mts;^SICS=0,passwd,mts;^SISS=1,srvType,Socket;^SISS=1,conId,0;^SISS=1,alphabet,1", ""};
int aMdmCmdInitPause[] =
{
50,
0
};
char* szMdmCmdConn="AT^SICS=0,apn,bashneft.ufa;^SISS=1,address,sockudp://%s:19864;^SISO=1";
#else
char* aszMdmCmdInit[] = {"AT&F;E0;&D2;^SICS=0,conType,GPRS0;^SICS=0,alphabet,0;^SICS=0,user,mts;^SICS=0,passwd,mts;^SISS=1,srvType,Socket;^SISS=1,conId,0;^SISS=1,alphabet,1", ""};
int aMdmCmdInitPause[] =
{
50,
0
};
char* szMdmCmdConn="AT^SICS=0,apn,internet.mts.ru;^SISS=1,address,sockudp://%s:19864;^SISO=1";
#endif

#include "PrtLiner.h"

#define MaxTxSize 400

#define RxIs(Resp) __rxequal(Rx,Resp,RxSize)
#define RxBeg(Resp) __rxBeg(Rx,Resp,RxSize)
#define RxEnd(Resp) __rxEnd(Rx,Resp,RxSize)
#define LinkTimeout (toTypeSec | 180)

#define MaxNoDataDisconnect 11

class THREAD_HLI : public THREAD {
  COMPORT *HLI;
public:

THREAD_HLI(COMPORT &HLI):THREAD(4096+2048){
  this->HLI=&HLI;
}

void execute(){
  SYS::sleep(5000); // При одновременном запуске модем не успевает инициализироваться.
  COMPORT& HLI=*(this->HLI);
  HLI.install(9600);
  PRT_LINER prt(&HLI);
  prt.Open();
  prt.TxCmd("AT+IPR=19200");
  SYS::sleep(100);
  // Для универсальности (TC65) еще и на 115200. Можно вообще по всем пробежаться.
  HLI.install(115200);
  prt.TxCmd("AT+IPR=19200");
  SYS::sleep(100);
  HLI.install(19200);
  dbg("\n\rHLI started (MC52-GPRS-UDP)");
  //U8 RxCnt=0, TxCnt=0;
  TIMEOUTOBJ toutState, toutTx;//, toutReset;
  enum {msDetect,msInit1,msInit2,msInit3,msInit4,msInit5,msInit6,msConn,msOnline,msDisc1,msDisc2,msDisc3,/*msRset,*/msUnreachable}
    State = msUnreachable, NewState = msDetect;
  enum {osNone, osTx, osRx } OnlineState = osNone;
  int RxSize, TxSize, TxRealSize;
  int nTout = 0;
  int cmdCounter = 0;
  int NoDataDisconnectCounter = 0;
  int NoDataDisconnectType = 0;
  BOOL IsTextMode = TRUE;
  int nSISR = 0;
  U8 TxBuf[MaxTxSize];
  //toutReset.start(toTypeSec | MaxNoDataExchangeTime);
  //toutReset.setSignaled();
  while(!Terminated){
    U16 R = prt.ProcessIO();
    //ConPrintf("\n\rDbg0 R=%d ", R);
    if(R & IO_UP==0)
    {
      //ConPrint("\n\r Debug Hli sleep ");
      SYS::sleep(20);
      continue;
    }
    const void *Rx;
    RxSize = 0;
    if (R & IO_RX)
    {
      //ConPrint("\n\r Debug RX");
      Rx = prt.GetBuf( RxSize );
      //ConPrintf("\n\r   Debug Hli_mc52 2 4 RxSiza=(%d)", RxSize);
      if(RxSize)
      {
        if(IsTextMode)
          ConPrintf(" <Rx=%d:%.*s> ",RxSize,RxSize,Rx);
        else
          ConPrintf(" [Rx=%d] ",RxSize);
        if(RxIs("RING"))
          NewState = msDisc1;
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

    //ConPrint("\n\r   Debug Hli_mc52 2 5 ");

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
        if(StateChanged){
          nTout = 0;
          if (++NoDataDisconnectCounter > MaxNoDataDisconnect)
          {
            NoDataDisconnectCounter = 0;
            if (NoDataDisconnectType++ > 1)
            {
              ConPrint("\n\rHard reset of MC52...\n\r");
              NoDataDisconnectType = 0;
              toutLink.setSignaled();
              SYS::sleep(5000);
            }
            else
            {
              ConPrint("\n\rSoft reset of MC52...\n\r");
              prt.TxCmd("AT+CFUN=1,1");
              SYS::sleep(7000);
            }
          }
          else
          {
            ConPrintf("\n\r [%d] attempts left before hard reset...\n\r", MaxNoDataDisconnect - NoDataDisconnectCounter);
          }
        }
        else if(nTout++ >= 5)
        {
    #ifdef __UseAvgTOfs
          HLI_chechForEmergencyResync();
    #endif
          nTout = 0;
          NewState = msDisc1;//msRset;
          break;
        }
        ConPrint("\n\rMC52: detecting...\n\r");
#ifdef __7188XB
        HLI.setRts(false); SYS::sleep(1000); HLI.setRts(true);
#else
        HLI.setDtr(false); SYS::sleep(1000); HLI.setDtr(true);
#endif
        if(R & IO_TX)
           prt.TxCmd("at+csq");
        toutState.start(toTypeSec | 5);
      }
      if(RxIs("OK"))
        NewState = msInit1;
      break;

    case msInit1:
      if( StateChanged )
      {
        nTout = 0;
        ConPrint("\n\rMC52: initializing [step 1]...\n\r");
        SYS::sleep(1000);
        prt.TxCmd(szMdmCmdInit1);
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR")){
        if(nTout++ >= 5)
           NewState = msDisc1;
        break;
      }
      else if(RxIs("OK"))
        NewState = msInit2;
      break;

    case msInit2:
      if( StateChanged )
      {
        nTout = 0;
        ConPrint("\n\rMC52: initializing [step 2]...\n\r");
        SYS::sleep(1000);
        prt.TxCmd(szMdmCmdInit2);
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR")){
        if(nTout++ >= 5)
           NewState = msDisc1;
        break;
      }
      else if(RxIs("OK"))
        NewState = msInit3;
      break;

    case msInit3:
      if( StateChanged )
      {
        nTout = 0;
        ConPrint("\n\rMC52: initializing [step 3]...\n\r");
        SYS::sleep(1000);
        prt.TxCmd(szMdmCmdInit3);
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR")){
        if(nTout++ >= 5)
           NewState = msDisc1;
        break;
      }
      else if(RxIs("OK"))
        NewState = msInit4;
      break;

    case msInit4:
      if( StateChanged )
      {
        nTout = 0;
        ConPrint("\n\rMC52: initializing [step 4]...\n\r");
        SYS::sleep(1000);
        prt.TxCmd(szMdmCmdInit4);
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR")){
        if(nTout++ >= 5)
           NewState = msDisc1;
        break;
      }
      else if(RxIs("OK"))
        NewState = msInit5;
      break;

    case msInit5:
      if( StateChanged )
      {
        nTout = 0;
        ConPrint("\n\rMC52: initializing [step 5]...\n\r");
        SYS::sleep(1000);
        prt.TxCmd(szMdmCmdInit5);
        toutState.start(toTypeSec | 10);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR")){
        if(nTout++ >= 10)
           NewState = msDisc1;
        break;
      }
      else if(RxIs("OK"))
        NewState = msInit6;
      break;

    case msInit6:
      if( StateChanged )
      {
        nTout = 0;
        ConPrint("\n\rMC52: initializing [step 6]...\n\r");
        SYS::sleep(1000);
        prt.TxCmd(szMdmCmdInit6);
        toutState.start(toTypeSec | 5);
      }
      else if(toutState.IsSignaled() || RxIs("ERROR")){
        if(nTout++ >= 5)
           NewState = msDisc1;
        break;
      }
      else if(RxIs("OK"))
        NewState = msConn;
      break;

   case msConn:
      if(StateChanged)
      {
        ConPrintf("\n\rMC52: connecting to '%s'...\n\r",szMdmCmdConnParam);
        SYS::sleep(1000);
        sprintf((char*)TxBuf,szMdmCmdConn,szMdmCmdConnParam);
        //ConPrint((char*)TxBuf);
        prt.TxCmd((char*)TxBuf);
        toutState.start(toTypeSec | 30);
      }
      else if( toutState.IsSignaled() )
      {
        ConPrint("\n\rMC52: connecting timeout\n\r");
        NewState = msDisc1;
      }
      else if(RxIs("ERROR"))
      {
        ConPrint("\n\rMC52: connecting error\n\r");
        NewState = msDisc1;
      }
      else if(RxIs("^SISW: 1, 1"))
        NewState = msOnline;
      break;

    case msOnline:
      if( StateChanged )
      {
        OnlineState = osNone;
        nTout = 0; nSISR = 1;
        ConPrint("\n\rMC52: online\n\r");
        SYS::sleep(5000);
        toutState.start(LinkTimeout); // restart link timeout
      }
      else if( toutState.IsSignaled() )
      {
        ConPrint("\n\rMC52: link timeout\n\r");
        NewState = msDisc1;
        break;
      }
      if(RxIs("^SISR: 1, 1"))
        nSISR++;


      //ConPrintf("\n\r   Debug Hli_mc52 3 2 nSISR(%d)", nSISR);

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
          //TxCnt++;
        }
        else if(RxIs("OK")){
          nTout = 0;
          OnlineState = osNone;
        }
        else if(RxIs("ERROR") || toutTx.IsSignaled()){
          ConPrint("\n\rMC52: sending failure\n\r");
          //SYS::sleep(1000);
          //if (nTout++ > 5) {
            NewState = msDisc1;
          //}
          //else OnlineState = osNone;
        }
        break;

      case osRx:
        if(RxBeg("^SISR: 1,")){
          //ConPrint("\n\rDbg1 ");
          // can read data
          IsTextMode = FALSE;
          NoDataDisconnectCounter = 0; // раз что-то пришло - сбрасываем счетчик неудачных попыток соединения.
        }
        else if(RxIs("OK")){
          //ConPrint("\n\rDbg2 ");
          // all data readed
          OnlineState = osNone;
          IsTextMode = TRUE;
        }
        else if(RxIs("ERROR")){
          //ConPrint("\n\rDbg3 ");
          NewState = msDisc1;
        }
        else if( RxSize && HLI_receive(Rx,RxSize) ) {
            //toutReset.start(toTypeSec | MaxNoDataExchangeTime);
            toutState.start(LinkTimeout); // restart link timeout
            //RxCnt++;
        }
        break;
      }
      break; // case msOnline

    case msDisc1:
      if(StateChanged)
      {
        SYS::sleep(5000);
        IsTextMode = TRUE;
        ConPrint("\n\rMC52: IC Info\n\r");
        prt.TxCmd("AT^SICI?");
        toutState.start(toTypeSec | 3);
      }
      else if( toutState.IsSignaled() || RxIs("OK") || RxIs("ERROR") )
      {
        NewState = msDisc2;
      }
      break;

    case msDisc2:
      if(StateChanged)
      {
        IsTextMode = TRUE;
        ConPrint("\n\rMC52: IS Info\n\r");
        prt.TxCmd("AT^SISI?");
        toutState.start(toTypeSec | 3);
      }
      else if( toutState.IsSignaled() || RxIs("OK") || RxIs("ERROR") )
      {
        NewState = msDisc3;
      }
      break;

    case msDisc3:
      if(StateChanged)
      {
        IsTextMode = TRUE;
        ConPrint("\n\rMC52: disconnecting\n\r");
        prt.TxCmd(szMdmCmdDisc);
        SYS::sleep(5000);
        toutState.start(toTypeSec | 20);
      }
      else if( toutState.IsSignaled() || RxIs("OK") || RxIs("ERROR") )
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
        ConPrint("\n\rMC52: resetting\n\r");
        prt.TxCmd(szMdmCmdRset);
        DIO::SetDO1(false); // modem power down
        SYS::sleep(5000);
        DIO::SetDO1(true); // modem power up
        toutState.start(toTypeSec | 10);
        toutReset.start(toTypeSec | MaxNoDataExchangeTime);
      }
      else if(RxIs("^SHUTDOWN")){
        ConPrint("\n\rMC52: ^SHUTDOWN\n\r");
        NewState = msDetect;
      }
      else if( toutState.IsSignaled() || RxIs("ERROR") )
      {
        NewState = msDetect;
      }
      break;
//*/
    } // end of switch(State)

//#ifndef __SHOWADCDATA
//    SYS::dbgLed( (U32(NewState)<<16) | (U32(RxCnt)<<8) | TxCnt );
//#endif
    sleep(1); // due to first time no recieve error
    if(Rx!=NULL){
      //ConPrint(" /Dbg prt.Rx(NULL)/ ");
      prt.Rx(NULL);
    }
    else
    {
      //ConPrint(" /Dbg rr/ ");
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


