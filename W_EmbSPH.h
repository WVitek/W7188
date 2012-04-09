// File W_EmbSPH.h - Embedded serial port Interrupt Handler (Am186)
// COM, COMBASE and INTTYPE parameters must be defined

//startSysTicksCount();

int status=inpw(COMBASE+OFFS_SPRT_STAT);
int ctrl=inpw(COMBASE+OFFS_SPRT_CTL);
outpw(COMBASE+OFFS_SPRT_CTL,NoIntCtrlMask);

//Receive
if(status & SPRT_STAT_RDR_ES)
{
    U8 c=(U8)inpw(COMBASE+OFFS_SPRT_RX); // inpw
    COM.RxB.Data[COM.RxB.WrP]=c;
    if((c & COM.RxB.ExpMask)==COM.RxB.ExpChar)
    {
        if(COM.RxCnt==0 || --COM.RxCnt==0)
        {
            SYS::disable_sti();
            COM.RxB.Event.set();
            SYS::enable_sti();
        }
    }
    COM.RxB.WrP=(COM.RxB.WrP+1) & COMBUF_WRAPMASK;
    if(COM.RxB.TriggerRX())
    {
        SYS::disable_sti();
        COM.RxB.Event.set();
        SYS::enable_sti();
    }
}

//Transmit
if((ctrl & SPRT_CTL_TMODE_ES) && (status & SPRT_STAT_THRE_ES))
{
    if(COM.TxB.RdP!=COM.TxB.WrP)
    {
        outpw( COMBASE+OFFS_SPRT_TX, COM.TxB.Data[COM.TxB.RdP++] );
        COM.TxB.RdP &= COMBUF_WRAPMASK;
        if(COM.TxB.RdP==COM.TxB.WrP)
            ctrl &= ~SPRT_CTL_TXIE_ES;
        else if(COM.TxB.TriggerTX())
        {
            SYS::disable_sti();
            COM.TxB.Event.set();
            SYS::enable_sti();
        }
    }
    else ctrl &= ~SPRT_CTL_TXIE_ES;
}
outpw(COMBASE+OFFS_SPRT_CTL,ctrl);
outpw(INT_EOI,INTTYPE);
//stopSysTicksCount(COM.ISRTicks);
