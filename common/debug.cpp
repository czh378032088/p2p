#include <syslog.h>  
#include <stdio.h>
#include <stdarg.h>

int useSyslog = 0, syslog_opened = 0;

void syslog_printf(const char * format, ...) 
{
    va_list va_ap;
    char buf[2048];

    va_start (va_ap, format);
    vsnprintf(buf, sizeof(buf)-1, format, va_ap);
    va_end(va_ap);

    if(useSyslog) 
    {
      if(!syslog_opened) 
      {
        openlog("p2p", LOG_PID, LOG_DAEMON);
        syslog_opened = 1;
      }
      syslog(LOG_INFO, "%s", buf);
    } 
    else 
    {
      printf("%s", buf);
    }
}
