#pragma once
#ifndef __WCLASSES_HPP
#define __WCLASSES_HPP

//***** Calling conventions
#define _fast _fastcall

typedef unsigned char  BOOL;
typedef unsigned char  U8;
typedef signed   char  S8;
typedef unsigned /*short*/ U16;
typedef signed   /*short*/ S16;
typedef unsigned long  U32;
typedef signed   long  S32;

struct CMD_LINE
{
  U8 Length;
  U8 Data[127];
};

#ifdef __WATCOMC__

typedef __int64 S64;
extern S64 abs64(S64 a);
#pragma aux abs64 = \
  "or   ax,ax" \
  "jns  Finish" \
  "not  dx" \
  "not  cx" \
  "not  bx" \
  "not  ax" \
  "add  dx,1" \
  "adc  cx,0" \
  "adc  bx,0" \
  "adc  ax,0" \
  "Finish:" \
  parm [ax bx cx dx] \
  value [ax bx cx dx];

#else

class S64 {
public:
  inline S64(S32 hi,U32 lo){ this->hi=hi; this->lo=lo; }
  S64(S32);
  S64(){}
  inline void operator+= (int);
  void operator+= (S32);
  friend S64 operator+ (S64,S64);
  friend S64 operator+ (S64,S32);
  friend S64 operator+ (S32,S64);
  friend S64 operator- (S64,S64);
  friend S64 operator- (S64,S32);
  friend S64 operator- (S32,S64);
  friend S64 operator<<(S64,unsigned char);
  friend S64 operator>>(S64,unsigned char);
  friend int operator% (S64,int);
  friend S64 abs64(S64 a);
  friend int operator==(S64,S64);
  friend int operator<(S64,S64);
  friend int operator>(S64,S64);
  friend int operator<=(S64,S64);
  friend int operator>=(S64,S64);
  inline U32 Lo() const { return lo;}
  inline S32 Hi() const { return hi;}
  inline void *LP() const { return (void*)lo;}
  inline void *HP() const { return (void*)hi;}
  inline operator S32() const {return lo;}
private:
  U32 lo;
  S32 hi;
};

inline S64 operator+(S32 a,S64 b){ return b+a; }
inline int operator==(S64 a,S64 b){ return a.lo==b.lo && a.hi==b.hi; }
inline int operator<=(S64 a,S64 b){ return !(a>b); }
inline int operator>=(S64 a,S64 b){ return !(a<b); }

#endif

typedef S64 TIME;

typedef U16 TIMEOUT;
#define toTypeMask   0xC000
//
#define toTypeNext   0xC000
#define toNext10ms   0xC00A
#define toNext100ms  0xC064
#define toNextSecond 0xC3E8
//
#define toTypeMs     0x0000
#define toTypeSec    0x4000

class OBJECT {
public:
  virtual ~OBJECT();
};

class STREAM : public OBJECT {
public:
  U16 virtual read(void* Buf,U16 Cnt)=0;
  U16 virtual write(const void* Buf,U16 Cnt)=0;
};

class STREAMABLE {
public:
  void virtual loadFrom(STREAM &IS)=0;
  void virtual storeIn(STREAM &OS)const=0;
};

typedef OBJECT *FUNC_Load(STREAM &IS);

#endif //__CLASSES_HPP
