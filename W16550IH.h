// File W16550IH.h - 16550 Interrupt Handler
// COM, COMBASE, COMDIRBIT and INTTYPE parameters must be defined

#if defined(__WATCOMC__) //|| defined(__BORLANDC__)

startSysTicksCount();
int i;
U8 c;
BOOL SignalRxEvent=FALSE;
U8 Status;
do{
  switch(inpb(COMBASE+Iir)&0x07){

  //***** Modem Status Register change
  case 0:
        Status = inpb(COMBASE+Msr);
        if(Status & 0x01 == 0) // CTS is changed?
            break;
        if(Status & 0x10){
            if(COM.Flags & CF_HWFLOWCTRL) {
                COM.TxPaused=FALSE;
                outp(COMBASE+Ier, 0x0B); // enable TX | RX interrupt
            }
            COM.timeOfHiCTS = SYS::SystemTime;
            if(!COM.EvtHiCTS.IsSignaled()){
                SYS::disable_sti();
                COM.EvtHiCTS.set();
                SYS::enable_sti();
            }
        }
        else{
            if(COM.Flags & CF_HWFLOWCTRL){
                COM.TxPaused=TRUE;
                outp(COMBASE+Ier, 0x09); // enable RX interrupt only
            }
            if(!COM.EvtLoCTS.IsSignaled()){
                SYS::disable_sti();
                COM.EvtLoCTS.set();
                SYS::enable_sti();
            }
        }
        break;

  //***** No interrupt to be service
  case 1:
    goto end;

  //***** can send data
  case 2:
    PERFINC(COM._TC);
    if(COM.TxB.RdP==COM.TxB.WrP)
      goto _QueueIsEmpty;
    #ifdef __SoftAutoDir
    set485DirTx(COMDIRBIT);
    #endif
    for(i=16; i>0; i--){
      PERFINC(COM._TB);
      outp(COMBASE,COM.TxB.Data[COM.TxB.RdP]);
      (++COM.TxB.RdP)&=COMBUF_WRAPMASK;
      if(COM.TxB.RdP==COM.TxB.WrP) goto _QueueIsEmpty;
    }
    if((COM.TxB.WrP-COM.TxB.RdP)&COMBUF_WRAPMASK>=COMBUF_TXLIMIT)
      break;
    if(!COM.TxB.Event.IsSignaled()){
      // Tx Trigger Event
      SYS::disable_sti();
      COM.TxB.Event.set();
      SYS::enable_sti();
}
    break;
  _QueueIsEmpty:
  #ifdef __SoftAutoDir
    if(COM.Flags & CF_AUTODIR){
      if(inpb(COMBASE+Lsr) & 0x40){ // transmitter shift register empty?
        outp(COMBASE+Ier, 0x09); // enable RX interrupt only
        set485DirRx(COMDIRBIT);
      }
      else{
        outp(COMBASE+Ier, 0x0B); // enable TX | RX interrupt
        goto end;
      }
    }
    else
  #endif
      outp(COMBASE+Ier, 0x09);
    break;
  //***** receive data
  case 4:
    PERFINC(COM._RC);
    U8 LSR;
    LSR=inpb(COMBASE+Lsr);
    if(LSR & 0x0E) COM.err|=(LSR & 0x0E);
    while(LSR & 0x01){
      PERFINC(COM._RB);
      c=inpb(COMBASE);  // receive next char
      COM.RxB.Data[COM.RxB.WrP]=c;
#ifdef __COM_STATUS_BUF
      COM.RxB.Status[COM.RxB.WrP]=LSR;
#endif
      ++COM.RxB.WrP &= COMBUF_WRAPMASK;
      if(COM.RxB.WrP==COM.RxB.RdP){
        // Queue Overflow
        ++COM.RxB.RdP &= COMBUF_WRAPMASK;
        COM.err|=0x01;
      }
      if((c & COM.RxB.ExpMask)==COM.RxB.ExpChar){
        // Expected char received
        if(COM.RxCnt==0 || --COM.RxCnt==0){
//          COM.RxB.ExpMask=0;
//          COM.RxB.ExpChar=1;
          SignalRxEvent=TRUE;
        }
      }
      LSR=inpb(COMBASE+Lsr);
    };
    // check Rx trigger
    if(!SignalRxEvent && (COM.RxB.WrP-COM.RxB.RdP)& COMBUF_WRAPMASK > COMBUF_RXLIMIT )
      SignalRxEvent=TRUE;
    if(SignalRxEvent && !COM.RxB.Event.IsSignaled()) {
      SYS::disable_sti();
      COM.RxB.Event.set();
      SYS::enable_sti();
    }
    break;
  case 6:
    inpb(COMBASE+Lsr);
    break;
  default:
    // error status
    goto end;
  }
}while(1);
end:
outpw(INT_EOI,INTTYPE);   // send EOI to CPU
stopSysTicksCount(COM.ISRTicks);

//*
#elif defined(__BORLANDC__)

startSysTicksCount();
do{
  switch(inpb(COMBASE+Iir)&0x07){
  case 0:
    inpb(COMBASE+Msr);
    break;
  case 2: // can send data
    _asm{
      ASMPERFINC(word ptr COM._TC)
      mov   bx,word ptr COM.TxB.RdP
      cmp   bx,word ptr COM.TxB.WrP
      je    _QueueIsEmpty
      mov   cx,16
      // Set RS-485 direction to transmit
#ifdef __SoftAutoDir
      mov   dx,ComDirPort
      in    al,dx
      or    al,COMDIRBIT
      out   dx,al
#endif
      mov   dx,COMBASE
  ASMLABEL(SendLoop)
      ASMPERFINC(word ptr COM._TB)
      mov   al,byte ptr COM.TxB.Data[bx]
      out   dx,al
      inc   bx
      and   bx,COMBUF_WRAPMASK
      cmp   bx,word ptr COM.TxB.WrP
      je    _QueueIsEmpty
      loop  SendLoop
      mov   word ptr COM.TxB.RdP,bx
      mov   ax,word ptr COM.TxB.WrP
      sub   ax,bx
      and   ax,COMBUF_WRAPMASK
      cmp   ax,COMBUF_TXLIMIT
      jb    TxTriggerEvent
    }
    break;
  TxTriggerEvent:
    SYS::disable_sti();
    COM.TxB.Event.set();
    SYS::enable_sti();
    break;
  _QueueIsEmpty:
    _asm mov   word ptr COM.TxB.RdP,bx
#ifdef __SoftAutoDir
    if(COM.Flags & CF_AUTODIR){
      if(inpb(COMBASE+Lsr) & 0x40){ // transmitter shift register empty?
        outp(COMBASE+Ier, 0x09);
        set485DirRx(COMDIRBIT);
      }
      else{
        outp(COMBASE+Ier, 0x0B);
        goto end;
      }
    }
    else
#endif
      outp(COMBASE+Ier, 0x09);
    break;
  case 4: // receive data
    BOOL SignalRxEvent=FALSE;
    asm{
      ASMPERFINC(word ptr COM._RC)
      mov   bx,word ptr COM.RxB.WrP
  ASMLABEL(ReceiveLoop)
      ASMPERFINC(word ptr COM._RB)
      mov   dx,COMBASE
      in    al,dx
      mov   byte ptr COM.RxB.Data[bx],al
      inc   bx
      and   bx,COMBUF_WRAPMASK
      cmp   bx,word ptr COM.RxB.RdP
      je    IsQueueOverflow
  ASMLABEL(ExpectationCheck)
      and   al,byte ptr COM.RxB.ExpMask
      cmp   al,byte ptr COM.RxB.ExpChar
      je    IsExpectedChar
  ASMLABEL(CheckReadAgain)
      mov   dx,COMBASE+Lsr
      in    al,dx
      test  al,1
      jnz   ReceiveLoop
      // CheckRxTrigger
      mov   ax,bx
      sub   ax,word ptr COM.RxB.RdP
      and   ax,COMBUF_WRAPMASK
      cmp   ax,COMBUF_RXLIMIT
      jbe   _Else
      mov   SignalRxEvent,TRUE
  ASMLABEL(_Else)
      mov   word ptr COM.RxB.WrP,bx // store cached value
    }
    if(SignalRxEvent && !COM.RxB.Event.IsSignaled()) {
      SYS::disable_sti();
      COM.RxB.Event.set();
      SYS::enable_sti();
    }
    break;
    IsQueueOverflow:
      asm{
        dec   word ptr COM.RxB.RdP
        and   word ptr COM.RxB.RdP,COMBUF_WRAPMASK
        mov   byte ptr COM.err,TRUE
        jmp   ExpectationCheck
    ASMLABEL(IsExpectedChar)
        mov   word ptr COM.RxB.WrP,bx // store cached value
      }
      if(COM.RxCnt==0 || --COM.RxCnt==0){
//        COM.RxB.ExpMask=0;
//        COM.RxB.ExpChar=1;
        SignalRxEvent=TRUE;
      }
      asm mov   bx,word ptr COM.RxB.WrP
      goto CheckReadAgain;
  case 6:
    inpb(COMBASE+Lsr);
    break;
  case 1: // No interrupt to be service
    goto end;
  default:
    // error status
    goto end;
  }
}while(1);
end:
outpw(INT_EOI,INTTYPE);   // send EOI to CPU
stopSysTicksCount(COM.ISRTicks);
//*/
#endif // __BORALNDC__
