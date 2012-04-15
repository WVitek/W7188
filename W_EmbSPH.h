// File W_EmbSPH.h - Embedded serial port Interrupt Handler (Am186)
// COM, COMBASE and INTTYPE parameters must be defined

startSysTicksCount();

U16 stat = inpw(COMBASE+OFFS_SPRT_STAT);

//Receive
while(stat & SPRT_STAT_RDR_ES)
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
    stat = inpw(COMBASE+OFFS_SPRT_STAT);
}

//Transmit (Tx)
U16 ctrl = inpw(COMBASE+OFFS_SPRT_CTL);
if(ctrl & SPRT_CTL_TMODE_ES) // if Tx enabled
    while(stat & SPRT_STAT_THRE_ES)    // while Tx holding register empty
        if(COM.TxB.RdP!=COM.TxB.WrP)
        {
            outpw( COMBASE+OFFS_SPRT_TX, COM.TxB.Data[COM.TxB.RdP++] );
            COM.TxB.RdP &= COMBUF_WRAPMASK;
            if(COM.TxB.RdP==COM.TxB.WrP)
                outpw(COMBASE+OFFS_SPRT_CTL,ctrl & ~SPRT_CTL_TXIE_ES); // disable transmit
            else if(COM.TxB.TriggerTX())
            {
                SYS::disable_sti();
                COM.TxB.Event.set();
                SYS::enable_sti();
            }
            stat = inpw(COMBASE+OFFS_SPRT_STAT);
        }
        else
        {
            outpw(COMBASE+OFFS_SPRT_CTL,ctrl & ~SPRT_CTL_TXIE_ES); // disable transmit
            break;
        }

outpw(INT_EOI,INTTYPE);
stopSysTicksCount(COM.ISRTicks);
