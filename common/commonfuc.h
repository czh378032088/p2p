
#ifndef __COMMONFUNC_H__
#define __COMMONFUNC_H__

#include "typedefine.h"

class CommonFuc
{
public:
   static uint32_t GetSTimeMs(void);
   static uint64_t GetLTimeMs(void);
   static uint64_t GetTimeMs(void);
   static uint32_t GetSTimeUs(void);
   static uint64_t GetLTimeUs(void);
   static uint64_t GetTimeUs(void);
   static uint32_t GetSTimeS(void);
   static uint64_t GetLTimeS(void);
   static uint64_t GetTimeS(void);

   static bool IsRequestType(char *data,const char *tagRequest);
   static char* FindString(const char *data,const char *subStr,bool retBegin);
   static int ReplaceString(char *data,const char *beginStr,const char *endStr,const char *replaceStr,int len);
   static int ReplacePortNum(char *data,const char *beginStr,const char *replaceStr,int len);
   static int GetPortNum(char *data,const char *beginStr,char *portStr,int len);
};


#endif 


