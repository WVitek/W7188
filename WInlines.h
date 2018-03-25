//file WInlines.h
#pragma once
#ifndef WINLINES_H_INCLUDED
#define WINLINES_H_INCLUDED

#include "WClasses.HPP"

inline U16 swapBytes(U16 x){return (x<<8) | (x>>8);}
inline U16 hi(U32 v){return FP_SEG((void* far)v);}
inline U16 lo(U32 v){return FP_OFF((void* far)v);}
inline U8 HiByte(U16 v){return ((U8*)&v)[1];}
inline U8 LoByte(U16 v){return ((U8*)&v)[0];}
inline void* U32toFP(U32 x) { return MK_FP( x>>4, x & 0xF ); }
inline U32 FPtoU32(const void *x) { return ((U32)FP_SEG(x)<<4) + FP_OFF(x); }

#if defined(__WATCOMC__)

extern CMD_LINE const * GetCmdLine();
#pragma aux GetCmdLine = \
  "mov ah,0x62" \
  "int 0x21" \
  "sub bx,8" \
  "xor ax,ax" \
  modify [ax bx] \
  value [ax bx];

extern inline U16 swap2bytes(U16 x);
#pragma aux swap2bytes = \
  "mov   ch, al" \
  "mov   cl, ah" \
  parm   [ax] \
  modify nomemory exact [cx] \
  value  [cx];

extern inline U32 swap4bytes(U32 x);
// dh dl ah al
// al ah bl bh
#pragma aux swap4bytes = \
  "mov   bh,al" \
  "mov   al,dh" \
  "mov   bl,ah" \
  "mov   ah,dl" \
  parm   [dx ax]\
  modify nomemory exact [ax bx] \
  value  [bx ax];

extern inline U32 umul(U16 x,U16 y); // x*y
#pragma aux umul = \
  "mul   dx" \
  parm   [ax] [dx] \
  value  [dx ax];

extern inline U16 udiv(U32 n,U16 d); // n/d
#pragma aux udiv = \
  "div   bx" \
  parm   [dx ax] [bx] \
  modify [dx] \
  value  [ax];

extern inline U16 udivmod(U32 n,U16 d,U16& rem);
#pragma aux udivmod = \
  "div   bx" \
  "mov   es:[di], dx" \
  parm   [dx ax] [bx] [es di] \
  modify [dx] \
  value  [ax];

extern inline S16 sdivmod(S32 n,S16 d,S16& rem);
#pragma aux sdivmod = \
  "idiv  bx" \
  "mov   es:[di], dx" \
  parm   [dx ax] [bx] [es di] \
  modify [dx] \
  value  [ax];

extern inline S32 smul(S16 x,S16 y); // x*y
#pragma aux smul = \
  "imul  dx" \
  parm   [ax] [dx] \
  value  [dx ax];

extern inline S16 sdiv(S32 n,S16 d); // n/d
#pragma aux sdiv = \
  "idiv  bx" \
  parm   [dx ax] [bx] \
  modify [dx] \
  value  [ax];

extern inline U16 umuldiv(U16 x,U16 y,U16 z); // x*y/z
#pragma aux umuldiv = \
  "mul  dx" \
  "div  bx" \
  parm   [dx] [ax] [bx] \
  modify [dx] \
  value  [ax];

extern inline U8 inpb(U16 port);
#pragma aux inpb = \
  "in   al,dx" \
  parm   [dx] \
  value  [al];

extern inline S64 _mult_helper(U32 hi,U32 mid, U32 lo);
#pragma aux _mult_helper = \
  "add   cx, si" \
  "adc   ax, di" \
  "adc   bx, 0" \
  "xchg  ax, bx" \
  parm   [bx ax] [di si] [cx dx] \
  modify nomemory exact [cx ax bx] \
  value  [ax bx cx dx];

inline S64 smul(S32 sAB, S32 sXY)
{
    BOOL sign = FALSE;
    U32 ab, xy;
    if(sAB<0)
    { ab = -sAB; sign = !sign; }
    else ab = sAB;
    if(sXY<0)
    { xy = -sXY; sign = !sign; }
    else xy = sXY;
    U16 a = hi(ab);
    U16 b = lo(ab);
    U16 x = hi(xy);
    U16 y = lo(xy);
    S64 res = _mult_helper(umul(a,x),umul(a,y)+umul(b,x), umul(b,y));
    if(sign)
        return -res;
    else return res;
}

inline S64 mul32to64(U32 ab, U32 xy)
{
    U16 a = hi(ab), b = lo(ab), x = hi(xy), y = lo(xy);
    S64 res = _mult_helper(umul(a,x),umul(a,y)+umul(b,x), umul(b,y));
    return res;
}

#elif defined(__BORLANDC__)

inline U32 _fastcall umul(U16 x,U16 y){ // x*y
  _AX=x; _DX=y;
  __emit__(0xF7,0xE2); // mul dx
  return U32(MK_FP(_DX,_AX));
}

inline U16 _fastcall udiv(U32 n,U16 d){ // n/d
  _AX=n; _DX=hi(n); _BX=d;
  __emit__(0xF7,0xF3); // div bx
  return _AX;
}

inline S32 _fastcall smul(S16 x,S16 y){ // x*y
  _AX=x; _DX=y;
  __emit__(0xF7,0xEA); // imul dx
  return S32(MK_FP(_DX,_AX));
}

inline S16 _fastcall sdiv(S32 n,S16 d){ // n/d
  _AX=n; _DX=hi(n); _BX=d;
  __emit__(0xF7,0xFB); // idiv bx
  return _AX;
}

inline U16 _fastcall umuldiv(U16 x,U16 y,U16 z){ // x*y/z
  _AX=x; _DX=y; _BX=z;
  __emit__(0xF7,0xE2,0xF7,0xF3);
  return _AX;
}

inline U8 _fastcall inpb(U16 port){return (U8)inp(port);}

#endif

inline U8& Mem(U16 Seg,U16 Offset){ return *(U8*)MK_FP(Seg,Offset); }
inline U16& MemW(U16 Seg,U16 Offset){ return *(U16*)MK_FP(Seg,Offset); }

//*** WARNING!!! Used undocumented? features of BC++'s far heap.
inline U16 MemBlockSize16(void* x){ return *(U16*)MK_FP(FP_SEG(x),0); }
inline U16 MemBlockSize16(U16 Size){ return (Size+4+15)>>4; }

#endif // ifndef WINLINES_H_INCLUDED
