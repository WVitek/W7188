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
    dbg("\n\rSTART GPS_Time");
    U8 cnt=0;
    TIME sysTimeOfNMEA;
    U16 prevPPS = 0;
    com.clearRxBuf();
    com.setExpectation(0xFF,'\n');
    U16 noInputLine = 0;
    U16 cntNoPulse = 0;
    U16 prevSec = 60;
    TIME sysTimeOfPPS;
    while(!Terminated)
    {
        int res = com.receiveLine(buf,200,FALSE,'\n');
        com.setExpectation(0xFF,'\n');

        if(res<0)
        {
            if((++noInputLine & 0xF)==0)
                ConPrint("\n\rGPS info expected at COM3...");
            continue;
        }
        else if(noInputLine)
        {
            noInputLine = 0;
            prevPPS = (U16)sysTimeOfPPS;
            sysTimeOfPPS = comPPS.TimeOfHiCTS();;
            SYS::getSysTime(sysTimeOfNMEA);
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
            ConPrint("\n\rGPS: unrecognized message format: ");
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
            if((U16)sysTimeOfPPS == prevPPS)
            {
                if(++cntNoPulse>5)
                    ConPrint("\n\rGPS: No PPS pulse detected");
            }
            else
            {
                cntNoPulse = 0;
                U16 delta = (U16)sysTimeOfNMEA - (U16)sysTimeOfPPS;
    //            ConPrintf("\n\rGPS: jBugs=%d, dPPS=%d, delta=%d, NMEA_sec=%d",
    //                jBugs, (U16)sysTimeOfPPS-prevPPS, delta, sec);
                if(delta>999)
                    ConPrintf("\n\r%02d:%02d GPS: PPS pulse skipped (delta=%d)",hour,min,delta);
                else
                {
                    TIME offs = timeGPS - sysTimeOfPPS;
                    TIME change = abs64(offs - SYS::NetTimeOffset);
    //                if(900<change && change<1100 && ++cnt<32)
    //                {
    //                    ConPrintf("\n\rGPS: '1s jitter' bug avoided (d=%dms)",delta);
    //                    jBugs++;
    //                }
    //                else
                    {
                        if(900<change && change<1100)
                        //if(cnt>=32)
                            ConPrintf( "\n\r%02d:%02d:%02d GPS: second jitter BUG? (d=%dms, dPPS=%dms, prevSec=%d)",
                                hour, min, sec, delta, (U16)sysTimeOfPPS-prevPPS, prevSec );
    //                        ConPrintf("\n\rGPS: second jitter BUG? (d=%dms)",d);
                        cnt=0;
                        SYS::setNetTimeOffset(offs);
                        //ConPrintf("\n\rGPS: delta=%LX-%LX=%dms", sysTimeOfNMEA, sysTimeOfPPS, delta);
                    }
                }
            }
            prevSec = sec;
        }
    }
    dbg("\n\rSTOP GPS_Sync");
}

#endif // GPS_TIME_H_INCLUDED
