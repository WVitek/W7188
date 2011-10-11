#pragma once
#ifndef WFLOAT32_H_INCLUDED
#define WFLOAT32_H_INCLUDED

#include "WClasses.hpp"
//#define DEBUG_F32

typedef unsigned long F32;

inline U32 extractF32Frac( F32 a )
{
    return a & 0x007FFFFF;
}
inline S16 extractF32Exp( F32 a )
{
    return ( a>>23 ) & 0xFF;
}
inline U8 extractF32Sign( F32 a )
{
    return (U8)(a>>31);
}
inline F32 packF32( U8 sign, S16 exp, U32 mant )
{
    return ( ((U32)sign)<<31 ) + ( ((U32)exp)<<23 ) + mant;
}
inline void shift32RightJamming( U32 a, S16 count, U32 *r )
{
    if (count == 0)
        *r = a;
    else if ( count < 32 )
        *r = (a>>count) | ( ( a<<((-count)&31) ) != 0 );
    else *r = ( a != 0 );
}

const F32 F32_default_nan = 0xFFC00000;

#define propagateF32NaN( a, b ) F32_default_nan

static F32 roundAndPackF32( U8 zSign, S16 zExp, U32 zSig )
{
#ifdef DEBUG_F32
    ConPrintf("rap32(%d,%X, %lX) ",zSign,zExp,zSig);
#endif

//  U8 roundingMode = float_rounding_mode;
//  U8 roundNearestEven = roundingMode == float_round_nearest_even;
    U8 roundIncrement = 0x40;
//    if ( ! roundNearestEven ) {
//        if ( roundingMode == float_round_to_zero ) {
//            roundIncrement = 0;
//        }
//        else {
//            roundIncrement = 0x7F;
//            if ( zSign ) {
//                if ( roundingMode == float_round_up ) roundIncrement = 0;
//            }
//            else {
//                if ( roundingMode == float_round_down ) roundIncrement = 0;
//            }
//        }
//    }
    U8 roundBits = (U8)zSig & 0x7F;
    U8 isTiny;
    if ( 0xFD <= (U16) zExp )
    {
        if ( ( 0xFD<zExp ) || ( (zExp==0xFD ) && ( (S32)(zSig+roundIncrement) < 0 ) ) )
//            float_raise( float_flag_overflow | float_flag_inexact );
            return packF32( zSign, 0xFF, 0 ) - ( roundIncrement == 0 );
        if ( zExp < 0 )
        {
            isTiny =
//                ( float_detect_tininess == float_tininess_before_rounding ) ||
                ( zExp < -1 ) ||
                ( zSig + roundIncrement < 0x80000000 );
            shift32RightJamming( zSig, - zExp, &zSig );
            zExp = 0;
            roundBits = zSig & 0x7F;
//            if ( isTiny && roundBits ) float_raise( float_U8_underflow );
        }
    }
//    if ( roundBits ) float_exception_flags |= float_flag_inexact;
    zSig = ( zSig + roundIncrement )>>7;
    zSig &= ~ ( ( roundBits ^ 0x40 ) == 0 );
//    zSig &= ~ ( ( ( roundBits ^ 0x40 ) == 0 ) & roundNearestEven );
    if ( zSig == 0 ) zExp = 0;
    return packF32( zSign, zExp, zSig );
}

static F32 addF32Sigs( F32 a, F32 b, U8 zSign )
{
    U32 aSig = extractF32Frac( a );
    S16 aExp = extractF32Exp( a );
    U32 bSig = extractF32Frac( b );
    S16 bExp = extractF32Exp( b );
    S16 expDiff = aExp - bExp;
    aSig <<= 6;
    bSig <<= 6;

    S16 zExp;
    U32 zSig;

    if ( 0 < expDiff )
    {
        if ( aExp == 0xFF )
        {
            if ( aSig ) return propagateF32NaN( a, b );
            return a;
        }
        if ( bExp == 0 )
            --expDiff;
        else bSig |= 0x20000000;
        shift32RightJamming( bSig, expDiff, &bSig );
        zExp = aExp;
    }
    else if ( expDiff < 0 )
    {
        if ( bExp == 0xFF )
        {
            if ( bSig ) return propagateF32NaN( a, b );
            return packF32( zSign, 0xFF, 0 );
        }
        if ( aExp == 0 )
            ++expDiff;
        else aSig |= 0x20000000;
        shift32RightJamming( aSig, - expDiff, &aSig );
        zExp = bExp;
    }
    else
    {
        if ( aExp == 0xFF )
        {
            if ( aSig | bSig ) return propagateF32NaN( a, b );
            return a;
        }
        if ( aExp == 0 ) return packF32( zSign, 0, ( aSig + bSig )>>6 );
        zSig = 0x40000000 + aSig + bSig;
        zExp = aExp;
        goto roundAndPack;
    }
    aSig |= 0x20000000;
    zSig = ( aSig + bSig )<<1;
    --zExp;
    if ( (S32) zSig < 0 )
    {
        zSig = aSig + bSig;
        ++zExp;
    }
roundAndPack:
    return roundAndPackF32( zSign, zExp, zSig );
}

static U8 countLeadingZeros32( U32 a )
{
    static const U8 countLeadingZerosHigh[] =
    {
        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    U8 shiftCount;

    shiftCount = 0;
    if ( a < 0x10000 )
    {
        shiftCount += 16;
        a <<= 16;
    }
    if ( a < 0x1000000 )
    {
        shiftCount += 8;
        a <<= 8;
    }
    shiftCount += countLeadingZerosHigh[ a>>24 ];
    return shiftCount;
}

static F32 normalizeRoundAndPackF32( U8 zSign, S16 zExp, U32 zSig )
{
    U8 shiftCount = countLeadingZeros32( zSig ) - 1;
#ifdef DEBUG_F32
    ConPrintf("clz(%ld)=%d;%lX ",zSig,shiftCount,zSig<<shiftCount);
#endif
    return roundAndPackF32( zSign, zExp - shiftCount, zSig<<shiftCount );
}

static void normalizeF32Subnormal( U32 aSig, S16 *zExpPtr, U32 *zSigPtr )
{
    U8 shiftCount;
    shiftCount = countLeadingZeros32( aSig ) - 8;
    *zSigPtr = aSig<<shiftCount;
    *zExpPtr = 1 - shiftCount;
}

static F32 subF32Sigs( F32 a, F32 b, U8 zSign )
{
    S16 aExp, bExp, zExp;
    U32 aSig, bSig, zSig;
    S16 expDiff;

    aSig = extractF32Frac( a );
    aExp = extractF32Exp( a );
    bSig = extractF32Frac( b );
    bExp = extractF32Exp( b );
    expDiff = aExp - bExp;
    aSig <<= 7;
    bSig <<= 7;
    if ( 0 < expDiff ) goto aExpBigger;
    if ( expDiff < 0 ) goto bExpBigger;
    if ( aExp == 0xFF )
    {
        if ( aSig | bSig ) return propagateF32NaN( a, b );
        //float_raise( float_U8_invalid );
        return F32_default_nan;
    }
    if ( aExp == 0 )
    {
        aExp = 1;
        bExp = 1;
    }
    if ( bSig < aSig ) goto aBigger;
    if ( aSig < bSig ) goto bBigger;
//    return packF32( float_rounding_mode == float_round_down, 0, 0 );
    return packF32( 0, 0, 0 );
bExpBigger:
    if ( bExp == 0xFF )
    {
        if ( bSig ) return propagateF32NaN( a, b );
        return packF32( zSign ^ 1, 0xFF, 0 );
    }
    if ( aExp == 0 )
        ++expDiff;
    else aSig |= 0x40000000;
    shift32RightJamming( aSig, - expDiff, &aSig );
    bSig |= 0x40000000;
bBigger:
    zSig = bSig - aSig;
    zExp = bExp;
    zSign ^= 1;
    goto normalizeRoundAndPack;
aExpBigger:
    if ( aExp == 0xFF )
    {
        if ( aSig ) return propagateF32NaN( a, b );
        return a;
    }
    if ( bExp == 0 )
        --expDiff;
    else bSig |= 0x40000000;
    shift32RightJamming( bSig, expDiff, &bSig );
    aSig |= 0x40000000;
aBigger:
    zSig = aSig - bSig;
    zExp = aExp;
normalizeRoundAndPack:
    --zExp;
    return normalizeRoundAndPackF32( zSign, zExp, zSig );

}

F32 F32_add( F32 a, F32 b )
{
    U8 aSign, bSign;

    aSign = extractF32Sign( a );
    bSign = extractF32Sign( b );
    if ( aSign == bSign )
    {
        return addF32Sigs( a, b, aSign );
    }
    else
    {
        return subF32Sigs( a, b, aSign );
    }

}

F32 F32_sub( F32 a, F32 b )
{
    U8 aSign, bSign;

    aSign = extractF32Sign( a );
    bSign = extractF32Sign( b );
    if ( aSign == bSign )
    {
        return subF32Sigs( a, b, aSign );
    }
    else
    {
        return addF32Sigs( a, b, aSign );
    }

}

inline void mul32To64( U32 a, U32 b, U32 &z0Ptr, U32 &z1Ptr )
{
    /*
        S16 aHigh, aLow, bHigh, bLow;
        U32 z0, zMiddleA, zMiddleB, z1;

        aLow = a;
        aHigh = a>>16;
        bLow = b;
        bHigh = b>>16;
        z1 = ( (U32) aLow ) * bLow;
        zMiddleA = ( (U32) aLow ) * bHigh;
        zMiddleB = ( (U32) aHigh ) * bLow;
        z0 = ( (U32) aHigh ) * bHigh;
        zMiddleA += zMiddleB;
        z0 += ( ( (U32) ( zMiddleA < zMiddleB ) )<<16 ) + ( zMiddleA>>16 );
        zMiddleA <<= 16;
        z1 += zMiddleA;
        z0 += ( z1 < zMiddleA );
        *z1Ptr = z1;
        *z0Ptr = z0;
    /*/
    S64 tmp = mul32to64(a,b);
    U32 z1 = ((U32*)&tmp)[0];
    U32 z0 = ((U32*)&tmp)[1];
    z0Ptr = z0;
    z1Ptr = z1;
#ifdef DEBUG_F32
    ConPrintf("%lX*%lX=%LX ",a,b,tmp);
#endif
//*/
}

F32 S32_to_F32( S32 a )
{
    if ( a == 0 ) return 0;
    if ( a == (S32) 0x80000000 ) return packF32( 1, 0x9E, 0 );
    U8 zSign = ( a < 0 );
#ifdef DEBUG_F32
    ConPrintf("a=%ld ",a);
#endif
    return normalizeRoundAndPackF32( zSign, 0x9C, zSign ? - a : a );
}

S32 F32_to_S32( F32 a )
{
    U8 aSign;
    S16 aExp, shiftCount;
    U32 aSig, aSigExtra;
    S32 z;
//    U8 roundingMode;

    aSig = extractF32Frac( a );
    aExp = extractF32Exp( a );
    aSign = extractF32Sign( a );
    shiftCount = aExp - 0x96;
    if ( 0 <= shiftCount )
    {
        if ( 0x9E <= aExp )
        {
            if ( a != 0xCF000000 )
            {
                //float_raise( float_U8_invalid );
                if ( ! aSign || ( ( aExp == 0xFF ) && aSig ) )
                {
                    return 0x7FFFFFFF;
                }
            }
            return (S32) 0x80000000;
        }
        z = ( aSig | 0x00800000 )<<shiftCount;
        if ( aSign ) z = - z;
    }
    else
    {
        if ( aExp < 0x7E )
        {
            aSigExtra = aExp | aSig;
            z = 0;
        }
        else
        {
            aSig |= 0x00800000;
            aSigExtra = aSig<<( shiftCount & 31 );
            z = aSig>>( - shiftCount );
        }
//        if ( aSigExtra ) float_exception_U8s |= float_U8_inexact;
//        roundingMode = float_rounding_mode;
//        if ( roundingMode == float_round_nearest_even )
        {
            if ( (S32) aSigExtra < 0 )
            {
                ++z;
                if ( (S32) ( aSigExtra<<1 ) == 0 ) z &= ~1;
            }
            if ( aSign ) z = - z;
        }
//        else {
//            aSigExtra = ( aSigExtra != 0 );
//            if ( aSign ) {
//                z += ( roundingMode == float_round_down ) & aSigExtra;
//                z = - z;
//            }
//            else {
//                z += ( roundingMode == float_round_up ) & aSigExtra;
//            }
//        }
    }
    return z;
}

F32 F32_mul( F32 a, F32 b )
{
    U8 aSign, bSign, zSign;
    S16 aExp, bExp, zExp;
    U32 aSig, bSig, zSig0, zSig1;

    aSig = extractF32Frac( a );
    aExp = extractF32Exp( a );
    aSign = extractF32Sign( a );
    bSig = extractF32Frac( b );
    bExp = extractF32Exp( b );
    bSign = extractF32Sign( b );
    zSign = aSign ^ bSign;
    if ( aExp == 0xFF )
    {
        if ( aSig || ( ( bExp == 0xFF ) && bSig ) )
        {
            return propagateF32NaN( a, b );
        }
        if ( ( bExp | bSig ) == 0 )
        {
            //float_raise( float_U8_invalid );
            return F32_default_nan;
        }
        return packF32( zSign, 0xFF, 0 );
    }
    if ( bExp == 0xFF )
    {
        if ( bSig ) return propagateF32NaN( a, b );
        if ( ( aExp | aSig ) == 0 )
        {
            //float_raise( float_U8_invalid );
            return F32_default_nan;
        }
        return packF32( zSign, 0xFF, 0 );
    }
    if ( aExp == 0 )
    {
        if ( aSig == 0 ) return packF32( zSign, 0, 0 );
        normalizeF32Subnormal( aSig, &aExp, &aSig );
    }
    if ( bExp == 0 )
    {
        if ( bSig == 0 ) return packF32( zSign, 0, 0 );
        normalizeF32Subnormal( bSig, &bExp, &bSig );
    }
    zExp = aExp + bExp - 0x7F;
    aSig = ( aSig | 0x00800000 )<<7;
    bSig = ( bSig | 0x00800000 )<<8;
    mul32To64( aSig, bSig, zSig0, zSig1 );
    zSig0 |= ( zSig1 != 0 );
    if ( 0 <= (S32) ( zSig0<<1 ) )
    {
        zSig0 <<= 1;
        --zExp;
    }
    F32 res = roundAndPackF32( zSign, zExp, zSig0 );
#ifdef DEBUG_F32
    ConPrintf("a=%lX, b=%lX, a*b=%lX (%ld, %ld, %ld)\n\r",a,b,res,F32_to_S32(a),F32_to_S32(b),F32_to_S32(res));
#endif
    return res;
}

inline F32 fmul(F32 a, F32 b)
{
    return F32_mul(a,b);
}
inline F32 fadd(F32 a, F32 b)
{
    return F32_add(a,b);
}

const static F32 f1000 = 0x447A0000; // 1000

S32 F32_to_1000scaled(F32 x)
{
    return F32_to_S32(fmul(x,f1000));
}

inline BOOL f_gt(F32 a, F32 b) { return (S32)a > (S32)b; }
inline BOOL f_lt(F32 a, F32 b) { return (S32)a < (S32)b; }

#endif // WFLOAT32_H_INCLUDED
