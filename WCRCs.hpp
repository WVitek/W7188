#ifndef __WCRCS_HPP
#define __WCRCS_HPP
#pragma once

#ifndef __WKERN_HPP
#include "WKern.hpp"
#endif

typedef void FUpdateCRC16(const void* Data, U16 DataLen, U16 &CurCRC);

//***** CRC: Point-to-Point Protocol FCS16 (RFC1662)
void CRC16_PPP   (const void *Data, U16 DataLen, U16 &CurCRC);
//***** CRC: ModBus protocol CRC16
void CRC16_ModBus(const void *Data, U16 DataLen, U16 &CurCRC);
//***** CRC: Grant MTU-05 CRC16
void CRC16_MTU05 (const void *Data, U16 DataLen, U16 &CurCRC);
//***** CRC: ICP-DAS MiniOS 7 filesystem CRC16
void CRC16_OS7FS (const void *Data, U16 DataLen, U16 &CurCRC);

//***** CRC16_get
// calculate CRC16 with use of the specified function
U16 CRC16_get(const void *Data, U32 DataLen, FUpdateCRC16 fucrc);
inline U16 CRC16(const void *Data, U16 DataLen, FUpdateCRC16 fucrc)
{
    U16 CRC;
    fucrc(NULL,0,CRC);
    fucrc(Data,DataLen,CRC);
    return CRC;
}

//***** CRC16_is_OK
// make validation of data with assumption that presaved CRC16 immediately follow data block
// return TRUE if calculated CRC16 == presaved CRC16
BOOL CRC16_is_OK(const void *Data, U32 DataLen, FUpdateCRC16 fucrc);

//***** CRC16_write
// calculate & write CRC16 immediately after block of data
void CRC16_write(void *Data, U32 DataLen, FUpdateCRC16 fucrc);

//***** CCITT-8 CRC
U8 CRC8_CCITT_get(const void *Data, U16 DataLen);

#endif
