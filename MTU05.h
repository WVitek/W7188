int __CntMTU_Query=0, __CntMTU_Answer=0;
inline void MTU_StatAdd(int Qry, int Ans) {
  SYS::cli();
  __CntMTU_Query +=Qry;
  __CntMTU_Answer+=Ans;
  SYS::sti();
}
inline void MTU_StatRead(int &Qry, int &Ans) {
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

    U8 Qry_MTU[16];
    U8 Rsp_MTU[64];
    dbg3("\n\rSTART MTU_Poll @ COM%d:%ld", pollPort, baudRate);
#define use_mtu_crc TRUE
    //***** Scan RS-485 bus
    U16 detectedMTU = 0;
    dbg("\n\rSearch MTUs...");
    while(!Terminated)
    {
        // scan all possible MTUs twice
        U16 detected[2];
        RS485.clearRxBuf();
        U16 bits = 0;
        for(int i=0; i<=15; i++)
        {
            // Check presence and set modbus mode
            RS485.writeChar(0xC0 | i);
            U8 ans;
            U16 cnt = RS485.read(&ans,1,10);
            if(cnt==1 && ans==0xAA)
            {
                U16 bit = 1<<i;
                bits |= bit;
                if((detectedMTU & bit) == 0)
                    dbg2("\n\rMTU-05: #%d detected", i);
                SYS::sleep(20);
            }
        }
        if(bits==0)
        {
            SYS::sleep(3000);
            continue;
        }
        if(detectedMTU==bits)
            break;
        detectedMTU = bits;
    }
    if(Terminated || detectedMTU==0)
        return;
    if(count==0)
        for(int i=0; i<=15; i++)
            if((detectedMTU & (1<<i))!=0)
                addrs[count++] = i;

    // create MTU objects
    PU_ADC_MTU *MTUs[16];
    U16 noCoeffMTU = 0;
    for(int i=0; i<count; i++)
    {
        noCoeffMTU |= 1<<addrs[i];
        MTUs[i] = new PU_ADC_MTU(addrs[i]);
    }
    ConPrintf("\n\rnMTU = %d",ctx_MTU.ADCsList->Count());
    noCoeffMTU &= detectedMTU;
    {
        Qry_MTU[1] = 0x04;
        Qry_MTU[2] = 0x00;
        Qry_MTU[4] = 0x00;
        Qry_MTU[5] = 0x02;
        while(noCoeffMTU!=0 && !Terminated)
        {
            // set modbus mode for present MTUs
            for(int i=0; i<=15; i++)
            {
                if((detectedMTU & (1<<i))==0)
                    continue;
                RS485.writeChar(0x40 | i);
                RS485.read(&Rsp_MTU,1,10);
            }
            // Query coeffs for transformation raw (p,t) values to physical P value
            for(int k = 0; k<count; k++)
            {
                PU_ADC_MTU *mtu = MTUs[k];
                int i = mtu->BusNum;
                if((noCoeffMTU & (1<<i))==0)
                    continue;
                SYS::sleep(200);
                // Get pressure calculation coeffs
                //BOOL ok = TRUE;
                Qry_MTU[0] = i;
                for(int j=0; j<9;)
                {
                    if(mtu->coeffs.C[j]!=0)
                    { j++; continue; }
                    if(Terminated)
                        return;
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
                }
                //if(ok)
                {
                    ConPrintf("\n\rMTU #%d: coeffs OK", i);
                    noCoeffMTU &= ~(1<<i);
                }
            }
        }
    }
    // disable modbus mode
    SYS::sleep(3000);
    // polling
    int iMTU=0;
    Realtime=TRUE;
    while(!Terminated)
    {
        SYS::sleep(toTypeNext | MTU_Period);
        // start new measurement
        RS485.writeChar(0xF0);
        TIME netTime;// = SYS::SystemTime;
        SYS::getNetTime(netTime);
        //Realtime=FALSE;
        SYS::sleep(1);
        //netTime += SYS::NetTimeOffset;
        //***** MTU receive previous response
        U8 ans = 0xFF;
        U16 cnt = RS485.read(&ans,1,1);
        BOOL ansOk =  cnt==1 && ans<=10;
        Rsp_MTU[0]=0;
        if(ansOk)
        {
            int size = ans*4+1;
            cnt = RS485.read(Rsp_MTU+1,size,1);
            if(cnt == size)
            {
                MTU_StatAdd(0,ans);
                Rsp_MTU[0]=ans;
            }
        }
        //***** next MTU query
        RS485.clearRxBuf();
        int iM=iMTU-1;
        if(iM<0) iM=count-1;
        {
            PU_ADC_MTU *mtu = MTUs[iM];
            // send MTU data request
            int num = mtu->BusNum;
            RS485.writeChar(0xE0 | num);
            MTU_StatAdd(1,0);
        }
        // process previous MTU response
        {
            PU_ADC_MTU *pm = MTUs[iMTU];
            pm->response(Rsp_MTU);
            pm->doSample(netTime-MTU_Period);
        }
        iMTU = iM;
    }
    dbg("\n\rSTOP MTU_Poll");
}

