#pragma once

#if defined(__7188X)

#define FlashBase      0xC000
#define AUTOSELECT_CMD 0x90
#define PROGRAM_CMD    0xA0
#define ERASE_CMD      0x80
#define READ_MODE      0xF0
#define ERASE_CHIP     0x10
#define ERASE_SECTOR   0x30
#define DONE_STATUS    0x80
#define TOGGLE_STATUS  0x40
#define TIMEOUT_STATUS 0x20
#define ERASE_TIMER    0x08

U16 FlashId, FlashSize, FlashStartSector;
U8 FlashProtectMode;

void FlashUnlock()
{
  Mem(FlashBase,0x5555)=0xAA;
  Mem(FlashBase,0x2AAA)=0x55;
}

void FlashCmd(U8 Cmd)
{
  Mem(FlashBase,0x5555)=Cmd;
}

void __ReadFlashInfo()
{
  static BOOL flashDetected=FALSE;
  if(flashDetected) return;
  SYS::cli();
  FlashUnlock();
  FlashCmd(AUTOSELECT_CMD);
  FlashId=MemW(FlashBase,0);
  FlashProtectMode=0;
  if (HiByte(FlashId)==0xB0){
    if(Mem(0xC000,2)==1) FlashProtectMode=FlashProtectMode | 1;
    if(Mem(0xD000,2)==1) FlashProtectMode=FlashProtectMode | 2;
    if(Mem(0xE000,2)==1) FlashProtectMode=FlashProtectMode | 4;
    if(Mem(0xF000,2)==1) FlashProtectMode=FlashProtectMode | 8;
    if(Mem(0xF800,2)==1) FlashProtectMode=FlashProtectMode | 16;
    if(Mem(0xFA00,2)==1) FlashProtectMode=FlashProtectMode | 32;
    if(Mem(0xFC00,2)==1) FlashProtectMode=FlashProtectMode | 64;
    FlashSize=256;
    FlashStartSector=0xC000;
  }
  else if (HiByte(FlashId)==0xA4) {
    if(Mem(0x8000,2)==1) FlashProtectMode=FlashProtectMode | 1;
    if(Mem(0x9000,2)==1) FlashProtectMode=FlashProtectMode | 2;
    if(Mem(0xA000,2)==1) FlashProtectMode=FlashProtectMode | 4;
    if(Mem(0xB000,2)==1) FlashProtectMode=FlashProtectMode | 8;
    if(Mem(0xC000,2)==1) FlashProtectMode=FlashProtectMode | 16;
    if(Mem(0xD000,2)==1) FlashProtectMode=FlashProtectMode | 32;
    if(Mem(0xE000,2)==1) FlashProtectMode=FlashProtectMode | 64;
    if(Mem(0xF000,2)==1) FlashProtectMode=FlashProtectMode | 128;
    FlashSize=512;
    FlashStartSector=0x8000;
  }
  else {
    FlashSize=0;
    FlashStartSector=0xFFFF;
  }
  FlashCmd(READ_MODE);
  SYS::sti();
  flashDetected=TRUE;
}

BOOL __waitFlashOp(void* Addr){
  U16 CurData=*(U8*)Addr;
  U16 PreData;
  U32 count=1000000;
  BOOL Res=FALSE;
  do{
    SYS::WDT_Refresh();
    PreData=CurData;
    CurData=*(U8*)Addr;
    if ( ((PreData ^ CurData) & TOGGLE_STATUS) != TOGGLE_STATUS ) 
      return TRUE;
    else if ( (PreData & TIMEOUT_STATUS) == TIMEOUT_STATUS ) 
      return FALSE;
  }while(--count);
  return FALSE;
};

//*
BOOL SYS::FlashWrite(void *Dst, U8 Data){
  __ReadFlashInfo();
  BOOL Res=FALSE;
  if (FlashStartSector<=FP_SEG(Dst) && FP_SEG(Dst)<0xF000) {
    SYS::cli();
    FlashUnlock();
    FlashCmd(PROGRAM_CMD);
    *(U8*)Dst = Data;
    Res=__waitFlashOp(Dst);
    FlashCmd(READ_MODE);
    SYS::sti();
  }
  return Res;
}

BOOL SYS::FlashErase(U16 Seg){
  __ReadFlashInfo();
  BOOL Res=FALSE;
  if (FlashStartSector<=Seg && Seg<0xF000) {
    SYS::cli();
    FlashUnlock();
    FlashCmd(ERASE_CMD);
    FlashUnlock();
    Mem(Seg,0)=ERASE_SECTOR;
    Res=__waitFlashOp(MK_FP(Seg,0));
    FlashCmd(READ_MODE);
    SYS::sti();
  }
  return Res;
}

/*/

BOOL SYS::FlashWrite(void *Dst,U8 Data)
{
  return ::FlashWrite( FP_SEG(Dst), FP_OFF(Dst), Data )==0;
}

BOOL SYS::FlashErase(U16 Seg)
{
    SYS::WDT_Refresh();
    BOOL r = ::FlashErase(Seg)==0;
    SYS::WDT_Refresh();
    return r;
}

//*/

void* SYS::FlashGetFreePosition()
{
  __ReadFlashInfo();
  FILE_DATA *pFD = (FILE_DATA*)MK_FP(FlashStartSector,0);
  while(pFD->mark==0x7188)
    pFD=(FILE_DATA*)U32toFP( FPtoU32(pFD) + sizeof(FILE_DATA) + pFD->size );
  return (void*)pFD;
}

#elif defined(__7188)

BOOL SYS::FlashErase(U16 Seg)
{
    SYS::WDT_Refresh();
    BOOL r = ::FlashErase(Seg)==0;
    SYS::WDT_Refresh();
    return r;
}

BOOL SYS::FlashWrite(void *Dst,U8 Data)
{
    return ::FlashWrite( FP_SEG(Dst), FP_OFF(Dst), Data )==0;
}

void* SYS::FlashGetFreePosition()
{
    switch( ::FlashReadId() )
    {
    case 0xA401:
      return MK_FP(0x9000,0); // 0x8000+0x1000
    case 0xB001:
      return MK_FP(0xD000,0); // 0xC000+0x1000
    default:
      return MK_FP(0xFFFF,0);
    }
}

#endif

BOOL SYS::FlashWriteBlock(void *Dst, const void* Src, U32 Size, BOOL AutoErase)
{
  U32 iSrc = FPtoU32(Src), iDst = FPtoU32(Dst);
  U16 SectorNum = ((U16)iDst == 0) ? 0 : hi(iDst); // can set SectorNum=0
  for(; Size>0; Size--, iSrc++, iDst++)
  {
  #ifdef __7188      
    if( (U8)Size & 0x7F == 0 ) // after every 128 bytes writing
      SYS::WDT_Refresh(); // reset watchdog 
  #endif
    if(AutoErase && SectorNum != hi(iDst))
    { // if new 64K-sector of flash begins, erase it
      SectorNum = hi(iDst);
      if( ! SYS::FlashErase(SectorNum<<12) )
        return FALSE;
    }
    if( ! SYS::FlashWrite( U32toFP(iDst), *(U8*)U32toFP(iSrc) ) )
    {
      ConPrintf(" [FlashWriteBlock ERROR (%ld byte(s) left)] ",Size);
      return FALSE;
    }
  }
  return TRUE;
}

