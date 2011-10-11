#pragma once

//class MSGBUFFER

#define MAXMSGDATASIZE 4096

struct MSG {
  MSG* Next;
  char Data[MAXMSGDATASIZE];
};

class MSGBUFFER {
  MSG *First, *Last;
  int Count;
  CRITICALSECTION MSGCS;
  void delMsg();
public:
  BOOL getMsg(char* Data);
  void putMsg(char* Data);
  BOOL readMsg(char* Data);
};

void MSGBUFFER::delMsg(){
  MSG* Tmp = First->Next;
  SYSTEM::freeMem(First);
  First=Tmp;
  if(Tmp==NULL) Last=NULL;
  Count--;
}

BOOL MSGBUFFER::getMsg(char* Data){
  MSGCS.acquire();
  BOOL Res = Count>0;
  if(Res){
    strcpy(Data,First->Data);
    delMsg();
  }
  MSGCS.release();
  return Res;
}

void MSGBUFFER::putMsg(char* Data){
  MSGCS.acquire();
  if(++Count>1000) delMsg();
  WORD Size = sizeof(MSG)-MAXMSGDATASIZE+strlen(Data)+1;
  MSG* Msg = (MSG*)SYSTEM::getMem(Size);
  memset(Msg,0,Size);
  strcpy(Msg->Data,Data);
  if (Last!=NULL) Last->Next=Msg; else First=Msg;
  Last=Msg;
  MSGCS.release();
}

BOOL MSGBUFFER::readMsg(char* Data){
  MSGCS.acquire();
  BOOL Res = Count>0;
  if (Res) strcpy(Data,First->Data);
  MSGCS.release();
  return Res;
}

