#pragma once
//file WKERN.HPP

#ifndef __WKERN_HPP

#define __WKERN_HPP

//***** Debugging switches
#define __UsePerfCounters
#define __DebugUseConsole
//#define __DebugUseCom 3
//#define __DebugThreadState
//#define __DbgThd

//***** Includes
#include <stddef.h>
#include <i86.h>
#include <conio.h>

#ifndef __WCLASSES_HPP
  #include "WClasses.hpp"
#endif
#ifndef __WAM186ES_H
  #include "WAm186ES.h"
#endif

//***** Kernel classes
class THREAD;
class EVENT;
class SYS;
class SYNCOBJ;
class TIMEOUTOBJ;
class EVENT;
class CRITICALSECTION;

//***** Constanst
#define SysTicksInMs 10000u //10001
#define SysTimerInterval 2u
#define SysThreadSwitchIntervalMask 0xFu
#define SysTimerMaxcount (SysTicksInMs*SysTimerInterval)
#define dt100MSec 100u
#define dtOneSecond 1000u
#define FALSE 0
#define TRUE 1
#define ThreadDefaultStackSize 2048

#define dbgInt(n,x) SYS::dbgLed((S32(n)<<16) | (x))
#define dbgLong(x) SYS::dbgLed(x)
#define dbgLine SYS::dbgLed(__LINE__)

#ifdef __DebugUseConsole
  #define dbg(x) ConPrint(x)
  #define dbg2(x,y) ConPrintf(x,y)
  #define dbg3(x,y,z) ConPrintf(x,y,z)
  #define dump(data,count) __sysdump(data,count)
  extern void __sysdump(const void* Data,U16 Len);
#elif defined(__DebugUseCom)
  #define dbg(x) __dbgPrintf(x)
  #define dbg2(x,y) __dbgPrintf(x,y)
  #define dbg3(x,y,z) __dbgPrintf(x,y,z)
  #define dump(data,count) __sysdump(data,count)
  extern void __dbgPrintf(const char* Fmt, ...);
  extern void __sysdump(const void* Data,U16 Len);
#else
  #define dbg(x)
  #define dbg2(x,y)
  #define dbg3(x,y,z)
  #define dump(data,count)
#endif

#ifdef __DebugThreadState
    #define S(x) setCurrThreadState(x);
    #define MTS(x) setMainThreadState(x);
    extern U8 LastThreadStates[8];
    void setMainThreadState(U8 State);
    void setCurrThreadState(U8 State);
#else
    #define S(x)
    #define MTS(x)
#endif

#include "WInlines.h"

extern volatile U8 __IntrLockCnt;
extern U16 Heap16Avail,Heap16Used;
extern U8 hex_to_ascii[17];

#ifdef __UsePerfCounters
  extern U16 __StartTicks;//,__StopTicks;
  #define startSysTicksCount() SYS::startCounting()
  #define stopSysTicksCount(_Cntr) SYS::stopCounting(_Cntr)
  #define ASMPERFINC(_Cnt) inc _Cnt
  #define PERFINC(_Cnt) _Cnt++
#else
  #define startSysTicksCount()
  #define stopSysTicksCount(_Cntr)
  #define ASMPERFINC(_Cnt)
  #define PERFINC(_Cnt)
#endif

#ifndef __FILE_DATA__
#define __FILE_DATA__
typedef struct  {
  unsigned mark;   /* 0x7188 -> is file */
  unsigned char fname[12];
  unsigned char year;
  unsigned char month;
  unsigned char day;
  unsigned char hour;
  unsigned char minute;
  unsigned char sec;
  unsigned long size;
  char far *addr;
  unsigned CRC;
  unsigned CRC32;
} FILE_DATA;
#endif

U32 _fast FromHexStr(const U8* Hex, int Digits);
U32 _fast FromDecStr(const U8* Dec, int Digits);

void* cdecl operator new(size_t size);
void cdecl operator delete(void* obj);

//
class _BDLI { // BiDirectional List Item
  friend class SYS;
public:
  _BDLI **Prev;
  void *Ref;
  _BDLI *Next;
  void remove();
  void add(_BDLI*& Head,void* Ref);
  inline THREAD** ThdPtr(){return (THREAD**)&Ref;}
  inline TIMEOUTOBJ** TOPtr(){return (TIMEOUTOBJ**)&Ref;}
protected:
  void _fast rem();
  void _fast ins(_BDLI*& Head);
  void _fast mov(_BDLI*& Head) { rem(); ins(Head); }
};

class SYS {
public:
  static TIME SystemTime;
  static TIME NetTimeOffset;
  static BOOL TimeOk;
  inline static void cli(){_disable(); __IntrLockCnt++; }
  inline static void sti(){ if(--__IntrLockCnt==0) _enable(); }
  inline static void enable_sti(){__IntrLockCnt=0;}
  inline static void disable_sti(){__IntrLockCnt=1;}
  inline static BOOL sti_disabled(){ return __IntrLockCnt; }
  inline static U8 readThreadSafe(U8& x)
  { cli(); U8 tmp=x; sti(); return tmp; }
  inline static void writeThreadSafe(U8& x, U8 value)
  { cli(); x=value; sti(); }
//  inline static void getSysTime(TIME& Time)
//  { _disable(); Time=SystemTime; _enable(); }
//  inline static void getNetTime(TIME& Time)
//  { _disable(); Time=SystemTime+NetTimeOffset; _enable(); }
#ifdef __UsePerfCounters
    inline static void startCounting()
    {
        __StartTicks=inpw(TMR_T1CNT);
    }

    inline static void stopCounting(U32& Counter)
    {
        S16 delta = inpw(TMR_T1CNT) - __StartTicks;
        if(delta<0)
            Counter+=SysTimerMaxcount+delta;
        else
            Counter+=delta;
    }
#endif
public:
  static void       dbgLed(U32 Val);
  static void       showDecimal(S16 Val);
  static void       free(void* Block);
  static void*_fast malloc(size_t Size);
  static void*_fast realloc(void* Block,size_t Size);
#ifdef __USESTDMEMMAN
  static void       exploreHeap(U16 &Mem16Used,U16 &Mem16Avail,U16 &Max16Avail);
#endif
  static void _fast led(char On);
//  static U16        ModBusCRC16(void* Data,U16 DataLen);
//  static U16        pppfcs16(const void *data, int len);
//  static BOOL       FCS_is_OK(const void *Packet, int len);
//  static void       FCS_write(void *Packet, int len);
  static void _fast getSysTime(TIME& Time);
  static void _fast getNetTime(TIME& Time);
  static void       reset(BOOL needStop);
//  static void       setNetTime(const TIME& Time);
  static void       setNetTimeOffset(const TIME& Offset);
  static void       checkNetTime(TIME &Time);
  static void _fast sleep(TIMEOUT Timeout);
  static void _fast switchThread();
  static int  _fast waitFor(TIMEOUT Timeout,SYNCOBJ *SO);
  static int  _fast waitForAny(TIMEOUT Timeout,int nCount,SYNCOBJ **SO);
  static BOOL       FlashErase(U16 Seg);
  static BOOL       FlashWrite(void *Dst,U8 Data);
  static BOOL       FlashWriteBlock(void *Dst, void const* Src, U32 Size, BOOL AutoErase);
  static void*      FlashGetFreePosition();
  static FILE_DATA* FileInfoByName(char const* name);
  static void       FileWrite(char const* name, void const* data, U16 size);
  static S16        NVRAM_Read(U8 Addr);
  static BOOL       NVRAM_Write(U8 Addr,U8 Data);
//  static S16        EEP_Read(U8 Blk,U8 Addr);
//  static BOOL       EEP_Write(U8 Blk,U8 Addr,U8 Data);
  static void       WDT_Refresh();
  static void       setRealtime(BOOL Realtime);
  static U16        GetCPUIdleMs();
  static void       DelayMs(U16 ms);
  static void       DecodeDate(TIME time, U16& year, U16& month, U16& day);
  static void       DecodeTime(TIME time, U16& hour, U16& min, U16& sec, U16& msec);
  static BOOL       TryEncodeTime(U16 year, U16 month, U16 day, U16 hour, U16 min, U16 sec, TIME &time);
#ifdef __UsePerfCounters
  static U16        GetTimerISRMs();
#else
  inline static U16 GetTimerISRMs(){return 0;}
#endif
  static void       printThreadsState();
  static void       startKernel();
  static bool       Stopping();
  static void       stopKernel();
private:
  static void       TimerOpen();
  static void       TimerClose();
  static void _fast addTimeout(TIMEOUTOBJ* pto,TIMEOUT Timeout);
  static void _fast addThreadTimeout(TIMEOUT Timeout);
  static void _fast TimerProc(BOOL Hw);
  friend void interrupt far TimerISR(void);
  friend class TIMEOUTOBJ;
};

#define THREAD_MAX_EVENTS 4

class SYNCOBJ {
  friend class SYS;
protected:
  _BDLI *ThreadsQueue;
  BOOL Signaled;
  BOOL Locking;
protected:
  inline SYNCOBJ(){};
  _fast SYNCOBJ(BOOL Signaled,BOOL Locking);
  BOOL _fast p(int Timeout=0);
  void v();
public:
  inline BOOL IsSignaled(){return Signaled;}
};

class TIMEOUTOBJ : public SYNCOBJ {
  friend class SYS;
  friend class THREAD;
  friend void resumeAll(_BDLI *_TN);
protected:
  _BDLI SysRef; // "timeouts" list item
  TIMEOUT Timeout;
  inline void setReady() { SysRef.remove(); v(); }
public:
  TIMEOUTOBJ();
  ~TIMEOUTOBJ();
  void start(TIMEOUT Timeout);
  void stop();
  void setSignaled();
  inline BOOL wait(TIMEOUT Timeout) { return p(Timeout); }
};

class THREAD {
  friend class SYS;
  friend class SYNCOBJ;
protected:
  U16 SavedSP,SavedSS,SavedBP;
  _BDLI SysRef; // "active threads" list item
  _BDLI EventRef[THREAD_MAX_EVENTS];
  TIMEOUTOBJ TOObj;
  void *StackPtr;
  U16 StackSize;
  int EventIdx;
  U8 Suspended;
  U8 Terminated;
  BOOL Realtime;
  void       releaseEventRefs(_BDLI *Reason);
  void       awake();
public:
#ifdef __DebugThreadState
  U8 StateAddr;
#endif //__DebugThreadState
  THREAD(U16 StackSize=ThreadDefaultStackSize);
  virtual ~THREAD();
  void       resume(BOOL FromWaitFor=FALSE);
  void       run();
  void       suspend(BOOL FromWaitFor=FALSE);
  void       stop();
  void       terminate();
  void virtual execute(){}
#ifdef __DbgThd
  void       printDebugInfo();
#endif
};

class EVENT : public SYNCOBJ {
public:
  inline BOOL waitFor(int Timeout=0){ return p(Timeout); }
  inline void set(){ v(); }
  inline void reset(){ SYS::writeThreadSafe(Signaled,FALSE); }
  inline void pulse(){ SYS::cli(); v(); Signaled=FALSE; SYS::sti(); }
};

class CRITICALSECTION : public SYNCOBJ {
public:
  inline CRITICALSECTION():SYNCOBJ(TRUE,TRUE){};
  inline void enter(){ p(); };
  inline void leave(){ v(); };
  inline BOOL tryEnter(){
    SYS::cli(); BOOL Res=Signaled; if(Res) p(); SYS::sti(); return Res;
  };
  inline BOOL tryEnter(int Timeout){ return p(Timeout)!=0; }
};

inline void sleep(TIMEOUT Timeout) { SYS::sleep(Timeout); }

void ConPrint(const char* Msg);
void ConPrintHex(void const * Buf,int Cnt);
void ConPrintf(const char* Fmt, ...);
U16 ConBytesInRxB();
U8 ConReadChar();
void ConWriteChar(U8 Char);

#endif // __WKERN_HPP
