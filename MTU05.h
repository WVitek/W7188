int __CntMTU_Query=0, __CntMTU_Answer=0;
inline void MTU_StatAdd(int Qry, int Ans) {
  SYS::cli();
  __CntMTU_Query +=Qry;
  __CntMTU_Answer+=Ans;
  SYS::sti();
}
inline void MTU_StatRead(int &Qry, int &Ans)
{
  SYS::cli();
  Qry=__CntMTU_Query;  __CntMTU_Query =0;
  Ans=__CntMTU_Answer; __CntMTU_Answer=0;
  SYS::sti();
}

#include "WCRCs.hpp"

class THREAD_POLL_MTU : public THREAD
{
    U16 pollPort;
    U32 baudRate;
public:
    U8 count;
    U8 addrs[16];

    THREAD_POLL_MTU(U16 pollPort, U32 baudRate)
    {
        this->pollPort=pollPort;
        this->baudRate=baudRate;
    }
    void execute();
};

void THREAD_POLL_MTU::execute()
{
    COMPORT& RS485=GetCom(pollPort);
    RS485.install(baudRate);
    RS485.setDataFormat(8,pkMark,1);
    S(0x01);
#ifdef __COM_STATUS_BUF
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
        MTU_StatAdd(1,0);
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
                    //TIME time;
                    //SYS::getSysTime(time);
                    U8 nM = nSyncMeasures[sid];
                    //if(time-lastSyncTime>33)
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
                    MTU_StatAdd(0,1);
                    ConPrint("\n\rMB->");
                    ConPrintHex(buf,2+n+2);
                    continue;
                }
                break;
        }
        if(size==0)
            continue;
        MTU_StatAdd(0,1);
        if(use_crc)
            buf[size] = CRC8_CCITT_get(buf,size++);
        RS485.setDataFormat(8,pkSpace,1);
        RS485.write(buf,size);
        while(!RS485.IsTxOver())
            SYS::sleep(1);
        RS485.setDataFormat(8,pkMark,1);
    }
#else // __COM_STATUS_BUF
    U8 Qry_MTU[16];
    U8 Rsp_MTU[64];
    dbg3("\n\rSTART MTU_Poll @ COM%d:%ld\n\rMTUs: ", pollPort, baudRate);
    ConPrintHex(addrs, count);
#define use_mtu_crc TRUE
    if(count==0)
    {   //***** detect MTUs
        U16 detectedMTU = 0;
        dbg("\n\rMTU: search...");
        {
            U16 k = count ? 2 : 65535;
            while(!Terminated && k-- > 0)
            {
                S(0x02);
                // scan all possible MTUs twice
                //U16 detected[2];
                RS485.clearRxBuf();
                U16 bits = 0;
                for(int i=0; i<=15; i++)
                {
                    S(0x03);
                    // Check presence and set modbus mode
                    RS485.writeChar(0xC0 | i);
                    U8 ans;
                    U16 cnt = RS485.read(&ans,1,10);
                    S(0x04);
                    if(cnt==1 && ans==0xAA)
                    {
                        U16 bit = 1<<i;
                        bits |= bit;
                        if((detectedMTU & bit) == 0)
                            dbg2("\n\rMTU: #%d detected", i);
                        SYS::sleep(20);
                    }
                }
                S(0x05);
                if(bits==0)
                {
                    SYS::sleep(3000);
                    continue;
                }
                if(detectedMTU==bits)
                    break;
                detectedMTU = bits;
            }
        }
        S(0x06);
        for(int i=0; i<=15; i++)
            if((detectedMTU & (1<<i))!=0)
                addrs[count++] = i;
    }

    // create MTU objects
    PU_ADC_MTU *MTUs[16];
    U16 noCoeffMTU = 0;
    char* coeffsFileName = "MTU_#.cfs";
    {
        for(int i=0; i<count; i++)
        {
            MTUs[i] = new PU_ADC_MTU(addrs[i]);
            coeffsFileName[4]=hex_to_ascii[addrs[i]];
            FILE_DATA* pFD = SYS::FileInfoByName(coeffsFileName);
            // Try to load saved coeffs
            if(pFD!=NULL && pFD->size==sizeof(MTU_Coeffs))
                MTUs[i]->coeffs = *(MTU_Coeffs*)pFD->addr;
            else noCoeffMTU |= 1<<addrs[i];
        }
    }
    S(0x07);
    if(noCoeffMTU!=0)
    {   // Load coeffs from MTUs
        Qry_MTU[1] = 0x04;
        Qry_MTU[2] = 0x00;
        Qry_MTU[4] = 0x00;
        Qry_MTU[5] = 0x02;
        while(noCoeffMTU!=0 && !Terminated)
        {
            // set modbus mode for all possible MTUs
            S(0x08);
            for(int i=0; i<=15; i++)
            {
                RS485.writeChar(0x40 | i);
                RS485.read(&Rsp_MTU,1,10);
            }
            // Query coeffs for transformation raw (p,t) values to physical P value
            S(0x09);
            for(int k = 0; k<count && !Terminated; k++)
            {
                PU_ADC_MTU *mtu = MTUs[k];
                int i = mtu->BusNum;
                if((noCoeffMTU & (1<<i))==0)
                    continue;
                S(0x0A);
                SYS::sleep(200);
                // Get pressure calculation coeffs
                //BOOL ok = TRUE;
                Qry_MTU[0] = i;
                for(int j=0; j<9 && !Terminated;)
                {
                    if(mtu->coeffs.C[j]!=0)
                    { j++; continue; }
                    S(0x0B);
                    Qry_MTU[3] = 0x0C + (j<<2);
                    {
                        U16 crc16 = CRC16(Qry_MTU, 6, use_mtu_crc ? CRC16_MTU05 : CRC16_ModBus);
                        *(U16*)&(Qry_MTU[6]) = crc16;
                        RS485.clearRxBuf();
                        RS485.write(Qry_MTU,8);
                        U16 n = RS485.read(Rsp_MTU,9,300);
                        if(n!=9)
                        {
                            ConPrintf("\n\rMTU #%d: Coeff[%d] ERROR RespLen:%d!=9",i,j,n);
                            SYS::sleep(200);
                            continue;
                            //ok = FALSE; //break;
                        }
                        S(0x0c);
                        crc16 = CRC16(Rsp_MTU, 7, use_mtu_crc ? CRC16_MTU05 : CRC16_ModBus);
                        if(crc16 != *(U16*)&(Rsp_MTU[7]))
                        {
                            ConPrintf("\n\rMTU #%d: Coeff[%d] ERROR CRC",i,j);
                            SYS::sleep(200);
                            continue;
                            //ok = FALSE; //break;
                        }
                        U32 coeff = swap4bytes(*(U32*)&(Rsp_MTU[3]));
                        mtu->coeffs.setCoeff(j,coeff);
                        j++;
                    }
                    S(0x0D);
                }
                S(0x0E);
                //if(ok)
                {
                    ConPrintf("\n\rMTU #%d: coeffs OK", i);
                    noCoeffMTU &= ~(1<<i);
                    coeffsFileName[4]=hex_to_ascii[i];
                    SYS::FileWrite(coeffsFileName,&mtu->coeffs,sizeof(MTU_Coeffs));
                }
            }
            S(0x0F);
        }
        S(0x10);
        // disable modbus mode
        if(!Terminated)
            SYS::sleep(3000);
    }
    RS485.clearRxBuf();
    S(0x11);
    // polling
#ifndef __MTU_Old
    int iMTU=0;
    Realtime=TRUE;
    Rsp_MTU[1]=0;
    while(!Terminated)
    {
        S(0x12);
        SYS::sleep(toTypeNext | ctx_MTU.Period);
        // start new measurement
        RS485.writeChar(0xF0);
        TIME netTime;// = SYS::SystemTime;
        SYS::getNetTime(netTime);
        S(0x13);
        //Realtime=FALSE;
        SYS::sleep(1);
        //netTime += SYS::NetTimeOffset;
        //***** MTU receive previous response
        U8 ans = 0xFF;
        U16 cnt = RS485.read(&ans,1,1);
        BOOL ansOk =  cnt==1 && ans<=10;
        Rsp_MTU[0]=0;
        S(0x14);
        if(ansOk)
        {
            int size = ans*4+1;
            cnt = RS485.read(Rsp_MTU+1,size,1);
            S(0x15);
            if(cnt == size)
            {
                MTU_StatAdd(0,ans);
                Rsp_MTU[0]=ans;
            }
        }
        //***** next MTU query
        S(0x16);
        RS485.clearRxBuf();
        int iM=iMTU-1;
        if(iM<0) iM=count-1;
        // send MTU data request
        {
            PU_ADC_MTU *mtu = MTUs[iM];

            int num = mtu->BusNum;
            RS485.writeChar(0xE0 | num);
            MTU_StatAdd(1,0);
        }
        // process previous MTU response
        S(0x17);
        {
            PU_ADC_MTU *pm = MTUs[iMTU];
            pm->response(Rsp_MTU);
            S(0x18);
            pm->doSample(netTime-ctx_MTU.Period);
        }
        iMTU = iM;
    }
#else
    // old polling of MTUs with old firmware
    int i=count-1;
    Realtime=TRUE;
    U16 Period = ctx_MTU.Period;
    while(!Terminated)
    {
        SYS::sleep(toTypeNext | Period);
        // start new measurement
        RS485.writeChar(0xF0);
        TIME netTime;
        SYS::getNetTime(netTime);
        // wait while measure in process
        SYS::sleep(19);
        MTU_StatAdd(1,0);
        RS485.clearRxBuf();
        for(int i=0; i<count; i++)
        {
            // archiving previous
            PU_ADC_MTU *mtu = MTUs[i];
            int num = mtu->BusNum;
            // read buffer with values
            RS485.writeChar(0xE0 | num);
            mtu->doSample(netTime - Period);
            U8 ans = 0xFF;
            U16 cnt = RS485.read(&ans,1,3);
            if(cnt!=1 || ans>10)
            {
                dbg3("\n\rMTU #%X : Buffer reading FAILED (nRes=%X)",num,ans);
                continue;
            }
            U16 size = ans*4+1;
            cnt = RS485.read(Rsp_MTU+1,size,size);
            if(cnt != size)
            {
                ConPrintf("\n\rMTU #%X : Buffer reading FAILED (wrong answer size, %d!=%d)",num,cnt,size);
                continue;
            }
            MTU_StatAdd(0,ans);
            Rsp_MTU[0]=ans;
            mtu->response(Rsp_MTU);
        }
    }
#endif
    dbg("\n\rSTOP MTU_Poll");
#endif // __COM_STATUS_BUF #else
    S(0x00);
}

