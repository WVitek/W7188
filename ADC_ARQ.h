#ifndef __SERVICE_H
#include "SERVICE.H"
#endif

#include <i86.h>

class ADC_SVC : public SERVICE
{
    TIME* lastSendTime;
    ADC_LIST *plADC;
public:
    ADC_SVC(ADC_LIST *ADCs, U8 ID=2):SERVICE(ID)
    {
        plADC = ADCs;
        int n = plADC->Count();
        lastSendTime = (TIME*)SYS::malloc(sizeof(TIME)*n);
        for(int i=n-1; i>=0; i--)
            lastSendTime[i]=0;
    }

    BOOL ADC_SVC::HaveDataToTransmit(U8)
    {
      BOOL Res=FALSE;
      if(SYS::TimeOk)
      {
        beginRead();
        for(int i=plADC->Count()-1; i>=0 && !Res; i--)
          Res=Res || ((*plADC)[i]->GetLastTime() > lastSendTime[i]);
        endRead();
      }
      return Res;
    }

    int ADC_SVC::getDataToTransmit(U8,void* Data,int MaxSize)
    {
        int n=plADC->Count();
        if(n==0) return 0;
        int iP = 0;
        TIME minTime=lastSendTime[0];
        for(int i=1; i<n; i++)
        {
            if(lastSendTime[i] >= minTime)
                continue;
            iP = i;
            minTime = lastSendTime[i];
        }
        int Cnt=(MaxSize-1-sizeof(TIME))/ASItemSize;
        PU_ADC* P = (*plADC)[iP];
        P->cs.enter();
        int size = P->readArchive(minTime,(U8*)Data+1+sizeof(TIME),Cnt);
        P->cs.leave();
        U8* Buf = (U8*)Data;
        Buf[0] = (U8)iP;
        *(TIME*)&(Buf[1]) = minTime;
        Cnt = size / ASItemSize;
        if(Cnt==0)
            return 0;
        lastSendTime[iP] = minTime + smul(Cnt,P->ADC_Period);
        return 1+sizeof(TIME)+Cnt*ASItemSize;
    }

    void receiveData(U8,const void* /*Data*/,int /*Size*/){}
};



