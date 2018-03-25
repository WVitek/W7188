#pragma once
#ifndef __WHRDWARE_HPP
#define __WHRDWARE_HPP

#include <conio.h>

#include "WAm186ES.h"

#if defined(__7188)
  //*** I7188 hardware defines
  #include "7188.h"
  #if defined(__SMALL__) || defined(__COMPACT__)
    #pragma library("lib\7188s.lib");
  #else
    #pragma library("lib\7188l.lib");
  #endif
  #define __SoftAutoDir
  #define ComDirPort   PIO_DATA_0
  #define Com1DirBit   0x20
  #define Com2DirBit   0x40
  #define Com1Base     0x200
  #define Com1Int      INT_I0INT
  #define Com1Msk      INT_MSKI0
  #define Com2Base     0x100
  #define Com2Int      INT_I1INT
  #define Com2Msk      INT_MSKI1
  /*
  #define ClkPort PIO_DATA_1
  #define ClkBit  0x0400

  #define LedPort PIO_DATA_1
  #define LedBit  0x0008

  #define Led5DiPort   PIO_DATA_1
  #define Led5LoadPort PIO_DATA_1
  #define Led5DiBit    0x0010
  #define Led5LoadBit  0x0020

  #define EepCsPort   PIO_DATA_0
  #define EepCsBit    0x0004
  #define EepDoPort   PIO_DATA_0
  #define EepDoBit    0x0008

  #define Com3DirPort PIO_DATA_0
  #define Com3DirBit  0x20

  #define Com4DirPort PIO_DATA_0
  #define Com4DirBit  0x40
  */
#elif defined(__I7188)
  //*** I7188 (muOS7) hardware defines
  #include "I7188.h"
  #if defined(__SMALL__) || defined(__COMPACT__)
    #pragma library("lib\I7188s.lib");
  #else
    #pragma library("lib\I7188l.lib");
  #endif
  #define __SoftAutoDir
  #define ComDirPort   PIO_DATA_0
  #define Com1DirBit   0x20
  #define Com2DirBit   0x40
  #define Com1Base     0x200
  #define Com1Int      INT_I0INT
  #define Com1Msk      INT_MSKI0
  #define Com2Base     0x100
  #define Com2Int      INT_I1INT
  #define Com2Msk      INT_MSKI1

#elif defined(__7188XA)
  //*** 7188XA hardware defines
  #define __7188X
  //#define __mOS7
  #include "7188xa.h"
  #if defined(__SMALL__) || defined(__COMPACT__)
    #pragma library ("lib\w7188xas.lib");
  #else
    #pragma library ("lib\w7188xal.lib");
  #endif
  #define Com1Base     0x100
  #define Com1Int      INT_I2INT
  #define Com1Msk      INT_MSKI2
  #define Com2Base     0x108
  #define Com2Int      INT_I3INT
  #define Com2Msk      INT_MSKI3

#elif defined(__7188XB)
  //*** 7188XB hardware defines
  #define __7188X
  #define __mOS7
  #include "7188xa.h"
  #if defined(__SMALL__) || defined(__COMPACT__)
    #pragma library ("lib\w7188xas.lib");
  #else
    #pragma library ("lib\w7188xal.lib");
  #endif

#elif defined(__IP8000)
  //*** 7188XA hardware defines
  //#definC:\I8000\W7188\Hardware.hppe __mOS7
  #include "8000a.h"
  #if defined(__SMALL__) || defined(__COMPACT__)
    #error Only LARGE or HUGE memory model supported for __IP8000;
  #else
    #pragma library ("lib\8000a.lib");
  #endif

  #define __UseVendorComms

#else
  #error Declare target controller (__7188, __7188XA, __7188XB or __IP8000) in compiler options
#endif

#if defined(__mOS7) || defined(__IP8000)
  #define ConsoleBaudRate 115200
#else
  #define ConsoleBaudRate 57600
#endif

  class DIO
  {
    inline static void SetDO(int port,int bits,bool high)
    {
        if(high)
          outpw(port, inpw(port) | bits);
        else
          outpw(port, inpw(port) & ~bits);
    }
    inline static bool GetD(int port,int bit) { return (inpw(port) & bit) != 0; }
  public:
#if defined(__7188XA)
    inline static void SetDO1(bool high) { SetDO(PIO_DATA_1,0x0010,high); }
    inline static void SetDO2(bool high) { SetDO(PIO_DATA_1,0x0020,high); }
    inline static bool GetDO1() { return GetD(PIO_DATA_1,0x0010); }
    inline static bool GetDO2() { return GetD(PIO_DATA_1,0x0020); }
    inline static bool GetDI1() { return GetD(PIO_DATA_1,0x0100); }
    inline static bool GetDI2() { return GetD(PIO_DATA_0,0x0001); }
#elif defined(__7188XB)
    inline static void SetDO1(bool high) { SetDO(PIO_DATA_1,0x0002,high); }
    inline static bool GetDO1() { return GetD(PIO_DATA_1,0x0002); }
    inline static bool GetDI1() { return GetD(PIO_DATA_1,0x8000); }
#else
    inline static void SetDO1(bool) { }
    inline static bool GetDO1() { return false; }
    inline static bool GetDI1() { return false; }
#endif
  };

#if defined(__X600)
  //*** X600 hardware defines
  #include "x600.h"
  #if defined(__SMALL__) || defined(__COMPACT__)
    #pragma library ("lib\x600s.lib");
  #else
    #pragma library ("lib\x600l.lib");
  #endif
#endif

#ifdef __BORLANDC__
#define ASMLABEL(L) } L: _asm{
#else
#define ASMLABEL(L) L:
#endif

#endif // __HARDWARE_HPP
