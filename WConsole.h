#pragma once

#ifndef __BORLANDC__
#include <stdarg.h>
#endif
#include <stdio.h>

#if !defined(__ConPort) && !defined(__7188XB)
  #define __ConPort 4
#endif

#if defined(__ConPort)
COMPORT *pCon = NULL;
char *pConBuf = NULL;
CRITICALSECTION csCon;

COMPORT &Console()
{
  if(pCon==NULL)
  {
    pCon = & (GetCom(__ConPort));
    pCon->install(ConsoleBaudRate);
  }
  return *pCon;
}

U16 ConBytesInRxB()
{
  return Console().BytesInRxB();
}

U8 ConReadChar()
{
  return Console().readChar();
}

void ConWriteChar(U8 Char)
{
  Console().writeChar(Char);
}

void ConPrint(const char* Msg)
{
  csCon.enter();
  Console().print(Msg);
  csCon.leave();
}

void ConPrintHex(void const * Buf,int Cnt)
{
  csCon.enter();
  Console().printHex(Buf,Cnt);
  csCon.leave();
}

void ConPrintf(const char* Fmt, ...)
{
  csCon.enter();
  if(pConBuf==NULL) pConBuf=(char*)SYS::malloc(1996);
#ifdef __BORLANDC__
  vsprintf(ConBuf,Fmt,...);
#else
  va_list args;
  va_start(args,Fmt);
  vsprintf(pConBuf,Fmt,args);
  va_end(args);
#endif
  Console().print(pConBuf);
  csCon.leave();
}

class CONSOLE_INIT
{
public:
  ~CONSOLE_INIT()
  {
    if(pConBuf!=NULL) SYS::free(pConBuf);
  }
} init_console;

#else
// !defined(__ConPort)

U16 ConBytesInRxB()
{
  return 0;
}

U8 ConReadChar()
{
  return 0;
}

void ConWriteChar(U8 /*Char*/)
{
}

void ConPrint(const char* /*Msg*/)
{
}

void ConPrintHex(void* /*Buf*/,int /*Cnt*/)
{
}

void ConPrintf(const char* /*Fmt*/, ...)
{
}

#endif


