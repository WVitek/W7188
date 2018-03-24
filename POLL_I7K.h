#pragma once
#ifndef POLL_I7K_H_INCLUDED
#define POLL_I7K_H_INCLUDED

int __CntI7K_Query=0, __CntI7K_Answer=0;
inline void I7K_StatAdd(int Qry, int Ans) {
  SYS::cli();
  __CntI7K_Query +=Qry;
  __CntI7K_Answer+=Ans;
  SYS::sti();
}
inline void I7K_StatRead(int &Qry, int &Ans)
{
  SYS::cli();
  Qry=__CntI7K_Query;  __CntI7K_Query =0;
  Ans=__CntI7K_Answer; __CntI7K_Answer=0;
  SYS::sti();
}

class THREAD_I7K_SAMPLER : public THREAD
{
public:
void execute()
{
    int n1=ctx_I7K.PollList->Count()-1;
    dbg("\n\rSTART I7K_Sampler");
    while(!Terminated)
    {
        S(0x01);
        SYS::sleep(toTypeNext | ctx_I7K.Period);
        TIME Time;
        SYS::getNetTime(Time);
        S16 dt = (S16)(Time % ctx_I7K.Period);
        if(dt>ctx_I7K.Period>>1)
        {  // секунда ещё не закончилась, т.к. время во время сна скорректировалось "назад", нужно "доспать"
            dt = ctx_I7K.Period-dt;
            Time += dt;
            SYS::sleep(dt);
        }
        else Time -= dt;
        // latch
        for(int j=n1; j>=0; j--)
            (*ctx_I7K.PollList)[j]->latchPollData();
        S(0x02);
        // free CPU
        SYS::switchThread();
        S(0x03);
        // sample latched data
        for(int j=n1; j>=0; j--)
            (*ctx_I7K.PollList)[j]->doSample(Time);
    }
    S(0x00);
    dbg("\n\rSTOP I7K_Sampler");
}
};


#include "Module.h"
#include "WHrdware.hpp"

class THREAD_I7K_POLL : public THREAD
{
    U16 pollPort;
    U32 baudRate;
    public:
    THREAD_I7K_POLL(U16 pollPort=0, U32 baudRate=38400)
    {
        if(pollPort==0)
        #ifdef __PollPort
            this->pollPort=__PollPort;
        #else
            this->pollPort=2;
        #endif
        else this->pollPort=pollPort;
        this->baudRate=baudRate;
        //modules = ctx_I7K.Modules;
        //pollList = ctx_I7K.PollList;
        //ConPrint("\n\r Dbg 1");
    }

    void execute()
    {
        //ConPrint("\n\r Dbg 2");
        COMPORT& RS485=GetCom(pollPort);
        RS485.install(baudRate);
        int nPoll=ctx_I7K.PollList->Count();
        ConPrintf("\n\rSTART I7K_Poll[%d] @ COM%d:%ld",nPoll,pollPort,baudRate);
        U8 Query[16];
        U8 Resp[256];
        POLL_UNIT *pu,*pu_;
        //Realtime=TRUE;
        S(0x01);
        // Configure modules
        while(TRUE)
        {
            BOOL Quit=TRUE;
            for(int j=ctx_I7K.Modules->Count()-1; j>=0; j--)
                if((*ctx_I7K.Modules)[j]->GetConfigCmd(Query))
                {
                    RS485.sendCmdTo7000(Query,TRUE);
                    RS485.receiveLine(Resp,100,TRUE);
                    Quit=FALSE;
                }
            if(Quit)break;
        }
        int i=0;
/*
        while(!Terminated)
        {
            pu=(*ctx_I7K.PollList)[i];
            BOOL more = pu->GetPollCmd(Query);

            //RS485.clearRxBuf();
            RS485.sendCmdTo7000(Query,TRUE);
            Resp[0]=0;
            //SYS::sleep(20);
            int r = RS485.receiveLine(Resp,30,TRUE);
            if(r!=0)
            {
                SYS::sleep(33);
                ConPrintf("\r\n%d:%d %s ? %s|",i,r,Query,Resp);
                r = RS485.receiveLine(Resp,0,TRUE);
                ConPrint((char*)Resp);
            }
            int Ans = pu->response((r==0)?Resp:NULL) ? 1: 0;
            I7K_StatAdd(1,Ans);

            if(!more && --i<0)
                i=nPoll-1;
        #ifdef __MTU
            SYS::sleep(17);
        #endif
        }
/*/
        pu_=(*ctx_I7K.PollList)[i];
        // send first command
        int i0=i;
        if(!pu_->GetPollCmd(Query)) if(--i<0) i=nPoll-1;;
        RS485.sendCmdTo7000(Query,TRUE);
        // polling loop
        while(!Terminated)
        {
            // prepare next command
            pu=(*ctx_I7K.PollList)[i];
            if(!pu->GetPollCmd(Query)) if(--i<0) i=nPoll-1;
            // wait response
            //Resp[0]=0;
            BOOL rcvd = RS485.RxEvent().waitFor(30);
            S(0x02);
            // send next commnad
            RS485.sendCmdTo7000(Query,TRUE);
            // process response string
            S(0x03);
            int r = RS485.receiveLine(Resp,0,TRUE);
        #ifdef __I7K_NOANS
            if(r!=0)
            {
                pu_->GetPollCmd(Query);
                dbg3("\r\nNOANS: %d, %s",r,Query);
            }
        #endif
            int Ans = (r==0 && pu_->response(Resp)) ? 1: 0;
            I7K_StatAdd(1,Ans);
        #ifdef __MTU
            SYS::sleep(17);
        #endif
            S(0x04);
            i0=i;
            pu_=pu;
        }
//*/
        S(0x00);
        dbg("\n\rSTOP I7K_Poll");
    }
};

#endif // POLL_I7K_H_INCLUDED
