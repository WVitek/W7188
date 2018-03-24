#define    IN_BUF_SIZE      1024

typedef unsigned int  uint;
typedef unsigned int  WORD;
typedef unsigned char uchar;
typedef unsigned char BYTE;
typedef unsigned long ulong;
typedef unsigned long DWORD;

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
#define    TimeIsUp        1

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


#define SEND_CMD(port,cmd,timeout,checksum)  SendCmdTo7000(port,cmd,checksum)
#define RECEIVE_CMD ReceiveResponseFrom7000

//extern char hex_to_ascii[16];
extern int Com3Base;
extern int Com4Base;

#ifdef __cplusplus
extern "C" {
#endif

void cdecl InitLib(void);
int  cdecl InstallCom(int port, unsigned long baud, int data, int parity, int stop);
int  cdecl InstallCom3(unsigned long baud, int data, int parity);
int  cdecl InstallCom4(unsigned long baud, int data, int parity);
int  cdecl RestoreCom(int port);
int  cdecl RestoreCom3(void);
int  cdecl RestoreCom4(void);
int  cdecl IsCom(int port);
int  cdecl IsCom3(void);
int  cdecl IsCom4(void);
int  cdecl ReadCom(int port);
int  cdecl ReadCom3(void);
int  cdecl ReadCom4(void);
int  cdecl ToCom(int port, int data);
int  cdecl ToCom3(int data);
int  cdecl ToCom4(int data);
int cdecl ToComBufn(int port,char *buf,int no);
int cdecl ToCom3Bufn(char *buf,int no);
int cdecl ToCom4Bufn(char *buf,int no);
int  cdecl ToComStr(int port, char *str);
int  cdecl ToCom3Str(char *str);
int  cdecl ToCom4Str(char *str);
int  cdecl ClearCom(int port);
int  cdecl ClearCom3(void);
int  cdecl ClearCom4(void);
int  cdecl WaitTransmitOver(int port);
int  cdecl WaitTransmitOver3(void);
int  cdecl WaitTransmitOver4(void);

int  cdecl ReadInitPin(void);
void cdecl LedOff(void);
void cdecl LedOn(void);
void cdecl Init5DigitLed(void);
int  cdecl Show5DigitLed(int position, int value);
int  cdecl Show5DigitLedSeg(int pos, unsigned char data);
void cdecl DelayTimeMs(unsigned int time);

int  cdecl ReadNVRAM(int addr);
int  cdecl WriteNVRAM(int addr, int data);

int  cdecl WriteEEP(int block, int addr, int data);
int  cdecl ReadEEP(int block, int addr);
void cdecl EnableEEP(void);
void cdecl ProtectEEP(void);

void cdecl EnableWDT(void);
void cdecl RefreshWDT(void);
void cdecl DisableWDT(void);

int cdecl FlashReadId(void);
int cdecl FlashWrite(unsigned int seg, unsigned int offset, char data);
int cdecl FlashErase(unsigned int seg);
int cdecl FlashRead(unsigned int seg, unsigned int offset);

void cdecl Delay(unsigned ms);
void cdecl Delay_1(unsigned ms);

int cdecl Is7188xa(void);

int cdecl ascii_to_hex(char ascii);
int cdecl SendCmdTo7000(int iPort, unsigned char *cCmd, int iChksum);
int cdecl ReceiveResponseFrom7000(int iPort, unsigned char *cCmd, long lTimeout, int iChksum);

int cdecl IsResetByWatchDogTimer(void);
int cdecl IsResetByPowerOff(void); /* for bios date 12/12/98 or later */

int cdecl Show5DigitLedWithDot(int pos, int data);
void cdecl Set5DigitLedTestMode(int mode);
void cdecl Set5DigitLedIntensity(int mode);
void cdecl Disable5DigitLed(void);
void cdecl Enable5DigitLed(void);
unsigned cdecl GetLibVersion(void);

/* 01/07/1999  Add Timer function */
int cdecl TimerOpen(void);
int cdecl TimerClose(void);
void cdecl TimerResetValue(void);
unsigned long cdecl TimerReadValue(void);
void cdecl DelayMs(unsigned t);
int cdecl StopWatchReset(int channel);
int cdecl StopWatchStart(int channel);
int cdecl StopWatchStop(int channel);
int cdecl StopWatchPause(int channel);
int cdecl StopWatchContinue(int channel);
int cdecl StopWatchReadValue(int channel,unsigned long *value);
int cdecl CountDownTimerStart(int channel,unsigned long count);
int cdecl CountDownTimerReadValue(int channel,unsigned long *value);
void cdecl InstallUserTimer(void (*fun)(void));
void cdecl InstallUserTimer1C(void (*fun)(void));

int cdecl DataSizeInCom(int port);
int cdecl DataSizeInCom3(void);
int cdecl DataSizeInCom4(void);
/* New for Ver 1.13 */
/* 07-23-1999 add 4 function for COM1 /DTR,/RTS output pins */
void cdecl SetRtsActive(void);
void cdecl SetRtsInactive(void);

/* [New for Ver 1.14] */
/**** 07/27/1999 add 5 function for COM1/2/3/4 formated output ****/
int cdecl printCom(int port,char *fmt,...);
int cdecl printCom3(char *fmt,...);
int cdecl printCom4(char *fmt,...);
/* 08/06/1999 add 2 function for COM1 CTS,DSR input pins */
int cdecl GetCtsStatus(void);

/* 08/25/1999 add 5 function for COM1-4 */
int cdecl IsTxBufEmpty(int port);
int cdecl IsTxBufEmpty3(void);
int cdecl IsTxBufEmpty4(void);
int cdecl IsCom3OutBufEmpty(void);
int cdecl IsCom4OutBufEmpty(void);

/* 10/19/1999 add 5 function for COM1/2/3/4 change baudrate */
int cdecl SetBaudrate(int port,unsigned long baud);
int cdecl SetBaudrate3(unsigned long baud);
int cdecl SetBaudrate4(unsigned long baud);

/* 10/20/1999 */
/* (1) add 10 function for COM1/2/3/4 send/detect BREAK signal */
int cdecl SendBreak(int port,unsigned timems);
void cdecl SendBreak3(unsigned timems);
void cdecl SendBreak4(unsigned timems);
int cdecl IsDetectBreak(int port);
int cdecl IsDetectBreak3(void);
int cdecl IsDetectBreak4(void);

/* (2) Add 3 functions for clear COM1/2/3/4 output buffer */
int cdecl ClearTxBuffer(int port);
void cdecl ClearTxBuffer3(void);
void cdecl ClearTxBuffer4(void);


/* 12/29/1999 */
/*
  Add functions for ReadOnly file system of 7188R/7188X
*/
int cdecl GetFileNo(void);
int cdecl GetFileName(int no,char *fname);
char far* cdecl GetFilePositionByNo(int no);
char far* cdecl GetFilePositionByName(char *fname);
FILE_DATA far* cdecl GetFileInfoByNo(int no);           /* 04/11/2000 */
FILE_DATA far* cdecl GetFileInfoByName(char *fname);    /* 04/11/2000 */

/*
 02/15/2000
 Add functions for system DATE/TIME
*/
void cdecl GetTime(int *hour,int *minute,int *sec);
int cdecl SetTime(int hour,int minute,int sec);
void cdecl GetDate(int *year,int *month,int *day);
int cdecl SetDate(int year,int month,int day);

void cdecl ClockHighLow(void);
void cdecl ClockHigh(void);
void cdecl ClockLow(void);

/*
 03/15/2000
 Add function Scanf ...
*/
int cdecl LineInput(char *buf,int maxlen); /* input one line from StdInput */
void cdecl ResetScanBuffer(void); /* Set Scanf to use default buffer(maxlen=80) */
void cdecl SetScanBuffer(unsigned char *buf,int len); /* Set user's buffer for Scanf*/
int cdecl Scanf(char *fmt, ...); /* like C's scanf */

/*
 06/09/2000
 Add function for COM3/COM4
  COM3 for 7522/7523 only
  COM4 for 7523 only
*/
int cdecl Set485DirToTransmit(int port);
int cdecl Set485DirToReceive(int port);
void cdecl SetCom2AutoDir(void);
void cdecl ResetCom2AutoDir(void);

int cdecl InstallCom1(unsigned long baud, int data, int parity, int stop);
int cdecl RestoreCom1(void);
int cdecl IsCom1(void);
int cdecl ToCom1(int data);
int cdecl ToCom1Str(char *str);
int cdecl ToCom1Bufn(char *buf,int no);
int cdecl printCom1(char *fmt,...);
void cdecl ClearTxBuffer1(void);
int cdecl SetCom1FifoTriggerLevel(int level);
int cdecl SetBaudrate1(unsigned long baud);
int cdecl ReadCom1(void);
int cdecl ClearCom1(void);
int cdecl DataSizeInCom1(void);
int cdecl WaitTransmitOver1(void);
int cdecl IsTxBufEmpty1(void);
int cdecl IsCom1OutBufEmpty(void);
void cdecl SetDtrActive1(void);
void cdecl SetDtrInactive1(void);
void cdecl SetRtsActive1(void);
void cdecl SetRtsInactive1(void);
int cdecl GetCtsStatus1(void);
int cdecl GetDsrStatus1(void);

int cdecl InstallCom2(unsigned long baud, int data, int parity, int stop);
int cdecl RestoreCom2(void);
int cdecl IsCom2(void);
int cdecl ToCom2(int data);
int cdecl ToCom2Str(char *str);
int cdecl ToCom2Bufn(char *buf,int no);
int cdecl printCom2(char *fmt,...);
void cdecl ClearTxBuffer2(void);
int cdecl SetCom2FifoTriggerLevel(int level);
int cdecl SetBaudrate2(unsigned long baud);
int cdecl ReadCom2(void);
int cdecl ClearCom2(void);
int cdecl DataSizeInCom2(void);
int cdecl WaitTransmitOver2(void);
int cdecl IsTxBufEmpty2(void);
int cdecl IsCom2OutBufEmpty(void);

/*
  06/15/2000
 Add function for COM3
  COM3 for 7522/7523 only
*/
void cdecl SetRtsActive1(void);
void cdecl SetRtsInactive1(void);
int cdecl GetCtsStatus1(void);

/*
 10/16/2000  start Ver 1.17
 Add new EEPROM functions
*/
void cdecl EE_WriteEnable(void);
void cdecl EE_WriteProtect(void);
int cdecl EE_RandomRead(int Block,int Addr);
int cdecl EE_ReadNext(int Block);
int cdecl EE_MultiRead(int StartBlock,int StartAddr,int no,char *databuf);
int cdecl EE_RandomWrite(int Block,int Addr,int Data);
int cdecl EE_MultiWrite(int Block,int Addr,int no,char *Data);

/*
 05/15/2001
 Add new Led functions
*/
void cdecl LedToggle(void);

/*
 09/14/2001
 Add new variable for system hardware serial number
*/
extern unsigned char far * const SystemSerialNumber;

/*
 09/21/2001
 Add new functions for DI1,DI2,DO1,DO2
 Add new functions for PIO pins on I/O expansion bus
*/
void cdecl SetDo1On(void);
/*
  DO1 is open collector output, SetDo1On will set the output Tx On.
*/
void cdecl SetDo1Off(void);
/*
  DO1 is open collector output, SetDo1Off will set the output Tx Off.
*/
int cdecl GetDo1(void);
/*
  DO1 is open collector output, GetDo1 will get the output Tx status.
  return value:
  0: the output Tx is Off.
  others(!=0):the output Tx is On.
*/

void cdecl SetDo2On(void);
/*
  DO2 is open collector output, SetDo2On will set the output Tx On.
*/
void cdecl SetDo2Off(void);
/*
  DO2 is open collector output, SetDo2Off will set the output Tx Off.
*/
int cdecl GetDo2(void);
/*
  DO2 is open collector output, GetDo2 will get the output Tx status.
  return value:
  0: the output Tx is Off.
  others(!=0):the output Tx is On.
*/
int cdecl GetDi1(void);
/*
  return value:
  0: the input is Low.
  others(!=0):the input is Low.
*/
int cdecl GetDi2(void);
/*
  return value:
  0: the input is Low.
  others(!=0):the input is Low.
*/
void cdecl SetDio4Dir(int dir);
void cdecl SetDio9Dir(int dir);
void cdecl SetDio14Dir(int dir);
void cdecl SetTi1Dir(int dir);
void cdecl SetTo1Dir(int dir);
void cdecl SetTi0Dir(int dir);
void cdecl SetTo0Dir(int dir);
void cdecl SetInt4Dir(int dir);
/*
  set the mode for PIO(Programmable I/O) pins
  [Input]
  dir:
   0: set to output mode
   1: set to Input With pull up/down mode(To0 & To1 are pull down, others pull up)
   2: set to Input Without pull up/down mode
   3: set to Normal mode
*/
void cdecl SetDio4High(void);
void cdecl SetDio9High(void);
void cdecl SetDio14High(void);
void cdecl SetTo1High(void);
void cdecl SetTi0High(void);
void cdecl SetTo0High(void);
void cdecl SetInt4High(void);
/*
  set the PIO(Programmable I/O) pins to output high.
  (work only when they are set on output mode )
*/

void cdecl SetDio4Low(void);
void cdecl SetDio9Low(void);
void cdecl SetDio14Low(void);
void cdecl SetTo1Low(void);
void cdecl SetTi0Low(void);
void cdecl SetTo0Low(void);
void cdecl SetInt4Low(void);
/*
  set the PIO(Programmable I/O) pins to output low.
  (work only when they are set on output mode )
*/

int cdecl GetDio4(void);
int cdecl GetDio9(void);
int cdecl GetDio14(void);
int cdecl GetTo1(void);
int cdecl GetTi0(void);
int cdecl GetTo0(void);
int cdecl GetInt4(void);
/*
  get the current input status of PIO(Programmable I/O) pins.(when set to input mode.)
  or get the current output status of PIO(Programmable I/O) pins.(when set to output mode.)
  return value:
  0:input/output low.
  1:input/output high.
*/

/*
 08/01/2001
 Add new timer functions
*/
extern const unsigned long far * const TimeTicks;

/* for StopWatch [計時碼表] */
#ifndef _T_STOPWATCH_
#define _T_STOPWATCH_
typedef struct {
 ulong ulStart,ulPauseTime;
 uint  uMode;  /* 0: pause, 1:run(start) */
} STOPWATCH;
#endif

void cdecl T_StopWatchStart(STOPWATCH *sw);
ulong cdecl T_StopWatchGetTime(STOPWATCH *sw);
void cdecl T_StopWatchPause(STOPWATCH *sw);
void cdecl T_StopWatchContinue(STOPWATCH *sw);

/* For CountDown Timer[倒數計時] */
#ifndef _T_COUNTDOWNTIMER_
#define _T_COUNTDOWNTIMER_
typedef struct {
 ulong ulTime,ulStartTime,ulPauseTime;
 uint  uMode;  /* 0: pause, 1:run(start) */
} COUNTDOWNTIMER;
#endif

void cdecl T_CountDownTimerStart(COUNTDOWNTIMER *cdt,ulong timems);
void cdecl T_CountDownTimerPause(COUNTDOWNTIMER *cdt);
void cdecl T_CountDownTimerContinue(COUNTDOWNTIMER *cdt);
int cdecl T_CountDownTimerIsTimeUp(COUNTDOWNTIMER *cdt);
ulong cdecl T_CountDownTimerGetTimeLeft(COUNTDOWNTIMER *cdt);

/*
 11/09/2001
 Add new timer functions
*/
void cdecl T2_UpdateCurrentTimeTicks(void);
void cdecl T2_StopWatchStart(STOPWATCH *sw);
ulong cdecl T2_StopWatchGetTime(STOPWATCH *sw);
void cdecl T2_StopWatchPause(STOPWATCH *sw);
void cdecl T2_StopWatchContinue(STOPWATCH *sw);

void cdecl T2_CountDownTimerStart(COUNTDOWNTIMER *cdt,ulong timems);
void cdecl T2_CountDownTimerPause(COUNTDOWNTIMER *cdt);
void cdecl T2_CountDownTimerContinue(COUNTDOWNTIMER *cdt);
int cdecl T2_CountDownTimerIsTimeUp(COUNTDOWNTIMER *cdt);
ulong cdecl T2_CountDownTimerGetTimeLeft(COUNTDOWNTIMER *cdt);


#ifdef __cplusplus
}
#endif
