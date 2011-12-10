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
inline void I7K_StatRead(int &Qry, int &Ans) {
  SYS::cli();
  Qry=__CntI7K_Query;  __CntI7K_Query =0;
  Ans=__CntI7K_Answer; __CntI7K_Answer=0;
  SYS::sti();
}

#include "Module.h"
#include "WHrdware.hpp"

class THREAD_POLL_I7K : public THREAD {
  U16 pollPort;
  U32 baudRate;
  MODULES *modules;
  PU_LIST *pollList;
public:
  THREAD_POLL_I7K(U16 pollPort=0, U32 baudRate=38400)
  {
    if(pollPort==0)
#ifdef __PollPort
      this->pollPort=__PollPort;
#else
      this->pollPort=2;
#endif
    else this->pollPort=pollPort;
    this->baudRate=baudRate;
    modules = ctx_I7K.Modules;
    pollList = ctx_I7K.PollList;
  }
  void execute();
};

void THREAD_POLL_I7K::execute(){
  COMPORT& RS485=GetCom(pollPort);
  RS485.install(baudRate);
  dbg3("\n\rPOLL_I7K started @ COM%d:%ld",pollPort,baudRate);
  U8 Query[16];
  U8 Resp[256];
  POLL_UNIT *pu,*pu_;
  int i0=0, i;
  //Realtime=TRUE;
  //dbg2("\n\rModulesCount=%d",modules->Count());
  // Configure modules
  while(TRUE){
    BOOL Quit=TRUE;
    for(i=modules->Count()-1; i>=0; i--){
      if((*modules)[i]->GetConfigCmd(Query)){
        RS485.sendCmdTo7000(Query,TRUE);
        RS485.receiveLine(Resp,100,TRUE);
        Quit=FALSE;
      }
    }
    if(Quit)break;
  }
  //dbg2("\n\rPollListCount=%d",pollList->Count());
  pu_=(*pollList)[i0];
  // send first command
  if(!pu_->GetPollCmd(Query)) if(--i0<0) i0=pollList->Count()-1;;
  RS485.sendCmdTo7000(Query,TRUE);
  // polling loop
  while(!Terminated){
    // prepare next command
    pu=(*pollList)[i0];
    if(!pu->GetPollCmd(Query)) if(--i0<0) i0=pollList->Count()-1;
    // wait response
    Resp[0]=0;
    RS485.RxEvent().waitFor(49);
    // send next commnad
    RS485.sendCmdTo7000(Query,TRUE);
    // process response string
    int R=RS485.receiveLine(Resp,0,TRUE);
    U8* Tmp = (R==0) ? Resp : NULL;
    int Ans = (pu_->response(Tmp)) ? 1: 0;
    I7K_StatAdd(1,Ans);
    //SYS::sleep(500);
    S(0x04);
    pu_=pu;
  }
  S(0x00);
  dbg("\n\rPOLL_I7K stopped");
}


#endif // POLL_I7K_H_INCLUDED
