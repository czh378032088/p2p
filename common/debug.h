


#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifndef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define MY_DEBUG

#ifdef MY_DEBUG

void syslog_printf(const char * format, ...);

#define Debug_ShowIp(ip,port)\
{\
    syslog_printf("%d.%d.%d.%d:%d",ip & 0xff,(ip >> 8) & 0xff,(ip >> 16) & 0xff,(ip >> 24) & 0xff,htons(port));\
}while(0)
#define Debug_Printf(...) syslog_printf(__VA_ARGS__)
#define Debug_ShowHex(buff,size)  \
do{\
  unsigned char *debugbuff = (unsigned char *)buff;\
  for(int debugindex = 0; debugindex < size ; debugindex ++) \
  {\
        if(debugindex != 0 && debugindex % 16 == 0)\
            syslog_printf("\n");\
        syslog_printf("%02x ",debugbuff[debugindex]);\
  }\
  if(size > 0)\
       syslog_printf("\n");\
}while(0)

#else

#define Debug_Printf(a,...) 
#define Debug_ShowHex(buff,size)

#endif

extern int useSyslog;


#ifndef __cplusplus
}
#endif

#endif

