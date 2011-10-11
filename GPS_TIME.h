#ifndef GPS_TIME_H_INCLUDED
#define GPS_TIME_H_INCLUDED

//class THREAD_GPS
class THREAD_GPS : public THREAD {
public:
  void execute();
};

// 7188.COM1.RTS (pin 7) === 1 PPS of GPS-721 module
// 7188.COM1.CTS (pin 8) === DO.PWR of GPS-721 module
// 7188.COM3.Rx === Tx if GPS-721
// 7188.COM3.Tx === Rx of GPS-721
void THREAD_GPS::execute()
{
    COMPORT& com = GetCom(3);
    COMPORT& comPPS = GetCom(1);
    EVENT& evtPPS = comPPS.EventHiCTS();
    EVENT& evtRx = com.RxEvent();
    com.install(9600);
    U8 buf[256];
    dbg("\n\rGPS sync started");
    U8 cnt=0;
    TIME sysTimeOfNMEA;
    U16 prevPPS = 0;
    com.clearRxBuf();
    com.setExpectation(0xFF,'\n');
    BOOL noInputLine = FALSE;
    U8 cntNoPulse = 0;
    while(!Terminated)
    {
        int res = com.receiveLine(buf,100,FALSE,'\n');
        com.setExpectation(0xFF,'\n');
        if(res<0)
        {
            noInputLine = TRUE;
            continue;
        }
        else if(noInputLine)
        {
            SYS::getSysTime(sysTimeOfNMEA);
            noInputLine = FALSE;
        }
        if(res<0 || buf[0]!='$' || buf[1]!='G' || buf[3]!='R' || buf[4]!='M'|| buf[5]!='C')
            continue;
        U8 checkSum = 0;
        U8* pTime = NULL;
        U8* pDate = NULL;
        U8* pCS = NULL;
        U8* pb = &(buf[1]);

        int iTail = 0;
        while(true)
        {   // calculate checksum & search asterrisk
            U8 c = *pb++;
            if(c==0)
                break;
            if(c=='*')
            { pCS = pb; break; }
            checkSum ^= c;
            if(c==',')
            {
                iTail++;
                if(iTail==1)
                    pTime=pb;
                else if(iTail==9)
                    pDate=pb;
            }
        }
        if(pCS==NULL)
        {
            ConPrint("\n\rGPS: unrecognized message format");
            ConPrint((char const*)buf);
            continue;
        }
        if(checkSum!=FromHexStr(pCS,2))
        { ConPrint("\n\rGPS: wrong checksum of message"); continue; }
        // parse date & time
        U16 year = (U16)FromDecStr(&(pDate[4]),2) + 2000;
        U16 month = (U16)FromDecStr(&(pDate[2]),2);
        U16 day = (U16)FromDecStr(&(pDate[0]),2);
        U16 hour = (U16)FromDecStr(&(pTime[0]),2);
        U16 min = (U16)FromDecStr(&(pTime[2]),2);
        U16 sec = (U16)FromDecStr(&(pTime[4]),2);
        TIME timeGPS;
        if(SYS::TryEncodeTime(year,month,day,hour,min,sec,timeGPS))
        {
            TIME sysTimeOfPPS = comPPS.TimeOfHiCTS();
            if((U16)sysTimeOfPPS == prevPPS)
            {
                if(++cntNoPulse>2)
                    ConPrint("\n\rGPS: No PPS pulse detected");
                continue;
            }
            cntNoPulse = 0;
            prevPPS = (U16)sysTimeOfPPS;
            TIME delta = sysTimeOfNMEA - sysTimeOfPPS;
            if(delta<=-999 || +999<=delta)
                ConPrintf("\n\rGPS: PPS pulse rejected (d=%Ld-%Ld=%Ldms)",
                    sysTimeOfNMEA, sysTimeOfPPS, delta);
            else
            {
                S16 d = (S16)delta;
                TIME offs = timeGPS - sysTimeOfPPS + ((d<0)? 1000 : 0);
                TIME change = abs64(offs - SYS::NetTimeOffset);
                if(900<change && change<1100 && ++cnt<32)
                    ConPrintf("\n\rGPS: '1s jitter' bug avoided (d=%dms)",d);
                else
                {
                    if(cnt>=32)
                        ConPrintf("\n\rGPS: second jitter BUG? (d=%dms)",d);
                    cnt=0;
                    SYS::setNetTimeOffset(offs);
                    //ConPrintf("\n\rGPS: delta=%LX-%LX=%dms", sysTimeOfNMEA, sysTimeOfPPS, delta);
                }
                //ConPrintf("\n\rOffs=%Ld, delta=%d", offs, delta );
            }
        }
    }
    dbg("\n\rGPS sync stopped");
}

#endif // GPS_TIME_H_INCLUDED
