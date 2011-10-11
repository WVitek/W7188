S64 abs64(S64 a){
  S64 r;
  _asm{
    // load value
    mov  ax,word ptr a+0
    mov  bx,word ptr a+2
    mov  cx,word ptr a+4
    mov  dx,word ptr a+6
    // test sign of value
    or   dx,dx
    jns  Finish
    // negate value
    not  ax
    add  ax,1
    not  bx
    adc  bx,0
    not  cx
    adc  cx,0
    not  dx
    adc  dx,0
  }
Finish:
  _asm{
    // store result
    mov  word ptr r+0,ax
    mov  word ptr r+2,bx
    mov  word ptr r+4,cx
    mov  word ptr r+6,dx
  }
  return r;
}

S64::S64(S32 v){
  _asm{
    mov ax,word ptr v+2
    les di,this
    mov bx,word ptr v
    cwd
    mov es:[di+0],bx
    mov es:[di+2],ax
    mov es:[di+4],dx
    mov es:[di+6],dx
  }
}

S64 operator+(S64 a,S64 b){
  S64 r;
  _asm{
    mov ax,word ptr a+0
    mov bx,word ptr a+2
    mov cx,word ptr a+4
    mov dx,word ptr a+6
    add ax,word ptr b+0
    adc bx,word ptr b+2
    adc cx,word ptr b+4
    adc dx,word ptr b+6
    mov word ptr r+0,ax
    mov word ptr r+2,bx
    mov word ptr r+4,cx
    mov word ptr r+6,dx
  }
  return r;
}

S64 operator+(S64 a,S32 b){
  S64 r;
  _asm{
    mov ax,word ptr b+2
    mov bx,word ptr b+0
    cwd
    mov cx,dx
    add bx,word ptr a+0
    adc ax,word ptr a+2
    adc cx,word ptr a+4
    adc dx,word ptr a+6
    mov word ptr r+0,bx
    mov word ptr r+2,ax
    mov word ptr r+4,cx
    mov word ptr r+6,dx
  }
  return r;
}


void S64::operator+= (int v){
  _asm{
    les di,this
    mov ax,v
    cwd
    add es:[di+0],ax
    adc es:[di+2],dx
    adc es:[di+4],dx
    adc es:[di+6],dx
  }
}

void S64::operator+= (S32 v){
  _asm{
    les di,this
    mov ax,word ptr v+2
    mov bx,word ptr v+0
    cwd
    add es:[di+0],bx
    adc es:[di+2],ax
    adc es:[di+4],dx
    adc es:[di+6],dx
  }
}

S64 operator-(S64 a,S64 b){
  S64 r;
  _asm{
    mov ax,word ptr a+0
    mov bx,word ptr a+2
    mov cx,word ptr a+4
    mov dx,word ptr a+6
    sub ax,word ptr b+0
    sbb bx,word ptr b+2
    sbb cx,word ptr b+4
    sbb dx,word ptr b+6
    mov word ptr r+0,ax
    mov word ptr r+2,bx
    mov word ptr r+4,cx
    mov word ptr r+6,dx
  }
  return r;
}

S64 operator-(S64 a,S32 b){
  S64 r;
  _asm{
    mov ax,word ptr b+2
    mov bx,word ptr b+0
    cwd
    mov cx,word ptr a+0
    mov di,word ptr a+2
    sub cx,bx
    sbb di,ax
    mov ax,word ptr a+4
    mov bx,word ptr a+6
    sbb ax,dx
    sbb bx,dx
    mov word ptr r+0,cx
    mov word ptr r+2,di
    mov word ptr r+4,ax
    mov word ptr r+6,bx
  }
  return r;
}

S64 operator-(S32 a,S64 b){
  S64 r;
  _asm{
    mov ax,word ptr a+2
    mov bx,word ptr a+0
    cwd
    sub bx,word ptr b+0
    mov cx,dx
    sbb ax,word ptr b+2
    sbb cx,word ptr b+4
    sbb dx,word ptr b+6
    mov word ptr r+0,bx
    mov word ptr r+2,ax
    mov word ptr r+4,cx
    mov word ptr r+6,dx
  }
  return r;
}

S64 operator<< (S64 a,unsigned char c){
  S64 r;
  _asm{
    mov  cl,c
    and  cl,0x1F
    mov  ax,word ptr a
    mov  bx,word ptr a+2
    mov  si,word ptr a+4
    mov  di,word ptr a+6
  }
Loop:
  _asm{
    sal  ax,1
    rcl  bx,1
    rcl  si,1
    rcl  di,1
    dec  cl
    jnz  Loop

    mov  word ptr r+0,ax
    mov  word ptr r+2,bx
    mov  word ptr r+4,si
    mov  word ptr r+6,di
  }
  return r;
}

S64 operator>> (S64 a,unsigned char c){
  S64 r;
  _asm{
    mov  cl,c
    and  cl,0x1F
    mov  di,word ptr a+6
    mov  si,word ptr a+4
    mov  bx,word ptr a+2
    mov  ax,word ptr a
  }
Loop:
  _asm{
    sar  di,1
    rcr  si,1
    rcr  bx,1
    rcr  ax,1
    dec  cl
    jnz  Loop

    mov  word ptr r+0,ax
    mov  word ptr r+2,bx
    mov  word ptr r+4,si
    mov  word ptr r+6,di
  }
  return r;
}

int operator% (S64 a,int b){
  _asm{
    mov  ax,word ptr a
    mov  bx,word ptr a+2
    mov  cx,word ptr a+4
    mov  dx,word ptr a+6
    mov  si,b
    xor  di,di

    or   dx,dx    // test sign of dividend
    jns  EndIf1
    not  ax
    add  ax,1
    not  bx
    adc  bx,0
    not  cx
    adc  cx,0
    not  dx
    adc  dx,0     // negate dividend
    or   di,1
  }
EndIf1:
  _asm{
    or   si,si    // test sign of divisor
    jns  EndIf2
    neg  si       // negate divisor
    xor  di,1
  }
EndIf2:
  _asm{
    push di
    push bp
    xor  di,di    // fake a 16+64 bit dividend
    mov  bp,64
  }
Loop:
  _asm{
    shl  ax,1
    rcl  bx,1
    rcl  cx,1
    rcl  dx,1
    rcl  di,1
    cmp  di,si
    jb   NoSub
    sub  di,si
  }
NoSub:
  _asm{
    dec  bp
    jnz  Loop

    pop  bp
    pop  ax
    test ax,1
    jz   Finish
    neg  di
  }
Finish:
  return _DI;
}

int operator<(S64 a,S64 b){
  _AX = 1;
  _asm{
    mov  cx,word ptr a+6
    cmp  cx,word ptr b+6
    jl   Finish
    jg   NotLess
    mov  cx,word ptr a+4
    cmp  cx,word ptr b+4
    jb   Finish
    ja   NotLess
    mov  cx,word ptr a+2
    cmp  cx,word ptr b+2
    jb   Finish
    ja   NotLess
    mov  cx,word ptr a+0
    cmp  cx,word ptr b+0
    jb   Finish
  }
NotLess:
  _AX = 0;
Finish:
  return _AX;
}

int operator>(S64 a,S64 b){
  _AX = 1;
  _asm{
    mov  cx,word ptr a+6
    cmp  cx,word ptr b+6
    jg   Finish
    jl   NotGreater
    mov  cx,word ptr a+4
    cmp  cx,word ptr b+4
    ja   Finish
    jb   NotGreater
    mov  cx,word ptr a+2
    cmp  cx,word ptr b+2
    ja   Finish
    jb   NotGreater
    mov  cx,word ptr a+0
    cmp  cx,word ptr b+0
    ja   Finish
  }
NotGreater:
  _AX = 0;
Finish:
  return _AX;
}
