#pragma once

#include <stdlib.h>
#include "Service.h"
#include "wkern.hpp"

//***************************

#define TimeServerAddr 1
#define MaxDataCnt 1

#ifdef __GPS_TIME
    #undef __UseAvgTOfs
#endif

#ifdef __WIRE
    #undef __UseAvgTOfs
    #define TOUT_TimeRequest (toTypeSec | 5)

    #ifdef __MTU
        #define TOUT_ResyncInterval (toTypeSec | 10)
    #else
        #define TOUT_ResyncInterval (toTypeSec | 30)
    #endif
#else
    #define TOUT_TimeRequest (toTypeSec | 30)
    #define TOUT_UrgentRequest (toTypeSec | 1)
    #define TOUT_ResyncInterval (toTypeSec | 73)
#endif

#ifdef __UseAvgTOfs
    #define __MinTimeDelta 1200000
    #define TOUT_EmergencyResyncInterval (toTypeSec | 240)
    #define __MaxAvgDeltaX10 25

    #if defined(__UTNP_VPO)
        #define __MaxTimeError 325
    #elif defined (__UTNP_UPO)
        #define __MaxTimeError 250
    #elif defined (__GPRS_GR47)
        #define __MaxTimeError 250
    #else
        #define __MaxTimeError 125
    #endif
#else
    #if defined(__UTNP_VPO)
        #define __MaxTimeError 400
    #elif defined (__UTNP_UPO)
        #define __MaxTimeError 300
    #elif defined (__GPRS_GR47)
        #define __MaxTimeError 250
    #else
        #define __MaxTimeError 125
    #endif
#endif


class TIMECLIENT_SVC : public SERVICE {

  struct _RESYNCPACKET{
    TIME TQTx,TQRx,TATx;
  };

  typedef TIME _RESYNCDATA[4];

  BOOL FTimeOk;
  TIME FLastReqNetTime;
  _RESYNCDATA* SD;
  int RDCnt;
  int IterCnt;
  int TryCnt;

#ifdef __UseAvgTOfs
  // Для сдвига времени при неудачной синхронизации.
  TIME AvgTOfsCur;
  TIME CumulTOfsCur;
  TIME AvgTOfs1;
  TIME AvgTOfs2;
  TIME LastGoodResyncTimeAfterUpdate1;
  TIME LastGoodResyncTimeBeforeUpdate2;
  TIME LastGoodResyncTimeAfterUpdate2;
  TIMEOUTOBJ toutEmergencyResync;
  TIME LastBadATO;
#endif

  TIMEOUTOBJ toutResync;
  void processTimeData();
#ifdef __UseAvgTOfs
  void correctTimeWithAvgTOfs(TIME CurrentTime, U8 DopCode);
#endif
public:
  TIMECLIENT_SVC();
  ~TIMECLIENT_SVC(){SYS::free(SD);}
  BOOL HaveDataToTransmit(U8 To);
#ifdef __UseAvgTOfs
  BOOL chechForEmergencyResync();
#endif
  int getDataToTransmit(U8 To,void* Data,int MaxSize);
  void receiveData(U8 From,const void* Data,int Size);
  inline BOOL TimeOk(){return FTimeOk;}
};

TIMECLIENT_SVC::TIMECLIENT_SVC():SERVICE(1){

#ifdef __UseAvgTOfs
    // Для сдвига времени при неудачной синхронизации.
    toutEmergencyResync.stop();
    AvgTOfsCur = 0;
    CumulTOfsCur = 0;
    AvgTOfs1 = 0;
    AvgTOfs2 = 0;
    LastGoodResyncTimeAfterUpdate1 = 0;
    LastGoodResyncTimeBeforeUpdate2 = 0;
    LastGoodResyncTimeAfterUpdate2 = 0;
    LastBadATO = 0;
#endif
    RDCnt=0;
    IterCnt=0;
    TryCnt=0;
    FTimeOk=FALSE;
    SD=(_RESYNCDATA*)SYS::malloc(MaxDataCnt*sizeof(_RESYNCDATA));
#ifndef TOUT_UrgentRequest
    toutResync.setSignaled();
#else
    toutResync.start( TOUT_UrgentRequest );
#endif
}


#ifdef __UseAvgTOfs
void TIMECLIENT_SVC::correctTimeWithAvgTOfs(TIME CurrentTime, U8 DopCode){
    if ( DopCode < 64) {
        // emg  2  Emergency resync
        // nnr  1  No need to resync
        // nvr  0  Ne v etot raz
        CurrentTime = (((CurrentTime - LastGoodResyncTimeAfterUpdate2 - CumulTOfsCur)*1000) / AvgTOfsCur) - CumulTOfsCur;
    }else{
        // nmt  254  Min time not OK
        // nmtn 255  ATO2 Min time not OK (такого уже не должно быть).
        CurrentTime = (((CurrentTime - CumulTOfsCur)*1000) / AvgTOfsCur) - CumulTOfsCur;
    }
    SYS::setNetTimeOffset(SYS::NetTimeOffset+CurrentTime);
    ConPrintf("\n\r_ATO resync (type %d). TOfs=%lld\n\r", DopCode, CurrentTime);
    CumulTOfsCur += CurrentTime;
}

BOOL TIMECLIENT_SVC::chechForEmergencyResync()
{
    if ( toutEmergencyResync.IsSignaled() ) {
        // Аварийная корректировка при долгом отсутствии связи.
        if ( AvgTOfsCur == 0) {
            toutEmergencyResync.stop();
            ConPrint("\n\r---Emergency resync failed\n\r"); // Сюда попадать не должны!
            return FALSE;
        }else{
            TIME CurTime;
            SYS::getSysTime(CurTime);
            correctTimeWithAvgTOfs(CurTime, 2);
            toutEmergencyResync.start(TOUT_EmergencyResyncInterval);
            return TRUE;
        }
    }else{
        return FALSE;
    }
}
#endif
BOOL TIMECLIENT_SVC::HaveDataToTransmit(U8 To)
{
#ifdef __GPS_TIME
  return FALSE;
#else
#ifndef __UseAvgTOfs
    return To==TimeServerAddr && toutResync.IsSignaled();
#else
/*    if ( toutEmergencyResync.IsSignaled() ) {
        // Аварийная корректировка при долгом отсутствии связи.
        if ( AvgTOfsCur == 0) {
            toutEmergencyResync.stop();
            ConPrint("\n\r---Emergency resync failed\n\r"); // Сюда попадать не должны!
        }else{
            TIME CurTime;
            SYS::getSysTime(CurTime);
            correctTimeWithAvgTOfs(CurTime, 2);
            toutEmergencyResync.start(TOUT_EmergencyResyncInterval);
        }*/
    if ( chechForEmergencyResync() ) {
        return To==TimeServerAddr && toutResync.IsSignaled();
    }else{
        if ( toutResync.IsSignaled() ) {
            // Если уже корректируем по среднему, то опрашивать каждый раз не нужно.
            if ( AvgTOfsCur != 0 ) {
                TIME CurTime;
                SYS::getSysTime(CurTime);
                CurTime = CurTime - LastGoodResyncTimeAfterUpdate2;
                if ( TIME(CurTime) < TIME(__MinTimeDelta) ) {
                    // Нет смысла посылать запрос. Сразу корректируем по среднему.
                    correctTimeWithAvgTOfs(CurTime, 254);
                    toutEmergencyResync.start(TOUT_EmergencyResyncInterval);
                    toutResync.start(TOUT_TimeRequest);
                    return FALSE;
                }
            }
            return To==TimeServerAddr;
        }else{
            return FALSE;
        }
    }
#endif // __UseAvgTOfs ... else ...
#endif // __GPS_TIME
}

#pragma argsused
int TIMECLIENT_SVC::getDataToTransmit(U8 To,void* Data,int/* MaxSize*/)
{
    if ( To != TimeServerAddr ) return 0;
    toutResync.start(TOUT_TimeRequest);
    int Size = sizeof(_RESYNCPACKET);
    SYS::getNetTime(FLastReqNetTime);
#ifdef __GPRS_GR47
    FLastReqNetTime = FLastReqNetTime+100; // GR47 GPRS-terminal has a 100ms transmission delay (timeout)
#endif
    ((_RESYNCPACKET*)Data)->TQTx=FLastReqNetTime;
    //scramble data
    for ( int i=0; i<Size; i++ ) ((U8*)Data)[i]^=i;
    return Size;
}

void TIMECLIENT_SVC::receiveData(U8 From,const void* Data,int Size){
    if ( From != TimeServerAddr ) return;
    TIME TARx;
    SYS::getNetTime(TARx);
    //descramble data
    for( int i=0; i<Size; i++ ) ((U8*)Data)[i]^=i;
    _RESYNCPACKET &RP=*((_RESYNCPACKET*)Data);
    if ( FLastReqNetTime != RP.TQTx ) {
        ConPrint("\n\rExtraordinary timepacket received");
        return;
    }
    TIME RQ=RP.TQRx-RP.TQTx;
    SD[IterCnt][RDCnt++]=RQ;
    TIME RS=TARx-RP.TATx;
    SD[IterCnt][RDCnt++]=RS;
    if ( RDCnt == 4 ) {
        RDCnt=0;
        if ( ++IterCnt == MaxDataCnt ) {
            IterCnt=0;
            processTimeData();
        }
    }else{
#ifndef TOUT_UrgentRequest
        toutResync.setSignaled(); // urgent time request
#else
        toutResync.start(TOUT_UrgentRequest);
#endif
    }
}

void TIMECLIENT_SVC::processTimeData()
{
#ifndef __UseAvgTOfs
    static S16 StatOfs=0;
#endif
    TIME D1 = SD[0][0] + SD[0][1]; // (QRx-QTx)+(ARx-ATx) = (ARx-QTx)-(ATx-QRx) = total delay in channel
    TIME D2 = SD[0][2] + SD[0][3];
    TIME TOfs0 = ( SD[0][0] - SD[0][1] ); // ( (QRx-QTx)-(ARx-ATx) ) = 2x time offset
    TIME TOfs1 = ( SD[0][2] - SD[0][3] );
    TIME Err = ( abs64(TOfs0-TOfs1) + abs64(D1-D2) ) >> 1;
    TIME TOfs = ( TOfs0 + TOfs1 ) >> 2;
#ifdef __WATCOMC__ // %Ld
    ConPrintf("\n\rTime sync [D1=%dms D2=%dms Err=%d TOfs=%lld] ",
        (int)D1, (int)D2, (int)Err, TOfs
    );
#else
    ConPrintf("\n\rD1=%dms D2=%dms Err=%d TOfs=%p:%p\n\r",
        (int)D1.Lo(), (int)D2.Lo(), (int)Err.Lo(), TOfs.Hi(), TOfs.Lo()
    );
#endif

#ifdef __GPS_TIME
    RDCnt=0;
    TryCnt=0;
    toutResync.start(TOUT_ResyncInterval);
#else
    TIME CurTime;
    if ( Err < TIME(__MaxTimeError) )
    {


      if ( Err < abs64(TOfs) )
      {

        RDCnt=0;
        TryCnt=0;
    #ifdef __UseAvgTOfs
        if ( LastGoodResyncTimeAfterUpdate1 == 0) { // if ( !FTimeOk ) {
            // Первый проход
            SYS::setNetTimeOffset( SYS::NetTimeOffset + TOfs );
            SYS::getSysTime(LastGoodResyncTimeAfterUpdate1);
            FTimeOk = TRUE;
            ConPrintf("\n\r_ATO l1\n\r+++Time sync OK. Err=%d\n\r", (U16)Err);
        }else{
            SYS::getSysTime(LastGoodResyncTimeBeforeUpdate2);
            CurTime = LastGoodResyncTimeBeforeUpdate2 - LastGoodResyncTimeAfterUpdate1;
            if ( LastGoodResyncTimeAfterUpdate2 == 0 ) {
                // Второй проход
                SYS::setNetTimeOffset( SYS::NetTimeOffset + TOfs );
                if ( TIME(CurTime) < TIME(__MinTimeDelta) ) {
                    CumulTOfsCur += TOfs;
                    ConPrintf("\n\r_ATO1 min time not OK (%lld)", CurTime);
                }else{
                    SYS::getSysTime(LastGoodResyncTimeAfterUpdate2);
                    AvgTOfs1 = ((CurTime - CumulTOfsCur)*1000) / (TOfs + CumulTOfsCur);
                    CumulTOfsCur = 0;
                    LastGoodResyncTimeAfterUpdate1 = LastGoodResyncTimeAfterUpdate2;
                    ConPrintf("\n\r_ATO1=%lld", AvgTOfs1);
                }
                ConPrintf("\n\r+++Time sync OK. TOfs=%lld\n\r", TOfs);
            }else{
                // Третий и следующие проходы
                if ( TIME(CurTime) < TIME(__MinTimeDelta) ) {
                    ConPrintf("\n\r_ATO2 min time not OK (%lld)", CurTime);
                    if ( AvgTOfsCur == 0 ) {
                        SYS::setNetTimeOffset( SYS::NetTimeOffset + TOfs ); // Нет сообщения!
                        CumulTOfsCur += TOfs;
                        ConPrintf("\n\r+++Time sync OK. TOfs=%lld\n\r", TOfs);
                    }else{
                        // Обновлять придется по среднему!!! поскольку иначе следующим проходом все равно вернет!!!
                        // По идее сюда больше попадать не должны.
                        correctTimeWithAvgTOfs(CurTime, 255);
                    }
                }else{
                    SYS::setNetTimeOffset( SYS::NetTimeOffset + TOfs );
                    SYS::getSysTime(LastGoodResyncTimeAfterUpdate2);
                    AvgTOfs2 = ((CurTime - CumulTOfsCur)*1000) / (TOfs + CumulTOfsCur);
                    CumulTOfsCur = 0;
                    LastGoodResyncTimeAfterUpdate1 = LastGoodResyncTimeAfterUpdate2;
                    // Учитываем возможность сдвигов времени на сервере
                    if ( ( ( (AvgTOfs2 < 0) && (AvgTOfs1 > 0) )
                        || ( (AvgTOfs2 > 0) && (AvgTOfs1 < 0) ) )
                        || ( abs64(AvgTOfs2*10) > abs64(AvgTOfs1*__MaxAvgDeltaX10) )
                        || ( abs64(AvgTOfs1) > abs64(AvgTOfs2*__MaxAvgDeltaX10) ) )
                    {
                        // Резкий скачок при использовании постоянного сдвига.
                        if ( LastBadATO == 0 ) {
                            // Если до этого еще не было - предполагаем разовый сдвиг времени сервера.
                            // ATO - НЕ обновляем!!!
                            LastBadATO = AvgTOfs2;
                            //LastBadTOfs = TOfs;
                            ConPrintf("\n\r_ATO2=%lld, time jump suspected (ATO1=%lld)", AvgTOfs2, AvgTOfs1);
                        }else{
                            // Если уже было - сравниваем с LastBadATO.
                            if ( ( ( (AvgTOfs2 < 0) && (LastBadATO > 0) )
                                || ( (AvgTOfs2 > 0) && (LastBadATO < 0) ) )
                                || ( abs64(AvgTOfs2*10) > abs64(LastBadATO*__MaxAvgDeltaX10) )
                                || ( abs64(LastBadATO) > abs64(AvgTOfs2*__MaxAvgDeltaX10) ) )
                            {
                                // И тут все плохо. Предполагаем, что были корявые сдвиги.
                                ConPrintf("\n\r_ATO jump error: ATO1=%lld, ATO2=%lld, LBA=%lld, cleared.", AvgTOfs1, AvgTOfs2, LastBadATO);
                                toutEmergencyResync.stop();
                                // Принимаем полученный ATO2 за ATO1, второй и текущий обнуляем.
                                AvgTOfs1 = AvgTOfs2;
                                AvgTOfs2 = 0;
                                LastBadATO = 0;
                                AvgTOfsCur = 0;
                                //// Либо возвращаемся к шагу 2.
                                //LastGoodResyncTimeAfterUpdate2 = 0;
                                //AvgTOfs1 = 0;
                                //// Либо к шагу 1.
                                //LastGoodResyncTimeAfterUpdate1 = 0;
                            }else{
                                // Предполагаем, что некорректен AТO1, используем вместо него LastBadATO.
                                AvgTOfsCur = (AvgTOfs2 + LastBadATO)>>1;
                                LastBadATO = 0;
                                ConPrintf("\n\r_ATO2=%lld, ATO=%lld, time shift suspected", AvgTOfs2, AvgTOfsCur);
                                AvgTOfs1 = AvgTOfsCur;
                            }
                        }
                    }else{
                        AvgTOfsCur = (AvgTOfs2 + AvgTOfs1)>>1;
                        ConPrintf("\n\r_ATO2=%lld, ATO=%lld", AvgTOfs2, AvgTOfsCur);
                        LastBadATO = 0;
                        AvgTOfs1 = AvgTOfsCur;
                    }
                    ConPrintf("\n\r+++Time sync OK. TOfs=%lld\n\r", TOfs);
                }
            }
        }
    #else
        SYS::setNetTimeOffset(SYS::NetTimeOffset+TOfs);
        if ( abs64(TOfs) < TIME(18000) ) {
          StatOfs=StatOfs+(S16)TOfs;
        }

        if( !FTimeOk ) {
            FTimeOk = TRUE;
            ConPrintf("\n\r+++Time sync OK. Err=%d\n\r",(U16)Err);
        }else{
            SYS::getSysTime(CurTime);
            #ifdef __WATCOMC__
            ConPrintf("\n\r+++Time sync OK. StatOfs=%d/%Ld\n\r", StatOfs,CurTime);
            #else
            ConPrintf("\n\r+++Time sync OK. StatOfs=%d/%p:%p\n\r", StatOfs,CurTime.Hi(),CurTime.Lo());
            #endif
        }
    #endif

      }
      else  // if(Err < abs64(TOfs)) ...
      {

    #ifdef __UseAvgTOfs
          if ( AvgTOfsCur == 0 ) {
              ConPrint("\n\r---No need to resync-\n\r");
          }else{
              // Сдвигаем на AvgTOfsCur, поскольку иначе в следующий неудачный раз сдвинется на удвоенный.
              SYS::getSysTime(CurTime);
              correctTimeWithAvgTOfs(CurTime, 1);
          }
    #else
          ConPrint("\n\r---No need to resync-\n\r");
    #endif

      } // if ( Err < abs64(TOfs) ) ... else ...


    }
    else //if ( Err < TIME(__MaxTimeError) )
    {


        if ( FTimeOk ) {
    #ifdef __UseAvgTOfs
            if ( AvgTOfsCur == 0) {
                ConPrint("\n\r---Ne v etot raz-\n\r");
            }else{
                SYS::getSysTime(CurTime);
                correctTimeWithAvgTOfs(CurTime, 0);
            }
    #else
            ConPrint("\n\r---Ne v etot raz-\n\r");
    #endif
        }else{
            ConPrint("\n\r---Time sync oblom-\n\r");
        }


    } //if( Err < TIME(__MaxTimeError) ) ... else ...

    if( !FTimeOk ) {
#ifndef TOUT_UrgentRequest
        toutResync.setSignaled(); // urgent time request
#else
        toutResync.start(TOUT_UrgentRequest);
#endif
    }else{
        toutResync.start(TOUT_ResyncInterval);
    #ifdef __UseAvgTOfs
        if ( AvgTOfsCur != 0 ) {
            toutEmergencyResync.start(TOUT_EmergencyResyncInterval);
        };
    #endif
    };
#endif // #ifdef __GPS_TIME ... #else ...
}
