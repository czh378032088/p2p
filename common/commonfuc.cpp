#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "commonfuc.h"


uint32_t CommonFuc::GetSTimeMs(void)
{
    return (uint32_t)GetLTimeMs();
}

uint64_t CommonFuc::GetLTimeMs(void)
{
    struct  timeval    tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t CommonFuc::GetTimeMs(void)
{
    return GetLTimeMs();
}

uint32_t CommonFuc::GetSTimeUs(void)
{
    return (uint32_t)GetLTimeUs();
}

uint64_t CommonFuc::GetLTimeUs(void)
{
    struct  timeval    tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

uint64_t CommonFuc::GetTimeUs(void)
{
    return GetLTimeUs();
}

uint32_t CommonFuc::GetSTimeS(void)
{
    return (uint32_t)GetLTimeS();
}

uint64_t CommonFuc::GetLTimeS(void)
{
    struct  timeval    tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec;
}

uint64_t CommonFuc::GetTimeS(void)
{
    return GetLTimeS();
}


bool CommonFuc::IsRequestType(char *data,const char *tagRequest)
{
    int i = 0;
    while(1)
    {
        if(tagRequest[i] == 0)
           return true;
        
        if(data[i] != tagRequest[i])
           return false;
        i ++;
    }
}

char* CommonFuc::FindString(const char *data,const char *subStr,bool retBegin)
{
    char *p1 = (char *)data;
    char *p2 = (char *)subStr;
    char *p3 = p1;

    while(*p1 != 0 && *p2 != 0)
    {
        if(*p1 == *p2)
        {
            p1 ++;
            p2 ++;
        }
        else
        {
            p2 = (char *)subStr;
            p3 ++;
            p1 = p3;
        }
    }

    if(*p2 != 0)
       return NULL;
    if(retBegin)
    {
        return p3;
    }
    else 
    {
        return p1;
    }
}

int CommonFuc::ReplaceString(char *data,const char *beginStr,const char *endStr,const char *replaceStr,int len)
{
    char *first ,*last;
    int newlen = len;
    
    first = FindString(data,beginStr,false);
    //Debug_Printf("first = %d\n",first - data);
    if(first == NULL)
       return newlen;
    if(*endStr == 0)
       last = data + len;
    else 
       last = FindString(first,endStr,true);

    int replaceLen = strlen(replaceStr);
    if(last == NULL)
    {
        memcpy(first,replaceStr,replaceLen);
        return newlen;
    }

    int movesize = replaceLen - (int)(last - first);
    if(movesize > 0)
    {
        //Debug_Printf("movesize > 0===================================\n");
        char *p1 = data + len + movesize;
        char *p2 = data + len;
        while(1)
        {
            *p1 = *p2;
            if(p2 == last)
               break;
            p1 --;
            p2 --;
        }
    }
    else if(movesize < 0)
    {
        //Debug_Printf("movesize < 0===================================%d\n",movesize);
        char *p1 = last + movesize;
        char *p2 = last;
        while(1)
        {
            //Debug_Printf("movesize < 0===================================%c\n",*p2);
            *p1 = *p2;
            if(*p2 == 0)
               break;
            p1 ++;
            p2 ++;
        }
    }

    memcpy(first,replaceStr,replaceLen);
    newlen = len + movesize;
    return newlen;
}

int CommonFuc::ReplacePortNum(char *data,const char *beginStr,const char *replaceStr,int len)
{
    char *first ,*last;
    int newlen = len;
    
    first = FindString(data,beginStr,false);
    //Debug_Printf("first = %d\n",first - data);
    if(first == NULL)
       return newlen;
    last = first;

    while(*last == '-' || (*last <= '9' && *last >= '0'))
    {
        last ++;
    }

    int replaceLen = strlen(replaceStr);
    if(last == NULL)
    {
        memcpy(first,replaceStr,replaceLen);
        return newlen;
    }

    int movesize = replaceLen - (int)(last - first);
    if(movesize > 0)
    {
        //Debug_Printf("movesize > 0===================================\n");
        char *p1 = data + len + movesize;
        char *p2 = data + len;
        while(1)
        {
            *p1 = *p2;
            if(p2 == last)
               break;
            p1 --;
            p2 --;
        }
    }
    else if(movesize < 0)
    {
        //Debug_Printf("movesize < 0===================================%d\n",movesize);
        char *p1 = last + movesize;
        char *p2 = last;
        while(1)
        {
            //Debug_Printf("movesize < 0===================================%c\n",*p2);
            *p1 = *p2;
            if(*p2 == 0)
               break;
            p1 ++;
            p2 ++;
        }
    }

    memcpy(first,replaceStr,replaceLen);
    newlen = len + movesize;
    return newlen;
}


int CommonFuc::GetPortNum(char *data,const char *beginStr,char *portStr,int len)
{
    char *p1 ,*p2 = portStr;
    int retlen = 0;
    
    p1 = FindString(data,beginStr,false);
    //Debug_Printf("first = %d\n",first - data);
    if(p1 == NULL)
       return -1;


    while(*p1 == '-' || (*p1 <= '9' && *p1 >= '0'))
    {
        *p2 ++ = *p1 ++;
        retlen ++;
    }
    *p2 = 0;
    return retlen;
}
