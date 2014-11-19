#pragma once

#include <stdlib.h>
#include "SERVICE.h"
#include "WHrdware.hpp"

#if defined(__mOS7)
#include <string.h>
#include "WCRCs.hpp"
#else
#include "WFlash.h"
#endif

#if defined(__ARQ)

class PROG_SVC : public SERVICE {
  U32 Offset;
  BOOL NewPacket;
#ifdef __mOS7
  U32 Addr;
  FILE_DATA fd;
  void writefd(BOOL WithName)
  {
    FILE_DATA fd = this->fd;
    if(!WithName) memset(fd.fname,0xFF,sizeof(fd.fname));
    SYS::FlashWriteBlock(U32toFP(Addr-sizeof(fd)),&fd,sizeof(fd),TRUE);
  }
#endif
public:
  PROG_SVC();
  BOOL HaveDataToTransmit(U8 /*To*/);
  int getDataToTransmit(U8 /*To*/, void* Data,int MaxSize);
  void receiveData(U8 /*From*/, const void* Data,int Size);
};

PROG_SVC::PROG_SVC():SERVICE(3){}

BOOL PROG_SVC::HaveDataToTransmit(U8 /*To*/){
  return NewPacket;
}

#pragma argsused
int PROG_SVC::getDataToTransmit(U8 /*To*/,void* Data,int /*MaxSize*/){
  NewPacket=FALSE;
  *(U32*)Data=Offset;
  return 4;
}

#define __EnableFlashWrite

void PROG_SVC::receiveData(U8 /*From*/,const void* Data,int Size){
  U32 Ofs=*(U32*)Data;
  NewPacket=TRUE;
#ifdef __EnableFlashWrite
  if(Ofs==0)
  { // start loading
#ifdef __mOS7
    Addr = FPtoU32(SYS::FlashGetFreePosition());
    ConPrintf("\n\r*** LOADING STARTED (Addr=%05lX)",Addr);
    Addr += sizeof(FILE_DATA);
    memset(&fd,0xFF,sizeof(fd));
    fd.mark = 0x7188;
    fd.addr = (char*)U32toFP(Addr);
    fd.size = 0xFFFF;
    writefd(FALSE);
#else
    ConPrint("\n\r*** LOADING STARTED");
    SYS::WDT_Refresh();
    SYS::FlashErase(0xA000);
    SYS::WDT_Refresh();
#endif
  }
#endif
  Size-=4;
  if(Size!=0)
  {
#ifdef __EnableFlashWrite
    U8* NewData=(U8*)Data+4;
  #ifdef __mOS7
    SYS::FlashWriteBlock(U32toFP(Addr+Ofs), NewData, Size, TRUE);
  #else
    SYS::FlashWriteBlock(U32toFP(0xA0000+Ofs),NewData,Size,FALSE);
  #endif
#endif
    Offset=Ofs+Size;
    if((Ofs>>11)!=(Offset>>11))
      ConPrintf("\n\r*** LOADED %03uKiB",U16(Offset>>10));
  }
  else
  { // loading complete
    ConPrintf("\n\r*** LOADING COMPLETE %u byte(s)",(U16)Offset);
#ifdef __EnableFlashWrite
  #ifdef __mOS7
    memset(&fd,0,sizeof(fd));
    fd.mark = 0x7188;
    fd.size = Offset;
    fd.addr = (char*)U32toFP(Addr);
    fd.CRC = CRC16_get(U32toFP(Addr),fd.size,CRC16_OS7FS);
    strncpy((char*)&fd.fname, "w.exe", sizeof(fd.fname));
    writefd(TRUE); // write CRC, size and file name to FILE_DATA
  #else
    SYS::setRealtime(TRUE);
    SYS::WDT_Refresh();
    SYS::FlashErase(0x9000);
    SYS::WDT_Refresh();
    SYS::FlashWriteBlock(U32toFP(0x90000),U32toFP(0xA0000),0x10000,FALSE);
    SYS::WDT_Refresh();
    SYS::FlashErase(0xA000);
  #endif
    SYS::reset(TRUE);
#endif
  }
}

#else

#define PROG_CANCEL    -1L
#define PROG_FILEINFO  -2L
#define PROG_ERRNAME   -3L
#define PROG_ERRSIZE   -4L
#define PROG_ERRWRITE  -5L
#define PROG_SOFTRESET -6L

class PROG_SVC : public SERVICE {
  U32 Addr;
  S32 Offset;
  FILE_DATA fd;
  TIMEOUTOBJ tout;
  U8 TOCnt;
protected:

#if defined(__mOS7)
void writefd(BOOL WithName)
{
  FILE_DATA fd = this->fd;
  if(!WithName) memset(fd.fname,0xFF,sizeof(fd.fname));
  SYS::FlashWriteBlock(U32toFP(Addr-sizeof(fd)),&fd,sizeof(fd),TRUE);
}
#endif

public:

PROG_SVC():SERVICE(3)
{
  TOCnt=0;
  Offset=0;
  tout.stop();
}

#pragma argsused
BOOL HaveDataToTransmit(U8 /*To*/)
{
  return tout.IsSignaled();
}

#pragma argsused
int getDataToTransmit(U8 /*To*/, void* Data,int /*MaxSize*/)
{
  if(TOCnt++<10)
    tout.start(toTypeSec | 15);
  else
  {
    tout.start(toTypeSec | 120);
    TOCnt=0;
  }
  if( Offset < (S32)fd.size )
    *(U32*)Data=Offset;
  else
  {
    if(TOCnt<3)
      *(U32*)Data=fd.size;
    else {
      fd.size=0; Offset=0;
      tout.stop();
    }
  }
  return 4;
}

#define FlashEndPos 0xF0000L

#pragma argsused
void receiveData(U8 /*From*/, const void* Data,int Size){
  tout.setSignaled();
  TOCnt=0;
  //
  S32 Ofs=*(S32*)Data;
  if(Ofs<0)
  { // some special features
    switch(Ofs)
    {
    case PROG_FILEINFO: // packet with info about file (download invitation)
      {
        Offset=0;
        FILE_DATA *pFD = (FILE_DATA*)((U8*)Data+4);
      #ifndef __mOS7
        if( strnicmp((char*)pFD->fname,"rom-disk.img",12) != 0 ) {
            fd.size = PROG_ERRNAME;
            return;
        }
      #endif
        U32 ABeg = FPtoU32( SYS::FlashGetFreePosition() );
      #if defined(__mOS7)
        U32 FlashFreeSize = FlashEndPos - ABeg;
      #elif defined(__7188)
        // only 1/2 of flash can be used for downloading new rom-disk
        U32 FlashFreeSize = ((FlashEndPos - ABeg) >> 1) & 0xF0000L; // 64K granulation
      #endif
        if( pFD->size > FlashFreeSize ) {
          fd.size = PROG_ERRSIZE;
          return;
        }
        fd = *pFD;
      #if defined(__mOS7)
        Addr = ABeg + sizeof(FILE_DATA);
        fd.addr = (char*)U32toFP(Addr);
        writefd(FALSE);
      #elif defined(__7188)
        if( *(U16*)U32toFP(ABeg) == 0x28EB ) // check for ROM-DISK signature
          Addr = ABeg + ((fd.size+0xFFFFL) & 0xF0000L);
        else
          Addr = ABeg;
      #endif
        ConPrintf("\n\rLOADING FILE %.12s [%ld byte(s) to %05lX]\n\r",fd.fname,fd.size,Addr);
      }
      break;
    case PROG_CANCEL:
      if(Offset < fd.size)
      {
        fd.size = -1l;
        ConPrint("\n\rLOADING CANCELED\n\r");
      }
      else
        tout.stop();
      break;
    case PROG_SOFTRESET:
      SYS::reset(TRUE); // AAAA!!!! :-)
    }
  }
  else if ( (S32)fd.size > 0 )
  {
    if(Offset!=Ofs)
      return; // Not requested part of a file is received
  #if !defined(__mOS7)
    if( !Ofs )
      ((U8*)Data)[4] = 0xFF; // mask 1st byte of rom-disk image (LSB of signature 0x28EB)
  #endif
    Size-=4;
    if((Ofs>>11)!=((Ofs+Size)>>11))
      ConPrintf("\n\r*** LOAD %03d/%03d K ",U16(Ofs>>10),U16(fd.size>>10));
    U32 ABeg=Addr+Ofs, AEnd=ABeg+Size-1;
    SYS::FlashWriteBlock( U32toFP(ABeg), (U8*)Data+4, Size, TRUE );
    Ofs+=Size;
    if(Ofs>=fd.size)
    {
    #if defined(__mOS7)
      fd.CRC = CRC16_get(U32toFP(Addr),fd.size,CRC16_OS7FS);
      writefd(TRUE); // write CRC and file name to FILE_DATA
    #elif defined(__7188)
      // move received rom-disk to it's natural location
      void *pBeg = SYS::FlashGetFreePosition();
      void *pNew = U32toFP(Addr);
      void *pOld = (pBeg==pNew) ? U32toFP(Addr+((fd.size+0xFFFFL) & 0xF0000)) : pBeg;
      SYS::setRealtime(TRUE);
      SYS::FlashWrite( pNew, 0xEB ); // restore 1st (LSB) byte of signature 0x28EB
      SYS::FlashWrite( pOld, 0x00 ); // mask old rom-disk
//      SYS::FlashWriteBlock( pDst, U32toFP(Addr), fd.size);
//      SYS::FlashWrite(U32toFP(Addr),0); // "delete" temporary rom-disk
      SYS::setRealtime(FALSE);
    #endif
      ConPrint("\n\rLOADING COMPLETE");
      Ofs=fd.size;
    }
    Offset=Ofs;
  }
}

};

#endif
