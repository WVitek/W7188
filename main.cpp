//__7188XA __GPRS_TC65 __TNGP
#define MyAddr 1
#define ADC_Freq 25 // Herz
#undef __ARQ

#include <stdio.h>
#include <string.h>
#include "WKern.hpp"
#include "WComms.hpp"
#include "WCRCs.hpp"

//#define _HLI_hostname "192.168.192.18";
//#define _HLI_hostname "wvn.selfip.net"
//#define _HLI_hostname "tngp.selfip.net"

static BOOL LedIsOn=FALSE;
#define switchLed() SYS::led(LedIsOn=!LedIsOn)

U32 __HLI_BaudRate;

#define _HLI_HDR
#if !defined(__ARQ)
#include "HLI.h"
#else
#include "HLI_ARQ.h"
#endif
//#define HLI_linkCheck()
//#define setNeedConnection(x)

#include "Module.h"
#ifndef __MTU
//***** BEG Analog Inputs
// TNGP
I7017 i7017a(0x01); // ( Address )
//I7017 i7017b(0x02);
//*
PU_ADC_7K
  puAD0(&i7017a,0), // ( Module, AIN, ArchNum )
  puAD1(&i7017a,1),
  puAD2(&i7017a,2);
//  puAD3(&i7017b,0,3),
//  puAD4(&i7017b,1,4),
//  puAD5(&i7017b,2,5);
/*/
PU_ADC_7K // testing mode only
  puAD0(&i7017a,0), // ( Module, AIN, ArchNum )
  puAD1(&i7017b,0);
//*/
//***** END Analog Inputs
#endif

//***** BEG Digital inputs & events
//*
//***** 70DI (TNGP: KP 74)
MODULE i70DI(0xFF);
PU_DI puDI(&i70DI,0x00DF);
/*/
//***** No DI (TNGP: GNS & T9)
PU_DI puDI(NULL,0x00);
//*/
//***** END Digital inputs & events

#include "Polling.h"

#undef _HLI_HDR
#if !defined(__ARQ)
#include "HLI.h"
#else
#include "HLI_ARQ.h"
#endif

#ifdef __GPS_TIME
#include "GPS_TIME.h"
#endif

struct COM_PARAMS
{
    U8 com;
    U32 speed;
};

U8 const *findCmdLineArg(char *prefix)
{
    int pn = strlen(prefix);
    CMD_LINE const * cl = GetCmdLine();
    int n=cl->Length;
    U8 const * cmdLine = &(cl->Data[0]);
    for(int i=0; i<n; i++, cmdLine++)
        if(prefix[0]==cmdLine[0])
            if(strncmp(prefix,(char*)cmdLine,pn)==0)
                return cmdLine+pn;
    return NULL;
}

U16 GetU16Param(char *prefix, U16 defaultValue)
{
    U8 const *arg = findCmdLineArg(prefix);
    if(arg==NULL)
        return defaultValue;
    int n=0;
    while(true)
    {
        U8 c=arg[n];
        if(c==0 || c==' ' || c==0x13 || ++n==128)
            break;
    }
    return (U16)FromHexStr(arg,n);
}

bool GetComParams(char *prefix, COM_PARAMS *res)
{
    U8 const *arg = findCmdLineArg(prefix);
    if(arg==NULL)
        return false;
    int n=0;
    int iComma=-1;
    while(true)
    {
        U8 c=arg[n];
        if(c==':')
            iComma=n;
        if(c==0 || c==' ' || c==0x13 || ++n==128)
            break;
    }
    U8 com = 0;
    U32 speed = 0;
    if(iComma>=0)
    {
        com = (U8)FromDecStr(arg,iComma);
        speed = FromDecStr(&(arg[iComma+1]),n-iComma-1);
    }
    else com = (U8)FromDecStr(arg,n);
    if(com!=0) res->com = com;
    if(speed!=0) res->speed = speed;
    return true;
}

cdecl main()
{
#if   defined(__7188XA)
    ConPrint("\n\rW.I7188XA(D)");
#elif defined(__7188XB)
    ConPrint("\n\rW.I7188XB(D)");
#elif defined(__7188)
    ConPrint("\n\rW.I7188(D)");
#endif
#if defined(__BORLANDC__)
    ConPrint("\n\rCompiler: Borland C++\n\r");
#elif defined(__WATCOMC__)
    ConPrint("\n\rCompiler: Open Watcom C++\n\r");
#endif
    SYS::startKernel();
    //SYS::sleep(2000); // need to avoid bug when very fast start from autoexec :-(
    dbg("\n\rMAIN started\n\r");

    THREAD_POLLING* ThdP;
    {
        COM_PARAMS cp;
        cp.com = __PollPort;
        cp.speed = 38400;
        GetComParams(" poll=",&cp);
        ThdP = new THREAD_POLLING(cp.com, cp.speed, GetU16Param(" MTUs=",0));
        ThdP->run();
    }

#if !defined(__7188XB) || !defined(__ConPort)
    #define ThdHLI
    THREAD_HLI* ThdHLI1;
    {
        COM_PARAMS cp;
    #ifdef __HLI_COM2
        cp.com = 2;
    #else
        cp.com = 1;
    #endif
        cp.speed = 19200;
        GetComParams(" hli=",&cp);
        __HLI_BaudRate = cp.speed;
        ThdHLI1 = new THREAD_HLI(cp.com);
        ThdHLI1->run();
    }
#endif

#ifdef __MTU
    while(!ThdP->scanComplete)
        SYS::sleep(100);
#endif

#ifdef __GPS_TIME
    THREAD_GPS* ThdGPS =new THREAD_GPS();  ThdGPS->run();
#endif
    THREAD_TMR* ThdTmr =new THREAD_TMR();  ThdTmr->run();

    // 'RESTART' event
    EVENT_DI Event;
    SYS::getNetTime(Event.Time);
    Event.Channel=255;
    Event.ChangedTo=0;
    puDI.EventDigitalInput(Event);
    //
    BOOL Quit=FALSE;
    //  TIMEOUTOBJ too;
    //  too.start(toTypeSec | 10);
    //  EVENT* evtCTS[] = { &GetCom(1).EventLoCTS(), &GetCom(1).EventHiCTS()};
    //  evtCTS[0]->reset();
    //  evtCTS[1]->reset();
    while(TRUE){
        while(ConBytesInRxB()){
            char c=ConReadChar();
            ConWriteChar(c);
            if(c==27) Quit=TRUE;
        }
        if(Quit) break;
        SYS::sleep(333);
        //    int i = SYS::waitForAny(20, 2, (SYNCOBJ**)&evtCTS);
        //    if (i>0) {
        //        if(evtCTS[0]->IsSignaled()){
        //            evtCTS[0]->reset();
        //            ConPrint("\n\rCTS0");
        //        }
        //        else if(evtCTS[1]->IsSignaled()){
        //            evtCTS[1]->reset();
        //            ConPrint("\n\rCTS1");
        //        }
        //        else ConPrint("\n\rCTS???");
        //    }
        //    if(GetCom(1).IsClearToSend())
        //        ConPrint("CTS");
        //    else ConPrint("cts");
    }
    SYS::stopKernel();
#ifdef ThdHLI
    delete ThdHLI1;
#endif
#ifdef __GPS_TIME
    delete ThdGPS;
#endif
    delete ThdP;
    delete ThdTmr;
    dbg("\n\rMAIN stopped\n\r");
    SYS::DelayMs(100);
    return 0;
}
