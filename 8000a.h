/*
Last Update at 2010/04/14 [yyyy/mm/dd] By Tim Tsai, ICP DAS.
*/
#ifndef __MINIOS7__
#define __MINIOS7__

#define _I8000_

typedef unsigned int  uint;
typedef unsigned int  WORD;
typedef unsigned char uchar;
typedef unsigned char BYTE;
typedef unsigned long ulong;
typedef unsigned long DWORD;

#ifdef __TURBOC__
  #if (__TURBOC__ < 0x0300)
    #define inpw   inport
    #define outpw  outport
  #endif
#endif

#define    NoError	    0
#define    InitPinIsOpen    0
#define    InitPinIsNotopen 1
#define    QueueIsEmpty     0
#define    QueueIsNotEmpty  1
#define    PortError	   -1
#define    DataError	   -2
#define    ParityError	   -3
#define    StopError	   -4
#define    TimeOut	   -5
#define    QueueEmpty	   -6
#define    QueueOverflow   -7
#define    PosError	   -8
#define    AddrError	   -9
#define    BlockError	   -10
#define    WriteError	   -11
#define    SegmentError    -12
#define    BaudRateError   -13
#define    CheckSumError   -14
#define    ChannelError    -15
#define    BaudrateError   -16
#define    TriggerLevelError   -17
#define    DateError    -18
#define    TimeError    -19
#define	   OutOfMemory	-20
#define    TimeIsUp        1

#ifdef __cplusplus
extern "C" {
#endif

/* Fro library version, date,...*/
void cdecl InitLib(void);
void cdecl GetLibDate(char *date);
unsigned cdecl GetLibVersion(void);

/* FOR WDT */
void cdecl EnableWDT(void);
void cdecl DisableWDT(void);
void cdecl RefreshWDT(void);

/* FOR INIT* pin */
int cdecl ReadInitPin(void);

/* For SCLK pin */
void cdecl ClockHigh(void);
void cdecl ClockHighLow(void);
void cdecl ClockLow(void);

/* For LED (L1 of S-MMI)*/
void cdecl LedOn(void);
void cdecl LedOff(void);
void cdecl LedToggle(void);

/* For L1/L2/L3 on I-8000 S-MMI */
#define LED_OFF     0
#define LED_ON      1
#define LED_TOGGLE  2
void cdecl SetLedL1(int mode);
void cdecl SetLedL2(int mode);
void cdecl SetLedL3(int mode);

/* FOR 5* 7-segments LED */
extern unsigned char ShowData[19];
void pascal Show5DigitLed(int pos, int data);
void pascal Show5DigitLedWithDot(int pos, int data);
void cdecl Init5DigitLed(void);
void pascal Show5DigitLedSeg(int pos, unsigned char data);
void pascal Set5DigitLedTestMode(int mode);
void pascal Set5DigitLedIntensity(int mode);
void cdecl Disable5DigitLed(void);
void cdecl Enable5DigitLed(void);

/* For STDIO */
void cdecl Putch(int data);
void cdecl Puts(char *str);
int cdecl Getch(void);
int cdecl Gets(char *str);
int cdecl Kbhit(void);
int cdecl LineInput(char *buf,int maxlen);
void cdecl ResetScanBuffer(void);
void cdecl SetScanBuffer(unsigned char *buf,int len);
int cdecl Scanf(char *fmt, ...); /* for TC/BC only */
int cdecl Print(const char *fmt, ...);
int cdecl _Printf(const char *fmt, ...); /* for TC/BC only */
int cdecl UngetchI(int key);
int cdecl Ungetch(int key);

/* For RTC/NVRAM */
void cdecl GetTime(int *hour,int *minute,int *sec);
void cdecl GetDate(int *year,int *month,int *day);
int cdecl GetWeekDay(void);
int cdecl SetDate(int year,int month,int day);
int cdecl SetTime(int hour,int minute,int sec);
int cdecl ReadNVRAM(int addr);
int cdecl WriteNVRAM(int addr, int data);
void cdecl ReadRTC(int addr, int *data);
void cdecl WriteRTC(int addr, int data);
int cdecl _ReadRTC(int addr); //[2008/11/24] add

/* for system timeticks */
/* [2012/08/23] marked by Tim Tsai.
extern const unsigned long far *const TimeTicks;
On iPAC-8000 please use GetTimeTicks() to instead of *TimeTicks.
*/

/* For old version EEPROM functions compatible */
#define WriteEEP	EE_RandomWrite
#define ReadEEP		EE_RandomRead
#define EnableEEP	EE_WriteEnable
#define ProtectEEP	EE_WriteProtect

/* for EEPROM(24WC128)*/
void cdecl EE_WriteProtect(void);
void cdecl EE_WriteEnable(void);
int cdecl EE_RandomRead(int Block,unsigned Addr);
unsigned char cdecl EE_ReadNext(int Block);
int cdecl EE_MultiRead(int StartBlock,unsigned StartAddr,int no,void cdecl *databuf);
int cdecl EE_RandomWrite(int Block,unsigned Addr,int Data);
int cdecl EE_MultiWrite(int Block,unsigned Addr,int no,const void cdecl *src);
int cdecl EE_MultiWrite_A(int Block,unsigned Addr,unsigned no,const void cdecl *src);
int cdecl EE_MultiWrite_L(unsigned address,unsigned no,const void cdecl *src);
int cdecl EE_MultiRead_L(unsigned address,unsigned no,void cdecl *databuf);

/*[2006/05/15] Add variables for EEPROM
For 24LC16, size=2K bytes
EE_Type=16;
EE_BlockNo=8;
EE_PageSize=16;

For 24WC128, size=16K bytes
EE_Type=128;
EE_BlockNo=64;
EE_PageSize=64;

For the EEPROM functions, every block with 256 Bytes.
for EE_Type=16, total size=256*EE_BlockNo=256*8=2K bytes,
for EE_Type=128, total size=256*EE_BlockNo=256*64=16K bytes,

EE_PageSize is for the function EE_MultiWrite().The maximum write size for
the function EE_MultiWrite() is EE_PageSize. for 24lc16 it is 16 bytes,
for 24WC128 it is 64 bytes.
*/
extern int const EE_Type;
extern int const EE_BlockNo;
extern int const EE_PageSize;

/* for IP/MASK/GATEWAY/MAC */
void cdecl GetIp(unsigned char *ip);
#define GetMac	GetEid
void cdecl GetEid(unsigned char *id);
void cdecl GetMask(unsigned char *mask);
void cdecl GetGateway(unsigned char *gate);
void cdecl SetIp(unsigned char *ip);
#define SetMac	SetEid
void cdecl SetEid(unsigned char *id);
void cdecl SetMask(unsigned char *mask);
void cdecl SetGateway(unsigned char *gate);

extern int IpErrno; /*[2008/01/21] */
extern int Ip2Errno; /*[2008/01/21] */

#define IP_ERROR		1
#define MASK_ERROR		2
#define GATEWAY_ERROR	4
#define MAC_ERROR		8
/*[2008/01/21] */
void cdecl GetIp2(unsigned char *ip);
#define GetMac2	GetEid2
void cdecl GetEid2(unsigned char *id);
void cdecl GetMask2(unsigned char *mask);
void cdecl GetGateway2(unsigned char *gate);
void cdecl SetIp2(unsigned char *ip);
#define SetEid2 SetMac2
void cdecl SetEid2(unsigned char *id);
void cdecl SetMask2(unsigned char *mask);
void cdecl SetGateway2(unsigned char *gate);

/* for system */
//extern unsigned long far *IntVect;
int cdecl IsMiniOS7(void);
int cdecl Is8000(void);
int cdecl Is8000Tcp(void);
#define IsResetByPowerOff	IsResetByPowerOn
int cdecl IsResetByPowerOn(void);
int cdecl IsResetByWatchDogTimer(void);

/* for FLASH MEMORY */
int cdecl FlashReadId(void);
int cdecl FlashErase(unsigned int FlashSeg);
int cdecl FlashWrite(unsigned int seg, unsigned int offset, char data);

#define FlashRead FlashReadB
unsigned char FlashReadB(unsigned seg, unsigned offset);
unsigned FlashReadI(unsigned seg, unsigned offset);
unsigned long FlashReadL(unsigned seg, unsigned offset);
void cdecl far *_MK_FP_(unsigned s,unsigned off);

extern unsigned FlashId;
extern int FlashSize;

/* Timer functions */
int cdecl TimerOpen(void);
int cdecl TimerClose(void);
void cdecl TimerResetValue(void);
unsigned long TimerReadValue(void);
int cdecl StopWatchReset(int channel);
int cdecl StopWatchStart(int channel);
int cdecl StopWatchStop(int channel);
int cdecl StopWatchPause(int channel);
int cdecl StopWatchContinue(int channel);
int cdecl StopWatchReadValue(int channel,unsigned long *value);
int cdecl CountDownTimerStart(int channel,unsigned long count);
int cdecl CountDownTimerReadValue(int channel,unsigned long *value);
void cdecl InstallUserTimer(void cdecl (*fun)(void));
void cdecl InstallUserTimer1C(void cdecl (*fun)(void));

/* StopWatch [計時碼表] */

#ifndef _T_STOPWATCH_
#define _T_STOPWATCH_
typedef struct {
 ulong ulStart,ulPauseTime;
 uint  uMode;  /* 0: pause, 1:run(start) */
} STOPWATCH;
#endif

/* CountDown Timer[倒數計時] */
#ifndef _T_COUNTDOWNTIMER_
#define _T_COUNTDOWNTIMER_
typedef struct {
 ulong ulTime,ulStartTime,ulPauseTime;
 uint  uMode;  /* 0: pause, 1:run(start) */
} COUNTDOWNTIMER;
#endif

void cdecl T_StopWatchStart(STOPWATCH *sw);
ulong T_StopWatchGetTime(STOPWATCH *sw);
void cdecl T_StopWatchPause(STOPWATCH *sw);
void cdecl T_StopWatchContinue(STOPWATCH *sw);

void cdecl T_CountDownTimerStart(COUNTDOWNTIMER *cdt,ulong timems);
void cdecl T_CountDownTimerPause(COUNTDOWNTIMER *cdt);
void cdecl T_CountDownTimerContinue(COUNTDOWNTIMER *cdt);
int cdecl T_CountDownTimerIsTimeUp(COUNTDOWNTIMER *cdt);
ulong cdecl T_CountDownTimerGetTimeLeft(COUNTDOWNTIMER *cdt);

/* Timer functions II */
void cdecl T2_UpdateCurrentTimeTicks(void); /* every loop must call T2_UpdateCurrentTimeTicks() to get new time.*/
void cdecl T2_StopWatchStart(STOPWATCH *sw);
ulong cdecl T2_StopWatchGetTime(STOPWATCH *sw);
void cdecl T2_StopWatchPause(STOPWATCH *sw);
void cdecl T2_StopWatchContinue(STOPWATCH *sw);

void cdecl T2_CountDownTimerStart(COUNTDOWNTIMER *cdt,ulong timems);
void cdecl T2_CountDownTimerPause(COUNTDOWNTIMER *cdt);
void cdecl T2_CountDownTimerContinue(COUNTDOWNTIMER *cdt);
int cdecl T2_CountDownTimerIsTimeUp(COUNTDOWNTIMER *cdt);
ulong T2_CountDownTimerGetTimeLeft(COUNTDOWNTIMER *cdt);

void cdecl Delay(unsigned ms); /* delay unit is ms, use CPU Timer 1. */
void cdecl Delay_1(unsigned ms); /* delay unit is 0.1 ms ,use CPU Timer 1.*/
void cdecl Delay_2(unsigned ms); /* delay unit is 0.01 ms ,use CPU Timer 1.*/
void cdecl DelayMs(unsigned t);/* delay unit is ms, use system timeticks. */

/* for MiniOS7 FLASH file system */
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

#ifndef _DISK_AB_
#define _DISK_AB_

typedef struct {
  unsigned sizeA:3;
  unsigned sizeB:3;
  unsigned sizeC:3;
  unsigned sum:7;
} SIZE_AB;
#endif

extern SIZE_AB SizeAB;
extern FILE_DATA far *fdata;
extern unsigned DiskAStartSeg,DiskBStartSeg;

#define DISKA	0
#define DISKB	1

/* int GetFileNo(void); */
#define GetFileNo()	GetFileNo_AB(DISKA)

/* int GetFileName(int no,char *fname); */
#define GetFileName(no,fname)	GetFileName_AB(DISKA,no,fname)

/* FILE_DATA far * GetFileInfoByNo(int no) */
#define GetFileInfoByNo(no)	GetFileInfoByNo_AB(DISKA,no)

/* FILE_DATA far * GetFileInfoByName(char *fname) */
#define GetFileInfoByName(fname)	GetFileInfoByName_AB(DISKA,fname)

/* char far * GetFilePositionByNo(int no) */
#define GetFilePositionByNo(no)	GetFilePositionByNo_AB(DISKA,no)

/* char far * GetFilePositionByName(char *fname) */
#define GetFilePositionByName(fname)	GetFilePositionByName_AB(DISKA,fname)

int cdecl GetFileNo_AB(int disk);
int cdecl GetFileName_AB(int disk,int no,char *fname);
FILE_DATA cdecl far * GetFileInfoByNo_AB(int disk,int no);
FILE_DATA cdecl far *GetFileInfoByName_AB(int disk,char *fname);
char cdecl far * GetFilePositionByNo_AB(int disk,int no);
char cdecl far * GetFilePositionByName_AB(int disk,char *fname);

void cdecl far *AddFarPtrLong(void cdecl far * ptr1,unsigned long size);
void cdecl ReadSizeAB(void);
void cdecl MoveToStartAddr(int disk);

/* for COM0 (used by 87K module) */
/* WITHOUT CTS & RTS control */
#define	COM1	0
#define	COM2	1
#define	COM3	2
#define	COM4	3

#define FLOW_CONTROL_DISABLE	0
#define FLOW_CONTROL_ENABLE	1
#define FLOW_CONTROL_AUTO_BY_HW 2
#define FLOW_CONTROL_AUTO_BY_SW	3

#define	ClearTxBuffer0		ClearTxBuffer_0
#define GetTxBufferFreeSize0	GetTxBufferFreeSize_0
#define PushDataToCom0		PushDataToCom_0

#define CheckInputBufSize0	CheckInputBufSize_0
#define InstallCom0		InstallCom_0
#define RestoreCom0		RestoreCom_0
#define SetBaudrate0		SetBaudrate_0
#define SetDataFormat0		SetDataFormat_0
#define ClearCom0		ClearCom_0
#define ClearCom0_DMA		ClearCom_DMA_0
#define DataSizeInCom0		DataSizeInCom_0
#define IsCom0			IsCom_0
#define IsCom0OutBufEmpty	IsComOutBufEmpty_0
#define ReadCom0		ReadCom_0
#define ToCom0Bufn		ToComBufn_0
#define ToCom0Str		ToComStr_0
#define SetCom0Timeout		SetComTimeout_0
#define ToCom0			ToCom_0
#define IsTxBufEmpty0		IsTxBufEmpty_0
#define WaitTransmitOver0	WaitTransmitOver_0
#define ReadCom0n		ReadComn_0
#define printCom0		printCom_0
#define SetBreakMode0		SetBreakMode_0
#define SendBreak0		SendBreak_0
#define IsDetectBreak0		IsDetectBreak_0

void cdecl ClearTxBuffer_0(void);
int cdecl GetTxBufferFreeSize_0(void);
int cdecl PushDataToCom_0(int data);
void cdecl CheckInputBufSize_0(void);
int cdecl InstallCom_0(unsigned long baud, int data, int parity,int stop);
int cdecl RestoreCom_0(void);
int cdecl SetBaudrate_0(unsigned long baud);
int cdecl SetDataFormat_0(int databit,int parity,int stopbit);
int cdecl ClearCom_0(void);
int cdecl DataSizeInCom_0(void);
int cdecl IsCom_0(void);
int cdecl IsComOutBufEmpty_0(void);
int cdecl ReadCom_0(void);
int cdecl ToComBufn_0(char *buf,int no);
int cdecl ToComStr_0(char *str);
void cdecl SetComTimeout_0(unsigned t);
int cdecl ToCom_0(int data);
int cdecl IsTxBufEmpty_0(void);
int cdecl WaitTransmitOver_0(void);
int cdecl ReadComn_0(unsigned char *buf,int no);
int cdecl printCom_0(const char *fmt,...);

int cdecl DataSizeInCom_DMA_0(void);
int cdecl ReadComn_DMA_0(unsigned char *buf,int maxsize);
int cdecl InstallCom_DMA_0(unsigned long baud, int data, int parity,int stop);
int cdecl ClearCom_DMA_0(void);
int cdecl IsCom_DMA_0(void);
int cdecl DataSizeInCom_DMA_0(void);
int cdecl ReadCom_DMA_0(void);

void cdecl SetBreakMode_0(int mode);
void cdecl SendBreak_0(unsigned TimeMs);
int cdecl IsDetectBreak_0(void);
void cdecl SetComPortBufferSize_0(int in_size,int out_size);

/* for COM0 use DMA */
unsigned Com0GetDataSize(void);
void cdecl Com0SetInputBuf(char far *ptr,unsigned cnt);
void cdecl Com0SendCmd(char far *cmd,unsigned length);
void cdecl OpenCom0UseDMA(void);
void cdecl CloseCom0UseDMA(void);

/* for COM1 */
/* WITHOUT CTS & RTS control */

#define	ClearTxBuffer1		ClearTxBuffer_1
#define GetTxBufferFreeSize1	GetTxBufferFreeSize_1
#define PushDataToCom1		PushDataToCom_1

#define CheckInputBufSize1	CheckInputBufSize_1
#define InstallCom1		InstallCom_1
#define RestoreCom1		RestoreCom_1
#define SetBaudrate1		SetBaudrate_1
#define SetDataFormat1		SetDataFormat_1
#define ClearCom1		ClearCom_1
#define DataSizeInCom1		DataSizeInCom_1
#define IsCom1			IsCom_1
#define IsCom1OutBufEmpty	IsComOutBufEmpty_1
#define ReadCom1		ReadCom_1
#define ToCom1Bufn		ToComBufn_1
#define ToCom1Str		ToComStr_1
#define SetCom1Timeout		SetComTimeout_1
#define ToCom1			ToCom_1
#define IsTxBufEmpty1		IsTxBufEmpty_1
#define WaitTransmitOver1	WaitTransmitOver_1
#define ReadCom1n		ReadComn_1
#define printCom1		printCom_1
#define SetBreakMode1		SetBreakMode_1
#define SendBreak1		SendBreak_1
#define IsDetectBreak1		IsDetectBreak_1

void cdecl ClearTxBuffer_1(void);
int cdecl GetTxBufferFreeSize_1(void);
int cdecl PushDataToCom_1(int data);
void cdecl CheckInputBufSize_1(void);
int cdecl InstallCom_1(unsigned long baud, int data, int parity,int stop);
int cdecl RestoreCom_1(void);
int cdecl SetBaudrate_1(unsigned long baud);
int cdecl SetDataFormat_1(int databit,int parity,int stopbit);
int cdecl ClearCom_1(void);
int cdecl DataSizeInCom_1(void);
int cdecl IsCom_1(void);
int cdecl IsComOutBufEmpty_1(void);
int cdecl ReadCom_1(void);
int cdecl ToComBufn_1(char *buf,int no);
int cdecl ToComStr_1(char *str);
void cdecl SetComTimeout_1(unsigned t);
int cdecl ToCom_1(int data);
int cdecl IsTxBufEmpty_1(void);
int cdecl WaitTransmitOver_1(void);
int cdecl ReadComn_1(unsigned char *buf,int no);
int cdecl printCom_1(const char *fmt,...);

void cdecl SetBreakMode_1(int mode);
void cdecl SendBreak_1(unsigned TimeMs);
int cdecl IsDetectBreak_1(void);
void cdecl SetComPortBufferSize_1(int in_size,int out_size);

int cdecl InstallCom_DMA_1(unsigned long baud, int data, int parity,int stop);
int cdecl DataSizeInCom_DMA_1(void);
int cdecl ReadComn_DMA_1(unsigned char *buf,int maxsize);
int cdecl ClearCom_DMA_1(void);
int cdecl IsCom_DMA_1(void);
int cdecl DataSizeInCom_DMA_1(void);
int cdecl ReadCom_DMA_1(void);

#define FLOW_CONTROL_DISABLE	0
#define FLOW_CONTROL_ENABLE	1
#define FLOW_CONTROL_AUTO_BY_HW 2
#define FLOW_CONTROL_AUTO_BY_SW	3

/*
 Functions for COM3
*/
#define InstallCom3	InstallCom_3
#define RestoreCom3	RestoreCom_3
#define IsCom3		IsCom_3
#define ToCom3		ToCom_3
#define ToCom3Str	ToComStr_3
#define ToCom3Bufn	ToComBufn_3
#define printCom3	printCom_3
#define ClearTxBuffer3	ClearTxBuffer_3
#define SetCom3FifoTriggerLevel		SetComFifoTriggerLevel_3
#define SetBaudrate3	SetBaudrate_3
#define ReadCom3	ReadCom_3
#define ClearCom3	ClearCom_3
#define DataSizeInCom3  DataSizeInCom_3
#define WaitTransmitOver3	WaitTransmitOver_3
#define IsTxBufEmpty3		IsTxBufEmpty_3
#define IsCom3OutBufEmpty	IsComOutBufEmpty_3
#define ReadCom3n		ReadComn_3
#define SetDataFormat3		SetDataFormat_3
#define SetRtsActive3		SetRtsActive_3
#define SetRtsInactive3		SetRtsInactive_3
#define GetCtsStatus3		GetCtsStatus_3
#define SetBreakMode3		SetBreakMode_3
#define SendBreak3		SendBreak_3
#define IsDetectBreak3		IsDetectBreak_3

int cdecl InstallCom_3(unsigned long baud, int data, int parity, int stop);
int cdecl RestoreCom_3(void);
int cdecl IsCom_3(void);
int cdecl ToCom_3(int data);
int cdecl ToComStr_3(char *str);
int cdecl ToComBufn_3(char *buf,int no);
int cdecl printCom_3(const char *fmt,...);
void cdecl ClearTxBuffer_3(void);
int cdecl SetComFifoTriggerLevel_3(int level);
int cdecl SetBaudrate_3(unsigned long baud);
int cdecl ReadCom_3(void);
int cdecl ClearCom_3(void);
int cdecl DataSizeInCom_3(void);
int cdecl WaitTransmitOver_3(void);
int cdecl IsTxBufEmpty_3(void);
int cdecl IsComOutBufEmpty_3(void);
int cdecl SetDataFormat_3(int databit,int parity,int stopbit);
int cdecl ReadComn_3(unsigned char *buf,int n);
void cdecl SetRtsActive_3(void);
void cdecl SetRtsInactive_3(void);
int cdecl GetCtsStatus_3(void);
void cdecl SendBreak_3(unsigned timems);
void cdecl SetBreakMode_3(int mode);
int cdecl IsDetectBreak_3(void);
void cdecl SetCtsControlMode_3(int mode);
void cdecl SetRtsControlMode_3(int mode);
void cdecl SetComPortBufferSize_3(int in_size,int out_size);
int cdecl GetComFifoTriggerLevel_3(void);	/*[2006/11/09]*/

/*
  mode can be:
FLOW_CONTROL_DISABLE	(0)
FLOW_CONTROL_ENABLE	(1) : only work for CTS control.
FLOW_CONTROL_AUTO_BY_HW (2) : set both RTC/CTS auto control by UART H/W
FLOW_CONTROL_AUTO_BY_SW	(3) : set RTC/CTS control by software(the driver/functions in this library)
*/
#define fCtsControlMode3	fCtsControlMode_3
extern int fCtsControlMode_3;

#define fRtsControlMode3	fRtsControlMode_3
extern int fRtsControlMode_3;

/*
 Functions for COM4
*/
#define InstallCom4	InstallCom_4
#define RestoreCom4	RestoreCom_4
#define IsCom4		IsCom_4
#define ToCom4		ToCom_4
#define ToCom4Str	ToComStr_4
#define ToCom4Bufn	ToComBufn_4
#define printCom4	printCom_4
#define ClearTxBuffer4	ClearTxBuffer_4
#define SetCom4FifoTriggerLevel		SetComFifoTriggerLevel_4
#define SetBaudrate4	SetBaudrate_4
#define ReadCom4	ReadCom_4
#define ClearCom4	ClearCom_4
#define DataSizeInCom4  DataSizeInCom_4
#define WaitTransmitOver4	WaitTransmitOver_4
#define IsTxBufEmpty4		IsTxBufEmpty_4
#define IsCom4OutBufEmpty	IsComOutBufEmpty_4
#define ReadCom4n		ReadComn_4
#define SetDataFormat4		SetDataFormat_4
#define SetRtsActive4		SetRtsActive_4
#define SetRtsInactive4		SetRtsInactive_4
#define GetCtsStatus4		GetCtsStatus_4
#define SetBreakMode4		SetBreakMode_4
#define SendBreak4		SendBreak_4
#define IsDetectBreak4		IsDetectBreak_4

int cdecl InstallCom_4(unsigned long baud, int data, int parity, int stop);
int cdecl RestoreCom_4(void);
int cdecl IsCom_4(void);
int cdecl ToCom_4(int data);
int cdecl ToComStr_4(char *str);
int cdecl ToComBufn_4(char *buf,int no);
int cdecl printCom_4(const char *fmt,...);
void cdecl ClearTxBuffer_4(void);
int cdecl SetComFifoTriggerLevel_4(int level);
int cdecl SetBaudrate_4(unsigned long baud);
int cdecl ReadCom_4(void);
int cdecl ClearCom_4(void);
int cdecl DataSizeInCom_4(void);
int cdecl WaitTransmitOver_4(void);
int cdecl IsTxBufEmpty_4(void);
int cdecl IsComOutBufEmpty_4(void);
int cdecl SetDataFormat_4(int databit,int parity,int stopbit);
int cdecl ReadComn_4(unsigned char *buf,int n);
void cdecl SetRtsActive_4(void);
void cdecl SetRtsInactive_4(void);
int cdecl GetCtsStatus_4(void);
void cdecl SendBreak_4(unsigned timems);
void cdecl SetBreakMode_4(int mode);
int cdecl IsDetectBreak_4(void);
void cdecl SetCtsControlMode_4(int mode);
void cdecl SetRtsControlMode_4(int mode);
void cdecl SetComPortBufferSize_4(int in_size,int out_size);
int cdecl GetComFifoTriggerLevel_4(void);	/*[2006/11/09]*/

/*
When use SetComPortBufferSize_0/1/3/4() to set the size of com port input/output buffer,
must call before call InstallCom_0/1/3/4().
*/

#define fCtsControlMode4	fCtsControlMode_4
extern int fCtsControlMode_4;

#define fRtsControlMode4	fRtsControlMode_4
extern int fRtsControlMode_4;

/* For COM4 without use INTERRUPT */
int cdecl IsCom_4_1(void);
int cdecl ReadCom_4_1(void);
int cdecl ReadComn_4_1(unsigned char *buf,int no);
int cdecl ToCom_4_1(int data);
void cdecl ReadDataFromUartFifo(void);
void cdecl SendDataToUartFifo(void);
void cdecl EnableMonitorCom4(void);
void cdecl DisableMonitorCom4(void);

/* for I-8000 module(slot/back plane) */
typedef union {
  struct {
   unsigned char do8,di8;
  } data8;
  unsigned      data16;
} DIODATA;

typedef union {
  DIODATA data;
  struct {
   unsigned do16,di16;
  } data16;
  unsigned long  data32;
} DIODATA32;


extern int NumberOfSlot;
extern unsigned SlotAddr[8];
extern int __Last87kSlot;
extern int __InChangeSlot;

#define _PARALLEL 0x80
#define _SCAN     0x40
#define _32BIT    0x20
#define _8BIT     0x10

#define _DI32     0xE3
#define _DO32     0xE0
#define _DI16DO16 0xE2
#define _DI16     0xC3
#define _DO16     0xC0
#define _DI8DO8   0xC2
#define _DI8      0x93
#define _DO8      0x90

//extern unsigned far *BA56912; /*=(unsigned far *)0x0040007EL; */
extern unsigned char far *const ModuleType; /*=(unsigned char far *)0x00400038L; */
extern unsigned char far *const NameOfModule; /*=(unsigned char far *)0x00400030L; */
extern unsigned long far *const LedData32; /*(unsigned long far *)0x0040004CL; */

extern DIODATA32 far *DIOData32;

int cdecl GetNetId(void);
void cdecl ChangeToSlot(unsigned slot);

//void cdecl ClrSlotInt(int mask);
//void cdecl ClrAllSlotInt(void);
//void cdecl DisableNMI(void);
//void cdecl EnableNMI(void);
//int GetSlotInt(void);
int cdecl GetNumberOfSlot(void);

void cdecl DO_32(int slot,unsigned long cdata);	/* For 32 bits output */
void cdecl DIO_DO_16(int slot,unsigned cdata);	/*  For:  16 bits output on DI16/DO16 */
void cdecl DO_16(int slot,unsigned int cdata);	/* For 16 bits output */
void cdecl DO_8(int slot,unsigned char cdata);	/* For 8 bits output */
void cdecl DIO_DO_8(int slot,unsigned char cdata);	/*  For:  8 bits output on DI8/DO8 or 4 bits output on DI4/DO4 */
unsigned long int cdecl DI_32(int slot);	/* For 32 bits input */
unsigned int cdecl DI_16(int slot);		/* For 16 bits input */
unsigned char cdecl DI_8(int slot);		/*For 8 bits input */
/*
[11/03/2003] add function for universal DI/O card.(such as 8050)
*/
#define UDIO_DI16	DI_16
#define UDIO_DO16	DO_16
unsigned cdecl UDIO_ReadConfig_16(int slot);
void cdecl UDIO_WriteConfig_16(int slot,unsigned config);

/*
 functions for COM port on 8142/8142I/8144/8144I/8112/8114
*/
int cdecl SetInBufSIze(int size);
int cdecl SetOutBufSIze(int size);
int cdecl _SetBaudrate(unsigned slot,unsigned port,unsigned long baud);
int cdecl _SetDataFormat(unsigned slot,unsigned port,int data, int parity,int stop);
int cdecl RestoreCom8000(unsigned slot);
int cdecl InstallCom8000(unsigned slot);
int cdecl IsCom8000(unsigned slot,unsigned port);
int cdecl ToCom8000(unsigned slot,unsigned port,int data);
int cdecl ToCom8000Str(unsigned slot,unsigned port,char *str);
int cdecl ToCom8000nBytes(unsigned slot,unsigned port,char *buf,int no);
int cdecl ReadCom8000(unsigned slot,unsigned port);
int cdecl ReadCom8000nBytes(unsigned slot,unsigned port,char *buf,int maxno);
int cdecl ClearCom8000(unsigned slot, unsigned port);
void cdecl ShowErrLedCom8000(unsigned slot,int data);
/*
	bit vallue: 0: LED ON, 1:LED OFF
	bit 0(0x01) for com port 1
	bit 1(0x02) for com port 2
	bit 2(0x04) for com port 3(for 8144(I)/8114 only)
	bit 3(0x08) for com port 4(for 8144(I)/8114 only)
*/

#define _MSR_dCTS	0x01
#define _MSR_dDSR	0x02
#define _MSR_TERI	0x04
#define _MSR_dDCD	0x08
#define _MSR_CTS	0x10
#define _MSR_DSR	0x20
#define _MSR_RI		0x40
#define _MSR_DCD	0x80

#define _MCR_DTR	0x01
#define _MCR_RTS	0x02
#define _MCR_OUT1	0x04
#define _MCR_OUT2	0x08
#define _MCR_LOOP	0x10
#define _MCR_AUTO_FLOW_CONTROL	0x20

int cdecl GetCom8000_MSR(unsigned slot, unsigned port);
/*
  Get current MSR.(Modem Status Register)

  On error return PortError(-1).
  On success return 8 bits status. The bit definition is as above.(_MSR_xxx)
*/
int cdecl SetCom8000_MCR(unsigned slot, unsigned port,int mcr);
/*
  Set the MCR(Modem Control Register) bit 0(DTR) & bit 1(RTS).
  On error return PortError(-1).
  On success return NoError(0).

  Only support to set DTR & RTS bit.
  for set RTS active just call SetCom8000_MCR(slot, port,_MCR_DTR+_MCR_RTS); (also active DTR)
  for set RTS inactive just call SetCom8000_MCR(slot, port,_MCR_DTR); (only active DTR)
  call SetCom8000_MCR(slot, port,0); to inactive both RTS & DTR.
*/

int cdecl SetCom8000_MCR_Bit(unsigned slot, unsigned port,int mcr_bit);
int cdecl ClearCom8000_MCR_Bit(unsigned slot, unsigned port,int mcr_bit);

int cdecl SetRts8000(unsigned slot, unsigned port,int mode);
/*
  mode=1 --> set RTS active
  mode=0 --> set RTS inactive
*/
int cdecl SetCtsControlMode8000(unsigned slot,unsigned port,int mode);
int cdecl SetRtsControlMode8000(unsigned slot,unsigned port,int mode);
/*
   mode: the same as SetCtsControlMode_3()
*/

int cdecl GetCtsControlMode8000(unsigned slot,unsigned port);
int cdecl GetRtsControlMode8000(unsigned slot,unsigned port);
int cdecl GetCom8000TxBufferFreeSize(unsigned slot, unsigned port);
void cdecl ClearCom8000TxBuffer(unsigned slot, unsigned port);
int cdecl GetCurMsr8000(unsigned slot,unsigned port);
int cdecl GetMsrChanged8000(unsigned slot,unsigned port);
void cdecl ClrMsrChanged8000(unsigned slot,unsigned port);
int cdecl SetCom8000FifoTriggerLevel(unsigned slot, unsigned port, int level);
int cdecl GetCom8000FifoTriggerLevel(unsigned slot, unsigned port);

void cdecl SendBreak8000(unsigned slot, unsigned port,unsigned timems);
void cdecl SetBreakMode8000(unsigned slot, unsigned port,int mode);
int cdecl IsDetectBreak8000(unsigned slot, unsigned port);


/*
  for S-MMI key
*/
#define SKEY_SET    1
#define SKEY_DOWN   2
#define SKEY_MODE   3
#define SKEY_UP     4

extern unsigned char far * const KeyStatus;
#define SKEY_SET_DOWN   0x08
#define SKEY_DOWN_DOWN  0x04
#define SKEY_UP_DOWN    0x02
#define SKEY_MODE_DOWN  0x01

#define IsSystemKey		_IsSystemKey
#define ClearSystemKey	_ClearSystemKey
#define GetSystemKey	_GetSystemKey

extern int (*_IsSystemKey)(void);
extern void cdecl (*_ClearSystemKey)(void);
extern int (*_GetSystemKey)(void);

/*
  For Send command to I-7000/I-87K series.
*/
//extern char hex_to_ascii[16];
int cdecl ascii_to_hex(char ascii);

int cdecl SendCmdTo7000(int iPort, unsigned char *cCmd, int iChksum);
/*
(INPUT)iPort:can be 0,1,3,4.
(INPUT)lTimeout: unit is ms.
(INPUT) cCmd: cmd for send to COM port(I-7000/I-87K).
              DON'T add '\r' at the end of cCmd, SendCmdTo7000() will add check sum(if needed) & '\r' after cCmd .
(INPUT) iChksum: 0: disable, 1: enable.
*/
#define ReceiveResponseFrom7000_loop	ReceiveResponseFrom7000
int cdecl ReceiveResponseFrom7000(int iPort, unsigned char *cCmd, long lTimeout, int iChksum);
/*
(INPUT)iPort:can be 0,1,3,4.
(INPUT)lTimeout: unit is check times.
(OUTPUT) cCmd: response from COM port(I-7000/I-87K).
(INPUT) iChksum: 0: disable, 1: enable.
*/

#define ReceiveResponseFrom7000_ms	ReceiveResponseFrom7000_1
int cdecl ReceiveResponseFrom7000_1(int iPort, unsigned char *cCmd, long lTimeout, int iChksum);
/*
(INPUT)iPort:can be 0,1,3,4.
(INPUT)lTimeout: unit is ms. (*****)
(OUTPUT) cCmd: response from COM port(I-7000/I-87K).
(INPUT) iChksum: 0: disable, 1: enable.
*/


/* for ALL COM PORT */
int cdecl printCom(int port,char *fmt,...);
int cdecl IsDetectBreak(int port);
int cdecl SendBreak(int port,unsigned timems);
int cdecl SetBreakMode(int port,int mode);
int cdecl ClearCom(int port);
int cdecl ClearTxBuffer(int port);
int cdecl InstallCom(int port, unsigned long baud, int data, int parity,int stop);
int cdecl ToComBufn(int port,char *buf,int no);
int cdecl RestoreCom(int port);
int cdecl ToComStr(int port,char *str);
int cdecl DataSizeInCom(int port);
int cdecl IsCom(int port);
int cdecl ReadComn(int port,unsigned char *buf,int n);
int cdecl ReadCom(int port);
int cdecl SetBaudrate(int port,unsigned long baud);
int cdecl SetDataFormat(int port,int databit,int parity,int stopbit); /*[2005/09/15]*/
int cdecl ToCom(int port,int data);
int cdecl IsTxBufEmpty(int port);
int cdecl GetTxBufferFreeSize(int port);
int cdecl WaitTransmitOver(int port);
int cdecl SetRtsActive(int port);
int cdecl SetRtsInactive(int port);
int cdecl GetCtsStatus(int port);
int cdecl SetCtsControlMode(int port,int mode);	/*[2006/01/19] add*/
int cdecl SetRtsControlMode(int port,int mode);	/*[2006/01/19] add*/
int cdecl SetXonXoffControlMode(int port,int mode);	/*[2006/01/19] add*/
int cdecl SetMCR(int port,int mcr); /*[2006/06/29] */
int cdecl SetMCR_Bit(int port, int mcr_bit); /*[2006/06/29] */
int cdecl ClearMCR_Bit(int port,int mcr_bit); /*[2006/06/29] */
int cdecl SetComFifoTriggerLevel(int port,int level); /*[2006/06/29] */
int cdecl SetComPortBufferSize(int port,int in_size,int out_size); /* [2006/07/12] */
int cdecl GetCtsControlMode(int port);/*[2006/11/09]*/
int cdecl GetRtsControlMode(int port);/*[2006/11/09]*/
int cdecl SetDtrActive(int port); /*[2006/11/09]*/
int cdecl SetDtrInactive(int port); /*[2006/11/09]*/
int cdecl GetDsrStatus(int port); /*[2006/11/09]*/
int cdecl GetMSR(int port);	/*[2006/11/09]*/
int cdecl GetCurMsr(int port);	/*[2006/11/09]*/
int cdecl GetMsrChanged(int port);	/*[2006/11/09]*/
int cdecl ClrMsrChanged(int port);	/*[2006/11/09]*/
int cdecl GetComFifoTriggerLevel(int port);	/*[2006/11/09]*/

/*  function table for up functions except printCom */
/*
  For example if want to call:
   if(IsCom(port)){
   	data=ReadCom(port);
   }
  also can use :
   if(IsCom_[port]()){
   	data=ReadCom_[port]();
   }

  IsCom(port)/ReadCom(port) just for backword compatible, it also will call IsCom_[port]()/ReadCom_[port]()

  and so on.
*/

#define COM_PORT_NO	4

extern int (*IsDetectBreak_[COM_PORT_NO+1])(void);
extern void (*SendBreak_[COM_PORT_NO+1])(unsigned timems);
extern void (*SetBreakMode_[COM_PORT_NO+1])(int mode);
extern int (*ClearCom_[COM_PORT_NO+1])(void);
extern void (*ClearTxBuffer_[COM_PORT_NO+1])(void);
extern int (*InstallCom_[COM_PORT_NO+1])(unsigned long baud, int data, int parity,int stop);
extern int (*ToComBufn_[COM_PORT_NO+1])(char *buf,int no);
extern int (*RestoreCom_[COM_PORT_NO+1])(void);
extern int (*ToComStr_[COM_PORT_NO+1])(char *str);
extern int (*DataSizeInCom_[COM_PORT_NO+1])(void);
extern int (*IsCom_[COM_PORT_NO+1])(void);
extern int (*ReadComn_[COM_PORT_NO+1])(unsigned char *buf,int n);
extern int (*ReadCom_[COM_PORT_NO+1])(void);
extern int (*SetBaudrate_[COM_PORT_NO+1])(unsigned long baud);
extern int (*ToCom_[COM_PORT_NO+1])(int data);
extern int (*IsTxBufEmpty_[COM_PORT_NO+1])(void);
extern int (*GetTxBufferFreeSize_[COM_PORT_NO+1])(void);
extern int (*WaitTransmitOver_[COM_PORT_NO+1])(void);
extern void (*SetRtsActive_[COM_PORT_NO+1])(void);
extern void (*SetRtsInactive_[COM_PORT_NO+1])(void);
extern int (*GetCtsStatus_[COM_PORT_NO+1])(void);
extern int (*SetDataFormat_[COM_PORT_NO+1])(int databit,int parity,int stopbit);
extern void (*SetCtsControlMode_[COM_PORT_NO+1])(int mode);	/*[2006/01/19] add*/
extern void (*SetRtsControlMode_[COM_PORT_NO+1])(int mode);	/*[2006/01/19] add*/
extern void (*SetXonXoffControlMode_[COM_PORT_NO+1])(int mode);	/*[2006/01/19] add*/
extern int (*SetMCR_[COM_PORT_NO+1])(int mcr); /* [2006/06/29] */
extern void (*ClearMCR_Bit_[COM_PORT_NO+1])(int mcr_bit); /*[2006/06/29] */
extern void (*SetMCR_Bit_[COM_PORT_NO+1])(int mcr_bit); /*[2006/06/29] */
extern int (*SetComFifoTriggerLevel_[COM_PORT_NO+1])(int level); /*[2006/06/29] */
extern void (*SetComPortBufferSize_[COM_PORT_NO+1])(int in_size,int out_size); /* [2006/07/12] */

extern int (*GetCtsControlMode_[COM_PORT_NO+1])(void);/*[2006/11/09]*/
extern int (*GetRtsControlMode_[COM_PORT_NO+1])(void);/*[2006/11/09]*/
extern void (*SetDtrActive_[COM_PORT_NO+1])(void); /*[2006/11/09]*/
extern void (*SetDtrInactive_[COM_PORT_NO+1])(void); /*[2006/11/09]*/
extern int (*GetDsrStatus_[COM_PORT_NO+1])(void); /*[2006/11/09]*/
extern int (*GetMSR_[COM_PORT_NO+1])(void);		/*[2006/11/09]*/
extern int (*GetCurMsr_[COM_PORT_NO+1])(void);	/*[2006/11/09]*/
extern int (*GetMsrChanged_[COM_PORT_NO+1])(void);	/*[2006/11/09]*/
extern int (*ClrMsrChanged_[COM_PORT_NO+1])(void);	/*[2006/11/09]*/
extern int (*GetComFifoTriggerLevel_[COM_PORT_NO+1])(void);	/*[2006/11/09]*/

int cdecl GetComportNumber(void);

extern const unsigned char far * const SystemSerialNumber;
void cdecl InitLib(void);
/*
  MUST CALL InitLib() first before use "DIOData32".
*/
void cdecl GetLibDate(char *date);
unsigned cdecl GetLibVersion(void);
/*
 Current version is 2.01 (return 0x0201)
*/

extern int TriggerLevel[4];

void cdecl InstallComInputData_3(int (*DoInputData)(unsigned char data));
void cdecl InstallComInputData_4(int (*DoInputData)(unsigned char data));
/*
After call InstallComInputData_3/4(), When the COM port ISR for COM3/4 receive a data,
it will call the function installed(DoInputData), and pass the received data to it.
And IsCom() will always return 0.
*/

/* [11/06/2003] add Software flow control(Xon/Xoff) for COM1~COM4
	COM2 is RS-485, used in half-duplex mode, need not software flow control.
*/
void cdecl SetXonXoffControlMode_1(int mode);
void cdecl SetXonXoffControlMode_3(int mode);
void cdecl SetXonXoffControlMode_4(int mode);
/*
  mode=0 --> disable Xon/Xoff control
  mode=1 --> enable Xon/Xoff control
*/

/*
 [2003/12/01]
 Add function for debug, using STDIO COM PORT.
 Even after all InstallCom_1() also can use these 3 functions to send message to STDIO COM port.
*/
void pascal _dPutch(int data1);
void cdecl _dPuts(char *str);
int cdecl _dPrint(char *fmt,...);

/*
 [2003/12/10]
 Add function for read system timeticks.
*/
long cdecl GetTimeTicks(void);
long cdecl GetTimeTicks_ISR(void); /* use this one in ISR */
/*
  The return value is *TimeTicks.
*/

int cdecl InstallUserTimerFunction_us(unsigned time,void cdecl (*fun)(void));
/*
  time unit is 0.1 us.

  for example:
	If want timer generate interrupt for every 0.5ms(500 us=5000*0.1us)
	(That is to say system will call your function once every 0.5 ms)
	just use

	void cdecl fun(void)
	{
		...
	}
	...
	InstallUserTimerFunction_us(5000,fun);
*/

int cdecl InstallUserTimerFunction_ms(unsigned time,void cdecl (*fun)(void));
/*
  time unit is ms.

  for example:
	If want timer generate interrupt for every 1 second(1 sec=1000 ms)
	(That is to say system will call your function once every 1 sec.)
	just use

	void cdecl fun(void)
	{
		...
	}
	...
	InstallUserTimerFunction_ms(1000,fun);
*/

void cdecl StopUserTimerFun(void);

/* [2004/02/17] add EEPROM(93C46) functions for I-8017/8024 */

/* For EEPROM(93C46) on 8017 */
void cdecl I8017_EE_WriteDisable(int slot);
void cdecl I8017_EE_WriteEnable(int slot);
unsigned cdecl I8017_EE_Read(int slot,int Addr);
void cdecl I8017_EE_Write(int slot,int Addr,unsigned Data);

/* For EEPROM(93C46) on 8024*/
#define I8024_EE_WriteEnable(slot)	I8017_EE_WriteEnable(slot)
#define I8024_EE_WriteDisable(slot)	I8017_EE_WriteDisable(slot)
unsigned cdecl I8024_EE_Read(int slot,int Addr);
void cdecl I8024_EE_Write(int slot,int Addr,unsigned Data);

/* 2004/02/26 add function usr burst mode to read date/time from RTC chip(DS-1302) */
typedef struct {
	int year;
	char month,day,weekday;
	char hour,minute,sec;
}TIME_DATE;

void cdecl GetTimeDate(TIME_DATE *timedate);
int cdecl SetTimeDate(TIME_DATE *timedate);
/*
  when call SetTimeDate(), need set the right year,month,day and the function
  will auto set the weekday.
*/

//extern const int IntVectNo8k[8];
//extern unsigned long OldIntVect8k[8];
//extern const unsigned ConPort8k[8];

/*
  [2004/04/19] add function CmdToArg().
  using " ,\r\t" as Delimiter.
  the value of cmd string  will be changed.
*/
#define MAX_CMD_NO 50
extern int Argc;
extern char *Argv[MAX_CMD_NO];
int cdecl CmdToArg(char *cmd);
/*
  return value is argument number(Argc).
  for example:

  char *cmd="p1 p2 p3\tp4,p5";

  after call CmdToArg(cmd);
  Argc=5,
  Argv[0]="p1";
  Argv[1]="p2";
  Argv[2]="p3";
  Argv[3]="p4";
  Argv[4]="p5";
*/

/*
 [2004/07/26] add
*/
extern int CurMsr_3;
extern int bMsrChanged_3;
extern int CurMsr_4;
extern int bMsrChanged_4;

/*
[2004/07/26] add DSR/DTR function for COM4.
*** DSR/DTR functions will work when COM4 can use interrupt mode. ***
*/
extern int fDsrControlMode_4,fDtrControlMode_4;
extern int CurDTR_4;

void cdecl SetDtrActive_4(void);
void cdecl SetDtrInactive_4(void);
int cdecl GetDsrStatus_4(void);
void cdecl SetDsrControlMode_4(int mode);/* not finished */
void cdecl SetDtrControlMode_4(int mode);/* not finished */

/*
 Functions for COM2
*/
#define InstallCom2	InstallCom_2
#define RestoreCom2	RestoreCom_2
#define IsCom2		IsCom_2
#define ToCom2		ToCom_2
#define ToCom2Str	ToComStr_2
#define ToCom2Bufn	ToComBufn_2
#define printCom2	printCom_2
#define ClearTxBuffer2	ClearTxBuffer_2
#define SetCom2FifoTriggerLevel		SetComFifoTriggerLevel_2
#define SetBaudrate2	SetBaudrate_2
#define ReadCom2	ReadCom_2
#define ClearCom2	ClearCom_2
#define DataSizeInCom2  DataSizeInCom_2
#define WaitTransmitOver2	WaitTransmitOver_2
#define IsTxBufEmpty2		IsTxBufEmpty_2
#define IsCom2OutBufEmpty	IsComOutBufEmpty_2
#define ReadCom2n		ReadComn_2
#define SetDataFormat2		SetDataFormat_2
#define SetBreakMode2		SetBreakMode_2
#define SendBreak2		SendBreak_2
#define IsDetectBreak2		IsDetectBreak_2

int cdecl InstallCom_2(unsigned long baud, int data, int parity, int stop);
int cdecl RestoreCom_2(void);
int cdecl IsCom_2(void);
int cdecl ToCom_2(int data);
int cdecl ToComStr_2(char *str);
int cdecl ToComBufn_2(char *buf,int no);
int cdecl printCom_2(const char *fmt,...);
void cdecl ClearTxBuffer_2(void);
int cdecl SetComFifoTriggerLevel_2(int level);
int cdecl SetBaudrate_2(unsigned long baud);
int cdecl ReadCom_2(void);
int cdecl ClearCom_2(void);
int cdecl DataSizeInCom_2(void);
int cdecl WaitTransmitOver_2(void);
int cdecl IsTxBufEmpty_2(void);
int cdecl IsComOutBufEmpty_2(void);
int cdecl SetDataFormat_2(int databit,int parity,int stopbit);
int cdecl ReadComn_2(unsigned char *buf,int n);
void cdecl SendBreak_2(unsigned timems);
void cdecl SetBreakMode_2(int mode);
int cdecl IsDetectBreak_2(void);
void cdecl SetComPortBufferSize_2(int in_size,int out_size);
int cdecl GetComFifoTriggerLevel_2(void);	/*[2006/11/09]*/

int cdecl GetSerialNumber(char *Serial);

/*
[2004/10/13] Add
  GetSerialNumber() is used to read system serial number from hardware.
  on sucess return 0, and the serial number store to Serial.
  on fail return -1: cannot find the hardware IC
	  return -2: CRC error
*/

void cdecl InstallNewTimer(void);
extern int Int9Flag;
/*
int cdecl _IsSystemKey(void);
int cdecl _GetSystemKey(void);
void cdecl _ClearSystemKey(void);
*/
/*
[2004/11/16] Add
  Support a new timer ISR, it WILL NOT call INT 0x1C(every 55ms),
  and will not call the ScanLed function if there is no I-8000 module on the slot.
*/

/*
[2005/02/14] add

   general purpose functions for all PIO pins
   Please be carefully for using these 3 functions.
   !!! NOT ALL 32 PIO pins can use used by user. !!!
*/
void cdecl SetPioDir(unsigned pin,int dir);
void cdecl SetPio(int pin,int mode);
int cdecl GetPio(int pin);
void cdecl SetPioHighLow(int pin); /* [2005/11/29]add */
void cdecl SetPioLowHigh(int pin); /* [2005/11/29]add */

#define _OUTPUT_MODE							0
#define _INPUT_MODE_WITH_PULL_HIGH_OR_LOW		1
#define _INPUT_MODE_WITHOUT_PULL_HIGH_OR_LOW	2
#define _NORMAL_MODE							3
/*
  input:
    pin : 0~31.
    mode: 0 or 1
    dir:
        0: set the PIO pin to output mode
        1: set the PIO pin to input with pull high(for some pin is pull low.)
        2: set the PIO pin to input without pull high/low.
    	3: set the PIO pin to normal mode

  output for GetPio():
	0: for input mode: the input is low.
	   for output mode: current output is low.
	non zero:   for input mode: the input is high.
	   for output mode: current output is high.
*/

/*
[2005/02/21] Add
*/
void cdecl far * AllocateTopMemory(unsigned long size);

/*
[2005/03/07] add
*/
int cdecl GetMSR_3(void);
int cdecl GetMSR_4(void);

int cdecl SetMCR_3(int mcr);
int cdecl SetMCR_4(int mcr);

void cdecl SetMCR_Bit_3(int mcr_bit);
void cdecl SetMCR_Bit_4(int mcr_bit);

void cdecl ClearMCR_Bit_3(int mcr_bit);
void cdecl ClearMCR_Bit_4(int mcr_bit);

/*
[2007/09/17] Add functions for S256/S512 on I-8000 back plate.

For S512 : 256(S256_MaxBlock) blocks, every block has 2048(S256_BlockSize) bytes.
			Total size is 512K bytes.
*/

#define OffsetError -100
#define S256_BlockSize		2048
#define S256_BlockWordSize	1024
#define S256_MaxBlock		256
/*---------------------------------------------------------------------*/
int cdecl S256_Init(void);
/*
Before use S256/S512 must call this function.

*** [DOES NOT DETECT battery status, because the new H/W does not support it.] ***
[return value]:
 512: ram size is 512K (S512).
   0: can not find S512.
*/

/*---------------------------------------------------------------------*/
int cdecl S256_Read(unsigned block,unsigned offset);
/*
Read data from SRAM by block & offset.
[input]
block:0-(S256_MaxBlock-1)
offset:0-(S256_BlockSize-1)

[return value]:
on success return the data sotred in sram.
if block is out of range return BlockError(-10),
if offset is out of range return OffsetError(-100).
*/

/*---------------------------------------------------------------------*/
int cdecl S256_Write(unsigned block,unsigned offset,unsigned char data);
/*
Write data to SRAM by block & offset.
[input]
block:0-(S256_MaxBlock-1)
offset:0-(S256_BlockSize-1)

[return value]:
on success return NoError(0).
if block is out of range return BlockError(-10),
if offser is out of range return OffsetError(-100).
*/

/*---------------------------------------------------------------------*/
int cdecl S256_ReadF(unsigned long address);
/*
Read data from SRAM by linear address.
[input]
address:0-0x7FFFF for S512

[return value]:
BlockError: the address is out og range,
>= 0 : return the data sotred in sram.
*/

/*---------------------------------------------------------------------*/
int cdecl S256_ReadFn(unsigned long address,unsigned no,unsigned char *buf);
/*
Read n bytes data from SRAM by linear address.

[input]
address:0-0x7FFFF for S512
no: the bytes number to read.
buf:the buffer address to store data.

[return value]:
return the readed data number.
if the address is out of range S256_ReadFn() return 0.
*/

/*---------------------------------------------------------------------*/
int cdecl S256_WriteF(unsigned long address,unsigned char data);
/*
Write data to SRAM by linear address.

[input]
address:0-0x7FFFF for S512

[return value]:
BlockError: the address is out og range,
NoError(0): Write success.
*/

/*---------------------------------------------------------------------*/
int cdecl S256_WriteFn(unsigned long address,unsigned no,unsigned char *data);
/*
Write n bytes data to SRAM by linear address.

[input]
address:0-0x7FFFF for S512
no: the bytes number to wirte.
buf:the ddress of the data buffer.

[return value]:
return the readed data number.
if the address is out of range S256_ReadFn() return 0.
*/
/*---------------------------------------------------------------------*/
int cdecl S256_WriteBlock(unsigned block,unsigned char *Buf);
int cdecl S256_ReadBlock(unsigned block,unsigned char *Buf);
/*---------------------------------------------------------------------*/

/*
[2005/05/27] add functions for using timer functions
*/
int cdecl SetDelayTimer(int no);
/*
The delay functions(Dealy()/Delay_1()/Delay_2() by default using system's timer 0.
user can call SetDelayTimer(1); set it to use timer 1.

return : 0 for using timer 0, 1 for using timer 1.
*/
int cdecl SetUserTimer(int no);
/*
The user timer functions(InstallUserTimerFunction_us()/InstallUserTimerFunction_ms()
 by default using system's timer 0.
user can call SetUserTimer(1); set it to use timer 1.

return : 0 for using timer 0, 1 for using timer 1.
*/

/*
  [2006/02/07] Add functions for write/delete file
  Please refer to the file:"OS7_file.txt" or "OS7_file_big5.txt"
*/
int cdecl OS7_DeleteAllFile(int disk);
long cdecl OS7_GetDiskFreeSize(int disk);
int cdecl OS7_OpenWriteFile(int disk);
int cdecl OS7_WriteFile(int disk,void cdecl *buf,int size);
int cdecl OS7_CloseWriteFile(int disk,FILE_DATA *f_data);
extern int OS7_FileDateTimeMode;
/*
  [2006/02/10] add functions for CRC16
  OS7_xxxx() will use these function to get the crc16 of the file data.
  Please refer to the file:"OS7_crc16.txt" or "OS7_crc16_big5.txt"
*/
int cdecl CRC16_Push(void);
int cdecl CRC16_Pop(void);
void cdecl CRC16_Set(unsigned val);
#define CRC16_Reset()	CRC16_Set(0)
unsigned cdecl CRC16_Read(void);
void cdecl CRC16_AddData(unsigned char data);
void cdecl CRC16_AddDataN(unsigned char far *data,unsigned length);
int cdecl CRC16_MakeTable(void);

/*
[2006/07/06] add 2 functions for 8112/8114/8142/8144 series.
*/
int cdecl IsCom8000OutBufEmpty(int slot, int port);
/*
on error return PortError(-1).
If the output buffer is empty retunrs 1.
If the output buffer is not empty returns 0.
*/

int cdecl WaitCom8000TransmitOver(int slot, int port);
/*
On success returns NoError(0).
On error returns PortError(-1) or TimeOut(-5).
*/

/*
[2006/08/28] Add
  The variable "ModuleName" is the same as the variable "NameOfModule".
  The variable "ModuleFullName" supports the full name of the modules plug on
the slots of I-8000 controller.
  ModuleFullName[0] contains the full name of the module on slot 0.
  ModuleFullName[1] contains the full name of the module on slot 1.
  ...
If there is no module on that slot , the value of ModuleFullName[slot] will be "".
for example :
for I-87017, ModuleName[slot]=17, ModuleFullName[slot] is "87017",
for I-87017ML, ModuleName[slot]=17, but ModuleFullName[slot] is "87017ML",
So if just use the ModuleName[slot] the program can not point out that module is
"87017" or "87017ML", but use the variable ModuleFullName[slot] the program can do it.
*/
extern unsigned char far * const ModuleName;
extern unsigned char far ** const ModuleFullName;

/*----------------------------------------------------------------------------*/
/*
 [2007/01/18] Add function to control the high ram(128K FRAM/SRAM or 256K SRAM )
 C837/813 can support high ram.(option)
*/
void cdecl EnableHighRam(void);
void cdecl DisableHighRam(void);
int cdecl GetHighRamMode(void); /* [2007/06/04] add */
/*
 return value:
 0 --> high ram is ENABLED.
 != 0 --> high ram is DISABLED.
*/

void cdecl SetHighRam(int mode); /* [2007/06/04] add */
/*
 mode=0 --> ENABLE high ram
 mode != 0 --> DISABLE high ram.
*/

/*----------------------------------------------------------------------------*/
#define LowByte(data)	(unsigned char)(data)
#define HighByte(data)	*(((unsigned char *)&(data))+1)

/* [2007/08/02] add get high/low word from a long variable */
#define LowWord(data)	(unsigned)(data)
#define HighWord(data)	*(((unsigned *)&(data))+1)

/*----------------------------------------------------------------------------*/
/*
[2007/06/20] Add functions for swap data byte order.
*/
unsigned cdecl SwapShort(unsigned data);
/*
  swap the high byte and low byte.
  SwapShort(0x1234) will return 0x3412;
*/

unsigned long cdecl SwapLong(unsigned long val);
/*
  SwapLong(0x12345678) will return 0x78563412;
*/

/*----------------------------------------------------------------------------*/
/*
[2007/06/28] Add functions for CRC-16 used by Modbus/RTU
*/
extern unsigned Modbus_CRC16;
void cdecl Modbus_CRC16_Set(unsigned val);
#define Modbus_CRC16_Reset()	Modbus_CRC16_Set(0xFFFF)
unsigned cdecl Modbus_CRC16_Read(void);
void cdecl Modbus_GetCRC16(unsigned char *puchMsg, int DataLen);
int cdecl Modbus_CRC16_Push(void);
int cdecl Modbus_CRC16_Pop(void);

/*----------------------------------------------------------------------------*/
/*
[2007/07/06] Add functions for the NAME/VER/DATE of the library.
*/
unsigned cdecl GetOsLibVersion(void); /* the same as GetLibVersion(). */
char cdecl *GetOsLibDate(void);
char cdecl *GetOsLibName(void);

/*----------------------------------------------------------------------------*/
/* [2007/09/11] Add functions for new Back Plane */
void cdecl Install_8KIsr(void); /* InitLib() will call this function.*/
void cdecl UnInstall_8KIsr(void);

int cdecl InstallSlotLevelIsr(unsigned slot,void cdecl (*isr)(int slot));
int cdecl UnInstallSlotLevelIsr(unsigned slot);

int cdecl InstallSlotRisingIsr(unsigned slot,void cdecl (*isr)(int slot));
int cdecl UnInstallSlotRisingIsr(unsigned slot);

int cdecl InstallComIsr_2(void cdecl (*isr)(void));
int cdecl UnInstallComIsr_2(void);
int cdecl InstallComIsr_3(void cdecl (*isr)(void));
int cdecl UnInstallComIsr_3(void);
int cdecl InstallComIsr_4(void cdecl (*isr)(void));
int cdecl UnInstallComIsr_4(void);

int cdecl InstallModulePlugIsr(void cdecl (*isr)(void));
int cdecl UnInstallModulePlugIsr(void);

int cdecl InstallTimerOutIsr(void cdecl (*isr)(void),unsigned dtHigh,unsigned dtLow); /* unit = 1 us */
int cdecl UnInstallTimerOutIsr(void);

int cdecl InstallTimer1Isr(void cdecl (*isr)(void),unsigned dt); /* unit = 1 us */
int cdecl UnInstallTimer1Isr(void);

int cdecl InstallTimer2Isr(void cdecl (*isr)(void),unsigned dt); /* unit = 10 us */
int cdecl UnInstallTimer2Isr(void);

/*----------------------------------------------------------------------------*/
/* [2008/03/03] add */
int cdecl FlashGetWpStatus(void);
/*
return 0: Flash is in Write-Enabled mode
return 1: Flash is in Write-Protected mode
*/

/*----------------------------------------------------------------------------*/
/* [2008/03/05] add */
int cdecl GetBatteryStatus(void);
#define BATTERY_LOW_1	2
#define BATTERY_LOW_2	4
/*
return value:
bit1 : status of battery 1
bit2 : status of battery 2
0 means battery is OK, 1 means battery low.
The two batteries are used for RTC & the 512K bytes battery backup SRAM.
*/

/*----------------------------------------------------------------------------*/
/*[2008/04/01] add function for microSD*/
int cdecl microSD_WriteBlock(void cdecl *data,unsigned long block,unsigned BlockNo);
int cdecl microSD_ReadBlock(void cdecl *data,unsigned long block,unsigned BlockNo);
int cdecl microSD_Start(unsigned long *BlockNo,unsigned long *MemorySize);
int cdecl microSD_CheckCardReady(void); //[2008/11/14] add.
/*
return value:
1--> SD card is ready.
0--> SD card is NOT ready.(maybe removed)
*/

/*----------------------------------------------------------------------------*/
/*[2008/08/14] add function for new plugged 8K module */
void cdecl Check_ID_8K(int slot);

/*----------------------------------------------------------------------------*/
/*[2009/04/28] */
extern unsigned char CPU_Version;
void cdecl Sound(unsigned freq,unsigned period);
void cdecl _Sound(unsigned freq);
void cdecl NoSound(void);

/*----------------------------------------------------------------------------*/
/*
[2010/04/14] Add functions for delay function to use Timer 0 or Timer 1.
*/
void cdecl Delay0(unsigned ms);
void cdecl Delay1(unsigned ms);
/*
	Delay time unit = 1 ms
	Delay0() and Delay1() are the same as Delay(),but
	Delay0() will use Timer 0,
	Delay1() will use Timer 1.
*/
void cdecl Delay0_1(unsigned ms);
void cdecl Delay1_1(unsigned ms);
void cdecl Delay2_1(unsigned ms);
/*
	Delay time unit = 0.1 ms
	Delay0_1() and Delay1_1() are the same as Delay_1(),but
	Delay0_1() will use Timer 0,
	Delay1_1() will use Timer 1.
	Delay2_1() will use Timer 2. [2011/12/14] add
*/

void cdecl Delay0_2(unsigned ms);
void cdecl Delay1_2(unsigned ms);
void cdecl Delay2_2(unsigned ms);
/*
	Delay time unit = 0.01 ms
	Delay0_2() and Delay1_2() are the same as Delay_2(),but
	Delay0_2() will use Timer 0,
	Delay1_2() will use Timer 1.
	Delay2_2() will use Timer 2. [2011/12/14] add
*/

void cdecl Delay2(unsigned ms);
/*
	Delay time unit = 1 ms
	Delay2() like Delay() but it use Timer 2.
	If the program only use Delay2(), it can use two timer ISRs by call
	(InstallUserTimer0Function_ms() or InstallUserTimer0Function_us() )
	and
	(InstallUserTimer1Function_us() or InstallUserTimer1Function_us() )
*/

int cdecl InstallUserTimer0Function_us(unsigned time,void cdecl (*fun)(void));
int cdecl InstallUserTimer0Function_ms(unsigned time,void cdecl (*fun)(void));
void cdecl StopUserTimer0Fun(void);

int cdecl InstallUserTimer1Function_us(unsigned time,void cdecl (*fun)(void));
int cdecl InstallUserTimer1Function_ms(unsigned time,void cdecl (*fun)(void));
void cdecl StopUserTimer1Fun(void);
/* End of [2010/04/14]--------------------------------------------------------*/

/* [2013/11/27] */
extern long Com8000TxTimeout; /* default value = 100(ms); */
/*
Set the timeout for function WaitCom8000TransmitOver(slot,port);
*/
/* End of [2013/11/27]--------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif
