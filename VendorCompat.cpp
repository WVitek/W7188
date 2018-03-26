#include "WKern.hpp"

// Is need for vendor library functions
extern "C" {
  void cdecl free( void *__ptr ){
    SYS::free(__ptr);
  }
  void cdecl * malloc( size_t __size ){
    void* mem=SYS::malloc(__size);
    return mem;
  }
  int cdecl atexit( void (*func)(void) ){
    return -1;
  }
  int cdecl sprintf( char *buf, const char *format, ... ){
    buf[0]='@';
    buf[1]='\0';
    return 1;
  }
}

