//file WKERN.CPP
#include "WKern.hpp"
#include <string.h>
#include "WComms.hpp"
#ifdef __BORLANDC__
  #include "Wint64.h"
#endif
#include "WHrdware.hpp"

//***
_BDLI *Threads=NULL;
THREAD *CurThread=NULL;
THREAD MainThread;

//*** timeouts lists
_BDLI *TOs1=NULL, *TOs2=NULL;

TIME SYS::SystemTime;
TIME SYS::NetTimeOffset=0;
BOOL SYS::TimeOk=FALSE;

//*** used during new thread initialization and switching
THREAD* NewThread;
#ifdef __DbgThd
U16 DbgThd1,DbgThd2;
#endif

//*** other
BOOL SystemStopped;
BOOL HardwareTimerIntr=TRUE;
volatile U8 __IntrLockCnt;

//*** Idle & performance counters
U16 IdleMsCnt;
U32 IdleTicks;
#ifdef __UsePerfCounters
  U16 __StartTicks,__StopTicks;
  U32 TimerISRTicks;
#endif

CRITICALSECTION MemMngr;

#ifdef __USESTDMEMMAN
#include <malloc.h>
EVENT NeedSafetyPool;
U16 Heap16Avail=0,Heap16Used;
#endif

// timer 1 storage
U32 OldIntVect12;

#include "WConsole.h"

#if defined(__WATCOMC__)

#define _SS _RegSS()
#define _SP _RegSP()
#define _BP _RegBP()

extern inline U16 _RegSS(void);
#pragma aux _RegSS = "mov ax,ss" value [ax];
extern inline U16 _RegSP(void);
#pragma aux _RegSP = value [sp] modify exact [];
extern inline U16 _RegBP(void);
#pragma aux _RegBP = value [bp] modify exact [];

extern inline void changeStack(U16 Seg,U16 Off,U16 Base);
#pragma aux changeStack = \
  "mov  ss,ax" \
  "mov  sp,cx" \
  "mov  bp,dx" \
  parm  [ax] [cx] [dx];

#elif defined(__BORLANDC__)

// mov ss,ax(Seg); mov sp,cx(Off); mov bp,dx(Base)
#define changeStack(Seg,Off,Base){\
  _AX=Seg; _CX=Off; _DX=Base; \
  _asm{ mov ss,ax; mov sp,cx; mov bp,dx } }

#endif

inline void far * normalizePtr(void far * ptr){  // far pointer normalization
#ifdef __BORLANDC__ // because a bug in "BC++ 3.1"'s codegenerator :(
  register U16 Seg = FP_SEG(ptr);
  return MK_FP( Seg+(FP_OFF(ptr)>>4), FP_OFF(ptr) & 0x000F );
#else
  return MK_FP( FP_SEG(ptr)+(FP_OFF(ptr)>>4), FP_OFF(ptr) & 0x000F );
#endif
}

/*
void printThreads(){
  _BDLI *I=Threads;
  while(I!=NULL) {
    (*(I->ThdPtr()))->printDebugInfo();
    I=I->Next;
  }
}
//*/

void* cdecl operator new(size_t size){
  void* mem=SYS::malloc(size);
  if(mem) memset(mem,0,size);
  return mem;
}

void cdecl operator delete(void* obj){
  SYS::free(obj);
}

#ifdef __DebugThreadState
void setMainThreadState(U8 State){
  SYS::cli();
  WriteNVRAM(MainThread.StateAddr,State);
  SYS::sti();
}
void setCurrThreadState(U8 State){
  SYS::cli();
  WriteNVRAM(CurThread->StateAddr,State);
//  Show5DigitLed(CurThread->StateAddr,State);
  SYS::sti();
}
#endif // __DebugThreadState

U8 hex_to_ascii[17]="0123456789ABCDEF";

U32 _fast FromHexStr(const U8* Hex, int Digits)
{
  U32 Res=0;
  for(int i=0; i<Digits; i++) Res=Res<<4 | ascii_to_hex(Hex[i]);
  return Res;
}

U32 _fast FromDecStr(const U8* Dec, int Digits)
{
  U32 Res=0;
  for(int i=0; i<Digits; i++) Res=Res*10 + ascii_to_hex(Dec[i]);
  return Res;
}

//******************** class _BDLI

void _fast _BDLI::rem(){
  if(Next!=NULL) Next->Prev=Prev;
  *Prev=Next;
  Next=NULL; Prev=NULL;
}
void _fast _BDLI::ins(_BDLI*& Head){
  if(Head) Head->Prev=&Next;
  Prev=&Head;
  Next=Head;
  Head=this;
}
void _BDLI::remove(){
  if(Ref!=NULL) {
    rem();
    Ref=NULL;
  }
}
void _BDLI::add(_BDLI*& Head,void* Ref){
  ins(Head);
  this->Ref=Ref;
}

//******************** class OBJECT

OBJECT::~OBJECT(){}

void resumeAll(_BDLI *_TN) {
  _BDLI *_Tmp=_TN;
  while(_Tmp){
    _BDLI *Next=_Tmp->Next;
    (*(_Tmp->TOPtr()))->setReady();
    _Tmp=Next;
  }
}

//******************** class SYS

void _fast SYS::TimerProc(){
    disable_sti();
    static BOOL Hard;
    Hard = HardwareTimerIntr;
#ifdef __UsePerfCounters
    //if(Hard)startSysTicksCount();
#endif
    BOOL DoThSw=!Hard;
    if(Hard)
    {
        //***** Period = SysTimerInterval ms
        SystemTime+=SysTimerInterval;
        U16 SysT=(U16)SystemTime;
        // millisecond-scale timeouts
        _BDLI *Tmp=TOs1;
        while(Tmp)
        {
            TIMEOUTOBJ* pto = *(Tmp->TOPtr());
            if(pto->Timeout != SysT)
                break;
            Tmp=Tmp->Next;
            pto->setReady();
            DoThSw=TRUE;
        }
        if((SysT & 0x3)==0)
        {
            DoThSw=TRUE;
            if((SysT & 0x3FF)==0)
            {
                // second-scale timeouts
                SysT=(U16)((U32)SystemTime>>10);
                _BDLI *Tmp=TOs2;
                while(Tmp)
                {
                    TIMEOUTOBJ* pto = *(Tmp->TOPtr());
                    if(pto->Timeout != SysT)
                        break;
                    Tmp=Tmp->Next;
                    pto->setReady();
                }
            }
        }
    }
    else if(NewThread!=NULL)
    {
        memcpy(MK_FP(NewThread->SavedSS,0),MK_FP(_SS,0),NewThread->StackSize);
        NewThread->SavedBP=_BP;
        NewThread->SavedSP=_SP;
        NewThread=NULL;
    }
    HardwareTimerIntr=TRUE;
#ifdef __UsePerfCounters
    //if(Hard)stopSysTicksCount(TimerISRTicks);
#endif
    static BOOL InProcess=FALSE;
    if(DoThSw && !InProcess)
    {
#ifdef __UsePerfCounters
        //if(Hard)startSysTicksCount();
#endif
        InProcess=TRUE;
        // if CurThread is not active "realtime" thread, then ...
        if(!(CurThread->Realtime && *(CurThread->SysRef.ThdPtr())) )
        {
            // ... do switching to next thread
            THREAD *NewCur;
            _BDLI *Tmp=CurThread->SysRef.Next;
            if(Tmp!=NULL) NewCur = *(Tmp->ThdPtr());
            else // no active threads.
            {
#ifdef __UsePerfCounters
//                if(Hard)stopSysTicksCount(TimerISRTicks);
#endif
                TIME IdleStart=SystemTime;
    //        U16 BegTicks=inpw(TMR_T1CNT);
                // Let's have a rest :)
                enable_sti();
                while(Threads==NULL)
                  _asm{
                    sti
                    hlt
                    cli
                  }
                disable_sti();
    //        U16 EndTicks=inpw(TMR_T1CNT);
    //        if(EndTicks<BegTicks) IdleTicks+=SysTimerMaxcount-BegTicks
                IdleMsCnt += (U16)(SystemTime-IdleStart);
#ifdef __UsePerfCounters
//                if(Hard)startSysTicksCount();
#endif
                NewCur = *(Threads->ThdPtr());
            }
            if(NewCur!=CurThread)
            {
                CurThread->SavedSS=_SS; CurThread->SavedSP=_SP; CurThread->SavedBP=_BP;
                CurThread=NewCur;
#ifdef __DebugThreadState
                WriteNVRAM(0,CurThread->StateAddr);
#endif // __DebugThreadState
                NewCur->Terminated=SystemStopped;
                changeStack(NewCur->SavedSS,NewCur->SavedSP,NewCur->SavedBP);
            }
        }
        InProcess=FALSE;
#ifdef __UsePerfCounters
//        if(Hard)stopSysTicksCount(TimerISRTicks);
#endif
    }
    enable_sti();
}

void interrupt far TimerISR(void){
  if(HardwareTimerIntr) outpw(INT_EOI,INT_TMREOI);
  SYS::TimerProc();
}

void SYS::switchThread(){
  _disable();
  HardwareTimerIntr=FALSE;
  _asm{ int 0x12 };
  _enable();
}

#ifdef __DebugUseConsole

void __sysdump(const void* Data,U16 Len){
  ConPrintf("\n\r%d: ",Len);
  const U8* D=(const U8*)Data;
  for(; Len>0; Len--,D++) ConPrintf("%02X ",*D);
}

#elif defined(__DebugUseCom)

CRITICALSECTION csDbgCom;
COMPORT *__dbgCom = NULL;
char* __pDbgBuf = NULL;

void __dbgPrintf(const char* Fmt, ...)
{
  csDbgCom.enter();
  if(__dbgCom==NULL)
  {
    __dbgCom = &(GetCom(__DebugUseCom));
    __dbgCom->install(ConsoleBaudRate);
  }
  if(__pDbgBuf==NULL) __pDbgBuf=(char*)SYS::malloc(1996);
  va_list args;
  va_start(args,Fmt);
  vsnprintf(__pDbgBuf,1996,Fmt,args);
  va_end(args);
  __dbgCom->print(__pDbgBuf);
  csDbgCom.leave();
}

void __sysdump(const void* Data,U16 Len){
  __dbgPrintf("\n\r%d: ",Len);
  const U8* D=(const U8*)Data;
  for(; Len>0; Len--,D++){
    U8 d = *D;
    char *fmt;
    char tmp[2];
    if(d=='\n' || d=='\r')
    { tmp[0]=d; tmp[1]=0; fmt=tmp; }
    else fmt="%02X ";
    __dbgPrintf(fmt,d);
  }
}

#endif

void SYS::TimerOpen(){
  _disable();
  outpw(INT_TCUCON,5);
  OldIntVect12=IntVect[0x12];
  IntVect[0x12]=(U32)&TimerISR;
  // enable timer 1 interrupt, and set timer 1 mode
  outpw(TMR_T1CMPA,SysTimerMaxcount);
  outpw(TMR_T1CNT,0);
  outpw(TMR_T1CON,TC_EN|TC_INH|TC_INT|TC_CONT);
  _enable();
}

void SYS::TimerClose(){
  _disable();
  IntVect[0x12]=OldIntVect12;
  outpw(TMR_T1CON,TC_INH);
  _enable();
}

U16 SYS::GetCPUIdleMs(){
  _disable();
  U16 Res=IdleMsCnt+U16(IdleTicks/SysTicksInMs);
  IdleMsCnt=0; IdleTicks=0;
  _enable();
  return Res;
}

#ifdef __UsePerfCounters
U16 SYS::GetTimerISRMs(){
  _disable();
  U16 Res=(U16)(TimerISRTicks/SysTicksInMs);
  TimerISRTicks=0;
  _enable();
  return Res;
}
#endif

#include "WFlash.h"

#ifdef __7188X
#include "W_FSOpt.h"
#endif

void SYS::startKernel(){
#ifdef __USESTDMEMMAN
  U16 Tmp;
  exploreHeap(Heap16Used,Heap16Avail,Tmp);
#endif
  Init5DigitLed();
  Set5DigitLedIntensity(0);
#ifdef __7188X
  OS7FileSystemOptimize();
#endif
#ifdef __DebugThreadState
  printThreadsState();
#endif // __DebugThreadState
  MainThread.SysRef.Prev=&Threads;
  *(MainThread.SysRef.ThdPtr()) = &MainThread;
  MainThread.SysRef.Next=NULL;
  CurThread=&MainThread;
  Threads=&(MainThread.SysRef);
  SystemStopped=FALSE;
  SYS::TimerOpen();
  EnableWDT();
}

void SYS::stopKernel(){
    SystemStopped=TRUE;
    while(true)
    {
        if(Threads->Next==NULL && TOs1==NULL)
            break;
        sleep(250);
    }
    DisableWDT();
    SYS::TimerClose();
    DelayMs(100);
}

#ifdef __DebugThreadState
void SYS::printThreadsState(){
//*
  ConPrint("\n\rActive thread: #");
//  U8 Byte = NVRAM_Read(0);
  _disable();
  U8 Byte = ReadNVRAM(0);
  _enable();
  ConPrintHex(&Byte,1);
  ConPrint("Threads state: ");
  for(int i=1; i<=5; i++){
    ConPrintf(" #%d:",i);
//    Byte = NVRAM_Read(i);
    _disable();
    Byte = ReadNVRAM(i);
    _enable();
    ConPrintHex(&Byte,1);
    NVRAM_Write(i,0x00);
  }
//*/
}
#endif

//void SYS::setRealtime(BOOL Realtime)
//{ CurThread->Realtime=Realtime; }


void SYS::checkNetTime(TIME &Time)
{ if(Time<NetTimeOffset) Time=NetTimeOffset+Time; }

//void SYS::setNetTime(const TIME& Time)
//{
//  _disable();
//  NetTimeOffset=Time-SystemTime;
//  _enable();
//  TimeOk = TRUE;
//}

void SYS::setNetTimeOffset(const TIME& Offset)
{
  _disable();
  NetTimeOffset=Offset;
  _enable();
  TimeOk = TRUE;
}

static void _fast SYS::getSysTime(TIME& Time)
{ _disable(); Time=SystemTime; _enable(); }

static void _fast SYS::getNetTime(TIME& Time)
{ _disable(); Time=SystemTime+NetTimeOffset; _enable(); }

void SYS::dbgLed(U32 Val){
  for(int i=5; 0<i; i--,Val>>=4) Show5DigitLed(i,(int)(Val & 0xF));
}

void SYS::showDecimal(S16 Val){
  if(Val<0) {
    Show5DigitLed(1,17); // '-'
    Val = -Val;
  }
  else Show5DigitLed(1,16); // ' '
  for(int i=5; i>1; i--){
    U16 Rem;
    _asm{
      mov  ax,Val
      xor  dx,dx
      mov  bx,10
      div  bx
      mov  Val,ax
      mov  Rem,dx
    }
    if(i==3) Show5DigitLedWithDot(3,Rem);
    else Show5DigitLed(i,Rem);
  }
}

void SYS::DelayMs(U16 ms){
  ::DelayMs(ms);
}

void _fast SYS::led(char On){
  _disable();
  if (On) LedOn(); else LedOff();
  _enable();
}

#ifdef __USESTDMEMMAN
void SYS::exploreHeap(U16 &Mem16Used,U16 &Mem16Avail,U16 &Max16Avail){
#ifdef __DebugUseConsole
  ConPrintf("\n\rSYS::exploreHeap: ");
#endif
  MemMngr.enter();
#ifdef __BORLANDC__
  U32 MemU=0, MemA=coreleft(), MaxA=0;
  struct farheapinfo HI;
  HI.ptr = NULL;
  while(farheapwalk(&HI) == _HEAPOK){
    if(!HI.in_use){
      MemA+=HI.size;
      if(HI.size>MaxA)MaxA=HI.size;
    }
    else {
      MemU+=HI.size;
    }
  }
#else // __WATCOMC__
  U32 MemU=0, MemA=0, MaxA=0;
  struct _heapinfo HI;
  HI._pentry = NULL;
  while(_heapwalk(&HI) == _HEAPOK){
    if(HI._useflag != _USEDENTRY){
      MemA+=HI._size;
      if(HI._size>MaxA) MaxA=HI._size;
    }
    else {
      MemU+=HI._size;
#ifdef __DebugUseConsole
      ConPrintf("%u ",HI._size);
#endif
    }
  }
#endif
  MemMngr.leave();
//  ConPrintf("\n\rMemU=%p, MemA=%p ",MemU,MemA);
  Mem16Used  = (U16)(MemU>>4);
  Mem16Avail = (U16)(MemA>>4);
  Max16Avail = (U16)(MaxA>>4);
}

void SYS::free(void* Block){
  MemMngr.enter();
#ifdef __BORLANDC__
  Heap16Avail+=MemBlockSize16(Block);
#else // __WATCOMC__
  Heap16Avail+=_msize(Block)>>4;
#endif
  ::free(Block);
  MemMngr.leave();
}

void* _fast SYS::malloc(size_t Size){
  MemMngr.enter();
  void* tmp=::malloc(Size);
#ifdef __BORLANDC__
  Heap16Avail-=MemBlockSize16(tmp);
  if(Heap16Avail<SafetyPool16Size) NeedSafetyPool.set();
#else // __WATCOMC__
  Heap16Avail-=_msize(tmp)>>4;
#endif
  MemMngr.leave();
  return tmp;
}

void*_fast SYS::realloc(void* Block,size_t Size){
  MemMngr.enter();
#ifdef __BORLANDC__
  if(Block)Heap16Avail+=MemBlockSize16(Block);
#endif
  void* tmp=::realloc(Block,Size);
#ifdef __BORLANDC__
  Heap16Avail-=MemBlockSize16(tmp);
  if(Heap16Avail<SafetyPool16Size) NeedSafetyPool.set();
#endif
  MemMngr.leave();
  return tmp;
}

#else //! defined(__USESTDMEMMAN)

extern U16 _dos_allocmem( U16 size, U16 far *seg );
#pragma aux _dos_allocmem = \
  "mov  ah,48h" \
  "int  21h" \
  "jc   DONE" \
  "mov  word ptr es:[di], ax" \
  "xor  ax,ax" \
  "DONE:" \
  parm  [bx] [es di] \
  modify [bx] \
  value [ax];


extern U16 _dos_freemem( U16 seg );
#pragma aux _dos_freemem = \
    "mov  ah,49h" \
    "int  21h" \
    parm [es] \
    value [ax];


void SYS::free(void* Block){
  if(Block==NULL) return;
  MemMngr.enter();
  _dos_freemem(FP_SEG(Block));
  MemMngr.leave();
}

void* _fast SYS::malloc(size_t Size){
  if(Size==0) return NULL;
  MemMngr.enter();
  U16 Seg;
  if( _dos_allocmem((Size+15)>>4,&Seg) != 0 )
  {
      Seg=0;
      dbg2("\n\r!malloc(%d)",(U16)Size);
  }
  MemMngr.leave();
  return MK_FP(Seg,0);
}

void*_fast SYS::realloc(void* Block,size_t Size){
  void* tmp=SYS::malloc(Size);
  memcpy(tmp,Block,Size);
  SYS::free(Block);
  return tmp;
}

#endif //__USESTDMEMMAN

void SYS::reset(){
  _asm{
    cli
    push 0xFFFF
    push 0
    retf
  }
}

//void SYS::WDT_Enable() { EnableWDT();  }
//void SYS::WDT_Disable(){ DisableWDT(); }

#if defined(__7188)

void SYS::WDT_Refresh(){ RefreshWDT(); }

#elif defined(__7188X)

void SYS::WDT_Refresh()
{
    _asm {
        mov     dx,0xFFE6
        mov     ax,0xAAAA
        out     dx,ax //al
        not     ax
        out     dx,ax //al
        pushf
//        cli
        mov         dx,0xFF7A
        in          ax,dx
        or          ah,0x02
        out         dx,ax
        and         ah,0xFD
        out         dx,ax
        popf
    }
}

#endif

S16 SYS::NVRAM_Read(U8 Addr){
  SYS::cli();
  S16 Res=ReadNVRAM(Addr);
  SYS::sti();
  return Res;
}
BOOL SYS::NVRAM_Write(U8 Addr,U8 Data){
  SYS::cli();
  BOOL Res=WriteNVRAM(Addr,Data)==0;
  SYS::sti();
  return Res;
}

void _fast SYS::addTimeout(TIMEOUTOBJ *pto,TIMEOUT Timeout)
{
    _BDLI **TOs;
    U16 offs;
    switch(Timeout & toTypeMask)
    {
        case toTypeNext:
        {
            TIME NetTime = SystemTime + NetTimeOffset;// + 1;
            Timeout &= ~toTypeMask;
            Timeout -= NetTime % Timeout;
            SYS::disable_sti();
            TOs=&TOs1;
            offs = (U16)SystemTime;
            break;
        }
        case toTypeMs:
            Timeout &= ~toTypeMask;
            SYS::disable_sti();
            TOs=&TOs1;
            offs = (U16)SystemTime;
            break;
        case toTypeSec:
            Timeout &= ~toTypeMask;
            SYS::disable_sti();
            TOs=&TOs2;
            offs = (U16)((U32)SystemTime>>10);
            break;
        default:
            return;
    }
    //dbg2("at:%d ",Timeout);
    if(Timeout==0)
        Timeout=1;
    else if(Timeout>32767)
        Timeout=32767;
    pto->Timeout = offs+Timeout;
    _BDLI *Tmp = *TOs;
    while(true)
    {
        if(!Tmp)
            break;
        TIMEOUTOBJ* to = *(Tmp->TOPtr());
        U16 left = to->Timeout - offs;
        if(left>=Timeout)
            break;
        TOs = &(Tmp->Next);
        Tmp = Tmp->Next;
    }
    Tmp=&(pto->SysRef);
    Tmp->add(*TOs,pto);
    SYS::enable_sti();
}

void _fast SYS::addThreadTimeout(TIMEOUT Timeout){
  TIMEOUTOBJ *pto = &(CurThread->TOObj);
  addTimeout(pto,Timeout);
  CurThread->EventRef[0].add(pto->ThreadsQueue,CurThread);
}

// wait Timeout ms
void _fast SYS::sleep(TIMEOUT Timeout){
  SYS::cli();
  addThreadTimeout(Timeout);
  CurThread->EventIdx=255;
  CurThread->suspend(TRUE);
  SYS::sti();
  S(0xFE);
  switchThread();
}

// wait Timeout ms for SO
int _fast SYS::waitFor(TIMEOUT Timeout,SYNCOBJ *SO){
  int Res=0;
  SYS::cli();
  if(SO->Signaled==FALSE){
    if(Timeout>0) addThreadTimeout(Timeout);
    CurThread->EventRef[1].add(SO->ThreadsQueue,CurThread);
    CurThread->EventIdx=255;
    CurThread->suspend(TRUE);
    SYS::sti();
    S(0xFD);
    switchThread();
    SYS::cli();
    Res=CurThread->EventIdx;
  }
  else{
    if (SO->Locking) SO->Signaled=FALSE;
    Res=1;
  }
  SYS::sti();
  return Res;
}

//wait Timeout ms for any of SO[i]
int _fast SYS::waitForAny(TIMEOUT Timeout,int nCount,SYNCOBJ **SO){
  if(nCount<=0 || THREAD_MAX_EVENTS<=nCount) return 255;
  SYS::cli();
  int i=nCount;
  while(--i >= 0) if(SO[i]->Signaled) break;
  if(i<0){
    if(Timeout) addThreadTimeout(Timeout);
    for(i=0; i<nCount; i++){
      CurThread->EventRef[i+1].add(SO[i]->ThreadsQueue,CurThread);
    }
    CurThread->EventIdx=255;
    CurThread->suspend(TRUE);
    SYS::sti();
    S(0xFD);
    switchThread();
    SYS::cli();
    i=CurThread->EventIdx;
  }
  else if (SO[i]->Locking) SO[i]->Signaled=FALSE;
  SYS::sti();
  return i;
}

//****************** class THREAD

THREAD::THREAD(U16 StackSize){
#ifdef __DebugThreadState
  static U8 ThreadNumber=1;
  StateAddr=ThreadNumber++;
#endif // _DebugThreadState
  if(this!=&MainThread && StackSize>0){
    this->StackSize=StackSize;
    StackPtr=SYS::malloc(StackSize+16);
    void * SPtr = normalizePtr(StackPtr);
    memset(StackPtr,0,StackSize+16);
    SavedSS=FP_SEG(SPtr)+((FP_OFF(SPtr)>0)?1:0);
  }
  else{
    StackPtr=NULL;
    SavedSS=_SS;
  }
//*/
}

THREAD::~THREAD(){
  stop();
  if(StackPtr){
    SYS::free(StackPtr);
    StackPtr=NULL;
  }
}

void THREAD::suspend(BOOL FromWaitFor){
  SYS::cli();
  SysRef.remove();
  Suspended=Suspended||!FromWaitFor;
  SYS::sti();
}

void THREAD::stop(){
  SYS::cli();
  Terminated=TRUE;
  suspend();
  S(0xDA);
  releaseEventRefs(NULL);
  SYS::sti();
}

void THREAD::releaseEventRefs(_BDLI *Reason){
  SYS::cli();
  TOObj.SysRef.remove();
  for(int i=0; i<THREAD_MAX_EVENTS; i++){
    if(&(EventRef[i])==Reason) EventIdx=i;
    EventRef[i].remove();
  }
  SYS::sti();
}

void THREAD::resume(BOOL FromWaitFor){
  if(Suspended==FromWaitFor) return;
  SYS::cli();
  SysRef.add(Threads,this);
  Suspended=FALSE;
  SYS::sti();
}

void THREAD::awake(){
  resume(TRUE);
  releaseEventRefs(&(EventRef[0]));
}

void THREAD::run(){
  if((CurThread!=&MainThread)||SYS::sti_disabled()) return;
  static void* st;
  static U16 OldSS,OldSP,OldBP;
  MTS(0xE0);
  st=SYS::malloc(StackSize+16); // create temporary stack
  memset(st,0,StackSize+16);
  void * SPtr = normalizePtr(st);
//  ConPrintf("\n\rst=%p, SPtr=%p",st,SPtr);
  SYS::cli();
  MTS(0xE1);
  NewThread=this;
  OldSS=_SS; OldSP=_SP; OldBP=_BP; // remember main stack state
  // switch to temporary stack
  changeStack(FP_SEG(SPtr)+((FP_OFF(SPtr)>0)?1:0),StackSize-2,StackSize-2);
  MTS(0xE2);
  SYS::sti();
  SYS::sleep(1); // SYS::TimerProc will initialize new thread stack
  if(CurThread!=&MainThread){
    // we are in new thread
    //*** WARNING!!! "this" value is not available!!!
    // MUST use "CurThread" instead
    MCS(0xE3);
    MCS(0xE4);
    CurThread->execute();
    S(0xDE);
    CurThread->stop();
    S(0xDD);
    SYS::switchThread();
  }
  else{
    // we are in MainThread
    MTS(0xE3);
    SYS::cli();
    changeStack(OldSS,OldSP,OldBP); // switch back to main stack
    SYS::sti();
    Suspended=TRUE;
    Terminated=FALSE;
    MTS(0xE4);
    resume();
    MTS(0xE5);
#ifdef __DbgThd
/*
    MainThread.Realtime=TRUE;
    printDebugInfo();
    MainThread.Realtime=FALSE;
*/
//    dbg("\n\rHello from main thread!");
#endif
    SYS::free(st);
    MTS(0xE6);
  }
}

void THREAD::terminate(){
  Terminated=TRUE;
}

#ifdef __DbgThd
void THREAD::printDebugInfo(){
  ConPrintf("\n\rSS=%04X SP=%04X BP=%04X Stack dump (SS:SP):",SavedSS,SavedSP,SavedBP);
  dump(MK_FP(SavedSS,SavedSP),30);
}
#endif

//****************** class SYNCOBJ

_fast SYNCOBJ::SYNCOBJ(BOOL Signaled,BOOL Locking)
{
  this->Signaled=Signaled;
  this->Locking=Locking;
  ThreadsQueue=NULL;
}

BOOL _fast SYNCOBJ::p(int Timeout){
  SYS::cli();
  if(Signaled){
    if(Locking)Signaled=FALSE;
    SYS::sti();
    return TRUE;
  }
  SYS::sti();
  return SYS::waitFor(Timeout,this)==1;
}

void SYNCOBJ::v(){
  SYS::cli();
  Signaled=TRUE;
  _BDLI *I=ThreadsQueue;
  if(Locking){
    if(I){
      Signaled=FALSE;
      (*(I->ThdPtr()))->resume(TRUE);
      (*(I->ThdPtr()))->releaseEventRefs(I);
    }
  }
  else{
    while(I){
      _BDLI *Next=I->Next;
      (*(I->ThdPtr()))->resume(TRUE);
      (*(I->ThdPtr()))->releaseEventRefs(I);
      I=Next;
    }
  }
  SYS::sti();
}

//****************** class TIMEOUTOBJ

TIMEOUTOBJ::TIMEOUTOBJ():SYNCOBJ(TRUE,FALSE)
{
    SysRef.Ref = NULL;
}

void TIMEOUTOBJ::start(TIMEOUT Timeout)
{
    SYS::cli();
    stop();
    SYS::addTimeout(this,Timeout);
    SYS::sti();
}

void TIMEOUTOBJ::stop()
{
    SYS::cli();
    SysRef.remove();
    v();
    Signaled = FALSE;
    SYS::sti();
}

void TIMEOUTOBJ::setSignaled()
{
    SYS::cli();
    SysRef.remove();
    v();
    SYS::sti();
}

TIMEOUTOBJ::~TIMEOUTOBJ()
{
    stop();
}

const U8 MonthDaysOfYear[12]     = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const U8 MonthDaysOfLeapYear[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

inline BOOL IsLeapYear(U16 y){ return (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0)); }

#define ZeroDate 693594ul // days between 0001-01-01 and 1899-12-31 (Delphi TDateTime compatibility)
#define MSecInDay (24*3600000ul)
const U16 D1 = 365;
const U16 D4 = D1 * 4 + 1; //1461
const U16 D100 = D4 * 25 - 1; // 36524
const U32 D400 = umul(D100,4) + 1; // 146097

void SYS::DecodeDate(TIME time, U16& year, U16& month, U16& day)
{
    U32 T = (U32)(time/MSecInDay) + ZeroDate - 1;
//  Dec(T);
    //T--;
//  Y := 1;
    U16 Y=1;
//  while T >= D400 do begin Dec(T, D400); Inc(Y, 400); end;
    while(T>=D400) { T-=D400; Y+=400; }
//  DivMod(T, D100, I, D);
    U16 I, D;
    I = udivmod(T, D100, D); // U16 I = (U16)(T / D100); U16 D = (U16)(T % D100);
//  if I = 4 then begin Dec(I); Inc(D, D100); end;
    if(I==4){I--; D+=D100;}
//  Inc(Y, I * 100);
    Y+=I*100;
//  DivMod(D, D4, I, D);
    I = udivmod(D, D4, D); // I = D / D4; D = D % D4;
//  Inc(Y, I * 4);
    Y+=I*4;
//  DivMod(D, D1, I, D);
    I = udivmod(D, D1, D); // I = D / D1; D = D % D1;
//  if I = 4 then begin Dec(I); Inc(D, D1); end;
    if(I==4){I--; D+=D1;}
//  Inc(Y, I);
    Y+=I;
//  DayTable := @MonthDays[IsLeapYear(Y)];
    const U8* DayTable;
    if(IsLeapYear(Y))
        DayTable = MonthDaysOfLeapYear;
    else DayTable = MonthDaysOfYear;
//  M := 1;
    U16 M=1;
//  while True do begin
//    I := DayTable^[M];
//    if D < I then Break;
//    Dec(D, I); Inc(M);
//  end;
    while(true)
    {
        I=DayTable[M-1];
        if(D<I) break;
        D-=I; M++;
    }
//  Year := Y; Month := M; Day := D + 1;
    year = Y; month = M; day = D+1;
}

//procedure DecodeTime(Time: TDateTime; var Hour, Min, Sec, MSec: Word);
void SYS::DecodeTime(TIME time, U16& hour, U16& min, U16& sec, U16& msec)
//var
//  MinCount, MSecCount: Word;
{
    U32 frac = (U32)(time % MSecInDay);
//  DivMod(DateTimeToTimeStamp(Time).Time, 60000, MinCount, MSecCount);
    U16 MinCount, MSecCount;
    MinCount = udivmod(frac,60000u,MSecCount);
//  DivMod(MinCount, 60, Hour, Min);
    hour = udivmod(MinCount,60,min);
//  DivMod(MSecCount, 1000, Sec, MSec);
    sec = udivmod(MSecCount, 1000, msec);
}

BOOL SYS::TryEncodeTime(U16 year, U16 month, U16 day, U16 hour, U16 min, U16 sec, TIME &time)
{
    const U8 *DayTable;
    if(IsLeapYear(year))
        DayTable = MonthDaysOfLeapYear;
    else DayTable = MonthDaysOfYear;
    if(year<1 || year>9999 || month<1 || month>12 || day<1 || day>DayTable[month-1] ||
       hour>=24 || min>=60 || sec>=60)
        return FALSE;
    U16 i;
    for(i=1; i<month; i++) day+=DayTable[i-1];
    i = year - 1;
    time = (unsigned long long)(umul(i,365) + (i/4 - i/100 + i/400 + day) - ZeroDate)*MSecInDay +
        (umul((hour * 60 + min),60) + sec)*1000u;
//        (hour * 3600000ul + min*60000ul + sec*1000u);
    return TRUE;
}
