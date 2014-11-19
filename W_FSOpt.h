#pragma once

struct FILEINFO {
  U32 Addr;
  FILE_DATA fd;
};

void printFILEINFOs(FILEINFO *fi, int n)
{
  for(int i=0; i<n; i++)
  {
    void *sa = ((FILE_DATA*)U32toFP(fi[i].Addr))->addr;
    FILE_DATA *fd = &(fi[i].fd);
    ConPrintf(
      "\n\r#%d : Src=%P Dst=%P Size=%05lX CRC=%04X Name=%.12s",
      i, sa, fd->addr, fd->size, fd->CRC, fd->fname
    );
  }
}

void ImmediateCharMsg(char c)
{
  ConWriteChar(c);
  SYS::DelayMs(5);
}

#define nMAXFILES 512
#define _32K 0x8000

void OS7FileSystemOptimize()
{
  __ReadFlashInfo();
//  ConPrintf("FLASH INFO: ID=0x%X Size=%d Start=0x%X PMode=0x%02X",FlashId,FlashSize,FlashStartSector,FlashProtectMode);
  ConPrint("\n\r*** FS checking...");
  SYS::DelayMs(100);
  FILEINFO *fi = (FILEINFO*)SYS::malloc(nMAXFILES*sizeof(FILEINFO));
  U16 nFiles = 0;
  //***** scan all files
  U32 Addr = (U32)FlashStartSector << 4;
  BOOL NeedOpt = FALSE;
  do {
    FILE_DATA *pFD = (FILE_DATA*)U32toFP(Addr);
    if( pFD->mark != 0x7188)
      break;
    if( pFD->fname[0]!=0xFF && pFD->fname[0]!=0 )
    { // do not remember files with invalid names
      // search for FILEINFO with such name
      int i;
      for(i=nFiles-1; i>=0; i--)
        if( strnicmp( (char const*)pFD->fname, (char const*)fi[i].fd.fname, 12 )==0 ) break;
      if(i<0){ // first occurrence of file with such name
        if(nFiles==nMAXFILES) break;
        i=nFiles++;
      }
      else NeedOpt = TRUE; // need to delete old versions of files
      // remember FILEINFO of last file occurrence
      fi[i].Addr = Addr;
      fi[i].fd = *pFD;
    }
    else NeedOpt = TRUE; // need to delete unnamed files
    // go to next file
    Addr += sizeof(FILE_DATA) + pFD->size;
  } while( Addr < 0xF0000ul );
  if(NeedOpt)
  {
    //***** sort FILEINFOs by Addr & determine new addresses of files
    Addr = (U32)FlashStartSector << 4;
    for(int i=0; i<nFiles; i++)
    {
      // search min. Addr
      int imin=i;
      U32 Min = fi[i].Addr;
      for(int j=nFiles-1; j>i; j-- )
      {
        if(fi[j].Addr < Min)
        {
          Min = fi[j].Addr;
          imin=j;
        }
      }
      // swap fi[i] and fi[imin], if needed
      if(imin!=i){ FILEINFO Tmp=fi[i]; fi[i]=fi[imin]; fi[imin]=Tmp; }
      fi[i].fd.addr = (char*)U32toFP(Addr + sizeof(FILE_DATA));
      Addr += sizeof(FILE_DATA) + fi[i].fd.size;
    }
  }
  printFILEINFOs(fi,nFiles);
  if(!NeedOpt)
  { // optimization isn't necessary
    ConPrint("\n\r*** FS OK.\n\r");
    SYS::free(fi);
    return;
  }
  U8 *Buf1 = (U8*)SYS::malloc(_32K);
  U8 *Buf2 = (U8*)SYS::malloc(_32K);
  U32 SrcPos = 0;
  U16 iFI = 0;
  BOOL IsFileData=TRUE;
  BOOL OkErase = TRUE, OkWrite = TRUE;
  ConPrint("\n\r");
  for(U16 sector = FlashStartSector; sector < 0xF000 && iFI<nFiles; sector+=0x1000)
  {
    ConPrintf(" %X:",sector);
    U16 Buf1Pos=0, Buf2Pos=0, BufPos = 0;
    // have some data to write
    ImmediateCharMsg('r');
    U8 *Buf = Buf1;
    // fill the Buffer
    while(1)
    {
      U32 Src;
      U32 SrcSize;
      if(IsFileData) {
        Src = FPtoU32(&(fi[iFI].fd));
        SrcSize = sizeof(FILE_DATA);
      }
      else {
        Src = fi[iFI].Addr + sizeof(FILE_DATA);
        SrcSize = fi[iFI].fd.size;
      }
      U32 Size = SrcSize - SrcPos;
      if(BufPos+Size > _32K)
        Size = _32K-BufPos;
      memcpy( Buf+BufPos, U32toFP(Src+SrcPos), (U16)Size );
      BufPos+=(U16)Size;
      SrcPos+=Size;
      if(SrcPos==SrcSize)
      { // next source
        if(!IsFileData && ++iFI==nFiles)
          break; // no more sources
        IsFileData = !IsFileData;
        SrcPos = 0;
      }
      if(BufPos==_32K){
        if(Buf == Buf1)
        {
          Buf1Pos = BufPos;
          Buf = Buf2;
          BufPos = 0;
        }
        else break;
      }
    }
    if(Buf==Buf1)
      Buf1Pos = BufPos;
    else
      Buf2Pos = BufPos;
    void *Dst1 = MK_FP(sector+0x000,0);
    void *Dst2 = MK_FP(sector+0x800,0);
    if( Buf2Pos==_32K && memcmp( Dst1,Buf1,_32K )==0 && memcmp( Dst2,Buf2,_32K)==0 )
    {
      ImmediateCharMsg('s');
      continue;
    }
    ImmediateCharMsg('e');
    // erase flash sector
    OkErase = SYS::FlashErase(sector); if(!OkErase) break;
    if(Buf1Pos!=0) {
      ImmediateCharMsg('w');
      OkWrite = SYS::FlashWriteBlock( Dst1, Buf1, Buf1Pos, FALSE ); if(!OkWrite) break;
    }
    if(Buf2Pos!=0) {
      ImmediateCharMsg('w');
      OkWrite = SYS::FlashWriteBlock( Dst2, Buf2, Buf2Pos, FALSE ); if(!OkWrite) break;
    }
  }
  if(!OkErase)
    ConPrint(" ERROR ERASING\n\r");
  else if(!OkWrite)
    ConPrintf(" ERROR WRITING\n\r");
  else
    ConPrint("\n\r*** FS compacting completed\n\r");
  SYS::free(Buf1);
  SYS::free(Buf2);
  SYS::free(fi);
}

#undef _32K
