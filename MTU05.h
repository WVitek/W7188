#include "WCRCs.hpp"

class THREAD_POLLING : public THREAD
{
    U16 pollPort;
    U32 baudRate;
    //MODULES *modules;
    //PU_LIST *pollList;
public:
#ifndef __PollPort
#define __PollPort 2
#endif
    THREAD_POLLING(U16 pollPort, U32 baudRate//, MODULES &m=Modules, PU_LIST &pl=PollList
    )
    {
        this->pollPort=pollPort;
        this->baudRate=baudRate;
        //modules=&m;
        //pollList=&pl;
    }
    BOOL scanComplete;
    void execute();
};

void THREAD_POLLING::execute()
{
    COMPORT& RS485=GetCom(pollPort);
    RS485.install(baudRate);
    RS485.setDataFormat(8,pkMark,1);
    scanComplete = TRUE;
#ifdef __COM_STATUS_BUF
/*
    // MTU simple emulator
    const int bufSize = 64;
    U8 buf[bufSize];
    U8 status[1];
    U8 mix=0;
    const int nSensors = 7;
    U8 nSyncMeasures[nSensors];
    BOOL syncMeasureInProcess[nSensors];
    TIME lastSyncTime = -1;
    while(!Terminated)
    {
        U16 cnt = RS485.read(buf,status,1,50);
        U8 cmd = buf[0];
        if(cnt==0)
            continue;
        U8 st = status[0] & 0x0E;
        ConPrintf("\n\r%02X: %02X", st, buf[0]);
        if(st!=0)
        {
            continue;
        }
        PollStatAdd(1,0);
        U8 sid = cmd & 0x0F;
        if(sid>=nSensors)
            continue;
        U8 size = 0;
        BOOL use_crc = false;
        cmd &= 0xF0;
        switch(cmd)
        {
            case 0x40: // ??? check modbus support
            case 0xC0: // check presence
                buf[0]=0xAA; size = 1;
                break;
            case 0x80: // query physical value
                buf[0]=0xBC; buf[1]=0x71; buf[2]=0x12 ^ (mix++ & 0x0F);
                size = 3; use_crc = TRUE;
                break;
            case 0xA0: // query codes without CRC
            case 0xB0: // query codes with CRC
                buf[0]=0x2D; buf[1]=0x31 ^ (mix & 0x0F);
                buf[2]=0xC2; buf[3]=0xF2 ^ (mix++ & 0x0F) ^ 0x0F;
                size = 4; use_crc = cmd==0xB0;
                break;
            case 0xE0: // read synchronized measures buffer
                {
                    TIME time;
                    SYS::getSysTime(time);
                    U8 nM = nSyncMeasures[sid];
                    if(time-lastSyncTime>33)
                    {
                        syncMeasureInProcess[sid] = false;
                        if(++nM>10) nM=10;
                    }
                    nSyncMeasures[sid]=0;
                    size = 1+nM*4;
                    U8 *p = buf;
                    *p = nM;
                    while(nM--)
                    {
                        *++p=0x2D; *++p=0x31 ^ (mix & 0x0F);
                        *++p=0xC2; *++p=0xF2 ^ (mix++ & 0x0F) ^ 0x0F;
                    }
                    use_crc = TRUE;
                }
                break;
            case 0xF0: // synhronize measurement
                {
                    TIME time;
                    SYS::getSysTime(time);
                    if(lastSyncTime<0)
                        lastSyncTime = time;
                    else if(time-lastSyncTime>33)
                    {
                        lastSyncTime = time;
                        for(int i=0; i<nSensors; i++)
                            if(!syncMeasureInProcess[i])
                                syncMeasureInProcess[i]=TRUE;
                            else if(++nSyncMeasures[i]>10)
                                nSyncMeasures[i]=10;
                    }
                }
                break;
            case 0x00: // modbus
                {
                    // read all bytes of modbus RTU query
                    int n = 1;
                    while(n<bufSize)
                    {
                        int i = RS485.read(&(buf[n]),1,1);
                        if(i==0) break;
                        else n++;
                    }
                    ConPrint("\n\rMB<-");
                    ConPrintHex(buf,n);
                    // check CRC
                    BOOL use_mtu_crc;
                    U16 crc16 = CRC16(buf, n-2, CRC16_MTU05);
                    if(crc16!=*(U16*)&(buf[n-2]))
                    {
                        U16 crc_mb = CRC16(buf, n-2, CRC16_ModBus);
                        if(crc_mb!=*(U16*)&(buf[n-2]))
                        {
                            ConPrintf("\n\rMB: wrong CRC (%04X|%04X)",crc16,crc_mb);
                            continue;
                        }
                        else use_mtu_crc = FALSE;
                    }
                    else use_mtu_crc = TRUE;
                    // get requested addr & count
                    U32 what;
                    U8 *pw = (U8*)&what;
                    pw[0]=buf[5]; pw[1]=buf[4]; pw[2]=buf[3]; pw[3]=buf[2];
                    char* res = NULL;
                    n = 0;
                    if(buf[1]==0x04) // Read Input Registers
                        switch(what)
                        {
                        case 0x00000001ul:
                            res = "\x02\x03\xFF"; n=3;
                            break;
                        case 0x00300002:
                            res = "\x04\x00\x00\x00\x00"; n=5;
                            break;
                        case 0x00340002ul:
                            res = "\x04\x41\xC8\x00\x00"; n=5;
                            break;
                        case 0x00040001ul:
                            res = "\x02\xFF\xFF"; n=3;
                            break;
                        case 0x000C0002ul:
                            res = "\x04\xC0\x07\x52\x09"; n=5;
                            break;
                        case 0x00100002ul:
                            res = "\x04\xB7\xC5\x5F\xAA"; n=5;
                            break;
                        case 0x00140002ul:
                            res = "\x04\xB0\xD0\x7D\xC5"; n=5;
                            break;
                        case 0x00180002ul:
                            res = "\x04\x39\xD6\xA3\xE4"; n=5;
                            break;
                        case 0x001C0002ul:
                            res = "\x04\x30\x03\x8D\xF1"; n=5;
                            break;
                        case 0x00200002ul:
                            res = "\x04\x29\x9D\xB6\xEA"; n=5;
                            break;
                        case 0x00240002ul:
                            res = "\x04\xB0\x4B\x3D\x7A"; n=5;
                            break;
                        case 0x00280002ul:
                            res = "\x04\x29\x10\x81\x75"; n=5;
                            break;
                        case 0x002C0002ul:
                            res = "\x04\xA0\xA5\x06\x06"; n=5;
                            break;
                        case 0x00020001ul:
                            res = "\x02\x08\xFF"; n=3;
                            break;
                        case 0x00500001ul:
                            //res = "\x02\x01\xFF"; n=3;
                            res = "\x02\x02\xFF"; n=3;
                            break;
                        case 0x00500002ul:
                            res = "\x02\x01\x00"; n=3;
                            break;
                        case 0x00540002ul:
                            res = "\x04\x00\x00\x05\x99"; n=5;
                            break;
                        case 0x00480002ul:
                            res = "\x04\x15\x81\x78\x00"; n=5;
                            break;
                        case 0x004C0002ul:
                            res = "\x04\x15\x81\x78\x00"; n=5;
                            break;
                        }
                    else if(buf[1]==0x10)
                    { res = &(buf[2]); n = 4; }
                    if(res==NULL)
                    {
                        ConPrint("\n\rMB: unsupported command ");
                        ConPrintHex(buf,n-2);
                        continue;
                    }
                    //n = strlen(res);
                    memcpy(&(buf[2]), res, n);
                    crc16 = CRC16(buf, 2+n, use_mtu_crc ? CRC16_MTU05 : CRC16_ModBus);
                    *(U16*)&(buf[2+n]) = crc16;
                    RS485.write(buf,2+n+2);
                    PollStatAdd(0,1);
                    ConPrint("\n\rMB->");
                    ConPrintHex(buf,2+n+2);
                    continue;
                }
                break;
        }
        if(size==0)
            continue;
        PollStatAdd(0,1);
        if(use_crc)
            buf[size] = CRC8_CCITT_get(buf,size++);
        RS485.setDataFormat(8,pkSpace,1);
        RS485.write(buf,size);
        while(!RS485.IsTxOver())
            SYS::sleep(1);
        RS485.setDataFormat(8,pkMark,1);
    }
/*/
    U8 prevStatus = 0xFF;
    const int BufSize = 32;
    U8 buffer[BufSize];
    U8 status[BufSize];
    Realtime=TRUE;
    U16 prevCnt = 0;
    TIME prevTime = 0;
    while(!Terminated)
    {
        TIME time;
        SYS::getSysTime(time);
        U16 cnt = RS485.read(buffer,status,BufSize,1);
        if(cnt==0)
        {
            if(prevCnt>0)
            {
                ConPrint("\n\r");
                prevCnt = 0;
            }
            prevStatus = 0xFF;
            continue;
        }
        prevCnt = cnt;
        TIME bigDelta = time - prevTime;
        U16 delta;
        if(bigDelta>=10000)
            delta = 9999;
        else delta = (U16)bigDelta % 10000;
        ConPrintf("\n\r[%dms]",delta);
        int iPrev = 0;
        for(int i=0; i<cnt; i++)
        {
            U8 st = status[i] & 0x0E;
            if(prevStatus!=st)
            {
                if(i>iPrev)
                    ConPrintHex(buffer+iPrev,i-iPrev);
                ConPrintf("\n\r%02X: ", st);
                prevStatus = st;
                iPrev = i;
            }
        }
        if(iPrev<cnt)
            ConPrintHex(buffer+iPrev,cnt-iPrev);
        prevTime = time;
    }
//*/
#else
    U8 Query[16];
    U8 Resp[256];
    dbg3("\n\rMTU polling started (COM%d, %ld)", pollPort, baudRate);
//*
#define use_mtu_crc TRUE
    //***** Scan RS-485 bus
    U16 detectedMTU;
    while(!Terminated)
    {
        SYS::sleep(3000);
        dbg("\n\rMTU-05: searching...");
        // scan all possible MTUs twice
        U16 detected[2];
        RS485.clearRxBuf();
        for(int j=0; j<2; j++)
        {
            U16 bits = 0;
            for(int i=0; i<=15; i++)
            {
                U8 ans;
                U16 cnt;
                // Check presence and set modbus mode
                RS485.writeChar(0xC0 | i);
                cnt = RS485.read(&ans,1,10);
                if(cnt==1 && ans==0xAA)
                {
                    bits |= 1<<i;
                    dbg2("\n\rMTU-05: #%d detected", i);
                    SYS::sleep(20);
                }
            }
            detected[j]=bits;
        }
        if(detected[0]!=0 && detected[0]==detected[1])
        {
            detectedMTU = detected[0];
            break;
        }
    }
    // create MTU objects
    PU_ADC_MTU *MTUs[16];
    int nMTU = 0;
    for(int i=0; i<=15; i++)
        if((detectedMTU & (1<<i))!=0)
            MTUs[nMTU++] = new PU_ADC_MTU(i);
    //dbg("\n\rMTU-05: getting coeffs...");
    {
        Query[1] = 0x04;
        Query[2] = 0x00;
        Query[4] = 0x00;
        Query[5] = 0x02;
        U16 noCoeffMTU = detectedMTU;
        while(noCoeffMTU!=0)
        {
            // set modbus mode for present MTUs
            for(int i=0; i<=15; i++)
            {
                if((detectedMTU & (1<<i))==0)
                    continue;
                RS485.writeChar(0x40 | i);
                RS485.read(&Resp,1,10);
            }
            // Query coeffs for transformation raw (p,t) values to physical P value
            for(int k = 0; k<nMTU; k++)
            {
                PU_ADC_MTU *mtu = MTUs[k];
                int i = mtu->BusNum;
                if((noCoeffMTU & (1<<i))==0)
                    continue;
//                dbg2("\n\rMTU #%X : get coeffs",i);
                SYS::sleep(200);
                // Get pressure calculation coeffs
                BOOL ok = TRUE;
                Query[0] = i;
                for(int j=0; j<9; j++)
                {
                    if(mtu->coeffs.C[j]!=0)
                        continue;
                    if(Terminated)
                        return;
                    Query[3] = 0x0C + (j<<2);
//                    for(int k = (j==0) ? 2 : 1; k>0; k--)
                    {
                        U16 crc16 = CRC16(Query, 6, use_mtu_crc ? CRC16_MTU05 : CRC16_ModBus);
                        *(U16*)&(Query[6]) = crc16;
                        RS485.clearRxBuf();
                        RS485.write(Query,8);
                        U16 n = RS485.read(Resp,9,300);
//                        if(n<9 && k==2)
//                        { use_mtu_crc = FALSE; continue; }
                        if(n!=9)
                        {
                            ConPrintf("\n\rMTU #%X: Coeff[%d] FAILED (RespLen:%d!=9)",i,j,n);
                            SYS::sleep(500);
                            ok = FALSE; //break;
                        }
                        crc16 = CRC16(Resp, 7, use_mtu_crc ? CRC16_MTU05 : CRC16_ModBus);
                        if(crc16 != *(U16*)&(Resp[7]))
                        {
                            ConPrintf("\n\rMTU #%X: Coeff[%d] FAILED (CRC)",i,j);
                            SYS::sleep(500);
                            ok = FALSE; //break;
                        }
                        U32 coeff = swap4bytes(*(U32*)&(Resp[3]));
                        mtu->coeffs.setCoeff(j,coeff);
//                        if(k==2) break;
                    }
                    //if(!ok) break;
                }
                if(ok)
                {
                    ConPrintf("\n\rMTU #%X: coeffs OK", i);
                    noCoeffMTU &= ~(1<<i);
                }
            }
        }
    }
    scanComplete = TRUE;
    // disable modbus mode
    SYS::sleep(3000);
    // polling
    int i=nMTU-1;
    Realtime=TRUE;
    while(!Terminated)
    {
        SYS::sleep(toTypeNext + 40);
        // start new measurement
        RS485.writeChar(0xF0);
        TIME netTime;
        SYS::getNetTime(netTime);
        // wait while measure in process
        SYS::sleep(19);
        PollStatAdd(1,0);
        RS485.clearRxBuf();
        for(int i=0; i<nMTU; i++)
        {
            // archiving previous
            PU_ADC_MTU *mtu = MTUs[i];
            // choose next MTU for reading
//            if(--i < 0)
//                i=nMTU-1;
            int num = mtu->BusNum;
            // read buffer with values
            RS485.writeChar(0xE0 | num);
            mtu->doSample(netTime-ADC_Period);
            U8 ans = 0xFF;
            U16 cnt = RS485.read(&ans,1,3);
            if(cnt!=1 || ans>10)
            {
                dbg3("\n\rMTU #%X : Buffer reading FAILED (nRes=%X)",num,ans);
                //PollStatAdd(1,0);
                continue;
            }
            U16 size = ans*4+1;
            cnt = RS485.read(Resp+1,size,size);
            if(cnt != size)
            {
                ConPrintf("\n\rMTU #%X : Buffer reading FAILED (wrong answer size, %d!=%d)",num,cnt,size);
                //PollStatAdd(1,0);
                continue;
            }
            PollStatAdd(0,ans);
            Resp[0]=ans;
            mtu->response(Resp);
        }
    }
/*/
    scanComplete = TRUE;
    //ile(!Terminated && !SYS::TimeOk)
    //  SYS::sleep(333);
    Realtime = TRUE;
    int j=0;
    //485.writeChar(0xF0);
    //S::sleep(toTypeNext + 1000);
    while(!Terminated)
    {
        //SYS::sleep(toTypeNext + 40);
        //int i = (j++ & 1) ? 3 : 4;
        //RS485.writeChar(0xF0);
        SYS::sleep(19);
        RS485.clearRxBuf();
        for(int i=3; i<=4; i++)
        {
            RS485.writeChar(0xE0 | i);
            //SYS::sleep(13);
            PollStatAdd(1,0);
            U8 nResults;
            U16 cnt = RS485.read(&nResults,1,10);
            if(cnt==1)
            {
                U16 size = (nResults<<2) + 1;
                cnt = RS485.read(Resp,size,100);
                if(cnt==size)// && 0<nResults && nResults<=10)
                {
                    PollStatAdd(0,nResults);
                    //break;
                }
                //else PollStatAdd(1,0);
            }
        }
    }
//*/
    dbg("\n\rMTU polling stopped");
#endif
}

