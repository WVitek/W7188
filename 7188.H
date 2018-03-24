#ifdef __TURBOC__
  #if (__TURBOC__ < 0x0300)
/*    #define inp    inportb */
    #define inpw   inport
/*    #define outp   outportb */
    #define outpw  outport
  #endif
#endif

#define    IN_BUF_SIZE      1024

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
#define    TimeIsUp        1


//extern char hex_to_ascii[16];

#ifdef __cplusplus
extern "C" {
#endif

void cdecl InitLib(void);
int  cdecl InstallCom(int port, unsigned long baud, int data, int parity, int stop);
int  cdecl InstallCom1(unsigned long baud, int data, int parity, int stop);
int  cdecl InstallCom2(unsigned long baud, int data, int parity, int stop);
int  cdecl InstallCom3(unsigned long baud, int data, int parity);
int  cdecl InstallCom4(unsigned long baud, int data, int parity);
int  cdecl RestoreCom(int port);
int  cdecl RestoreCom1(void);
int  cdecl RestoreCom2(void);
int  cdecl RestoreCom3(void);
int  cdecl RestoreCom4(void);
int  cdecl IsCom(int port);
int  cdecl IsCom1(void);
int  cdecl IsCom2(void);
int  cdecl IsCom3(void);
int  cdecl IsCom4(void);
int  cdecl ReadCom(int port);
int  cdecl ReadCom1(void);
int  cdecl ReadCom2(void);
int  cdecl ReadCom3(void);
int  cdecl ReadCom4(void);
int  cdecl ToCom(int port, int data);
int  cdecl ToCom1(int data);
int  cdecl ToCom2(int data);
int  cdecl ToCom3(int data);
int  cdecl ToCom4(int data);
int  cdecl ToComBufn(int port,char *buf,int no);
int  cdecl ToCom1Bufn(char *buf,int no);
int  cdecl ToCom2Bufn(char *buf,int no);
int  cdecl ToCom3Bufn(char *buf,int no);
int  cdecl ToCom4Bufn(char *buf,int no);
int  cdecl ToComStr(int port, char *str);
int  cdecl ToCom1Str(char *str);
int  cdecl ToCom2Str(char *str);
int  cdecl ToCom3Str(char *str);
int  cdecl ToCom4Str(char *str);
int  cdecl ClearCom(int port);
int  cdecl ClearCom1(void);
int  cdecl ClearCom2(void);
int  cdecl ClearCom3(void);
int  cdecl ClearCom4(void);
int  cdecl WaitTransmitOver(int port);
int  cdecl WaitTransmitOver1(void);
int  cdecl WaitTransmitOver2(void);
int  cdecl WaitTransmitOver3(void);
int  cdecl WaitTransmitOver4(void);
int  cdecl Set485DirToTransmit(int port);
int  cdecl Set485DirToReceive(int port);

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

int cdecl Is7188(void);
int cdecl getch4(void);
int cdecl kbhit4(void);
int cdecl ungetch4(int key);
void cdecl putch4(int data);

#define SEND_CMD(port,cmd,timeout,checksum)  SendCmdTo7000(port,cmd,checksum)
#define RECEIVE_CMD ReceiveResponseFrom7000
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
int cdecl IsCom3OutBufEmpty(void);
int cdecl IsCom4OutBufEmpty(void);
int cdecl DataSizeInCom(int port);
int cdecl DataSizeInCom1(void);
int cdecl DataSizeInCom2(void);
int cdecl DataSizeInCom3(void);
int cdecl DataSizeInCom4(void);
/* New for Ver 1.13 */
/* 07-23-1999 add 4 function for COM1 /DTR,/RTS output pins */
void cdecl SetDtrActive(void);
void cdecl SetDtrInactive(void);
void cdecl SetRtsActive(void);
void cdecl SetRtsInactive(void);

/* [New for Ver 1.14] */
/**** 07/27/1999 add 5 function for COM1/2/3/4 formated output ****/
int cdecl printCom(int port,char *fmt,...);
int cdecl printCom1(char *fmt,...);
int cdecl printCom2(char *fmt,...);
int cdecl printCom3(char *fmt,...);
int cdecl printCom4(char *fmt,...);
/* 08/06/1999 add 2 function for COM1 CTS,DSR input pins */
int cdecl GetCtsStatus(void);
int cdecl GetDsrStatus(void);

/* 08/25/1999 add 5 function for COM1-4 */
int cdecl IsTxBufEmpty(int port);
int cdecl IsTxBufEmpty1(void);
int cdecl IsTxBufEmpty2(void);
int cdecl IsTxBufEmpty3(void);
int cdecl IsTxBufEmpty4(void);

/* 10/11/1999 add 2 functions for COM2 */
void cdecl SetCom2AutoDir(void);
void cdecl ResetCom2AutoDir(void);

/* 10/12/1999 add 2 functions for COM1 */
void cdecl SetCom1AutoDir(void);
void cdecl ResetCom1AutoDir(void);

/* 10/19/1999 add 5 function for COM1/2/3/4 change baudrate */
int cdecl SetBaudrate(int port,unsigned long baud);
int cdecl SetBaudrate1(unsigned long baud);
int cdecl SetBaudrate2(unsigned long baud);
int cdecl SetBaudrate3(unsigned long baud);
int cdecl SetBaudrate4(unsigned long baud);

/* 10/20/1999 */
/* (1) add 10 function for COM1/2/3/4 send/detect BREAK signal */
int cdecl SendBreak(int port,unsigned timems);
void cdecl SendBreak1(unsigned timems);
void cdecl SendBreak2(unsigned timems);
void cdecl SendBreak3(unsigned timems);
void cdecl SendBreak4(unsigned timems);
int cdecl IsDetectBreak(int port);
int cdecl IsDetectBreak1(void);
int cdecl IsDetectBreak2(void);
int cdecl IsDetectBreak3(void);
int cdecl IsDetectBreak4(void);

/* (2) Add 3 functions for clear COM1/2/3/4 output buffer */
int cdecl ClearTxBuffer(int port);
void cdecl ClearTxBuffer1(void);
void cdecl ClearTxBuffer2(void);
void cdecl ClearTxBuffer3(void);
void cdecl ClearTxBuffer4(void);

/* (3) Add 3 function for set COM1/2 FIFO trigger level, can be 1,4,8,14 */
int cdecl SetFifoTriggerLevel(int port,int level);
int cdecl SetCom1FifoTriggerLevel(int level);
int cdecl SetCom2FifoTriggerLevel(int level);

#ifdef __cplusplus
}
#endif
