


#ifndef __CLIENTNODE_H__
#define __CLIENTNODE_H__

#include <list>
#include <sys/epoll.h>
#include "typedefine.h"

using namespace std;

class EpollCtrl;

typedef void (*CALL_BACK_FUNC)(EpollCtrl*,void*,int events);

struct epollfd_info
{
    int fd;
    CALL_BACK_FUNC call_back;
    void *p_func_data;
    int events;
    int timeout;
    uint32_t last_active_time;
};


class EpollCtrl
{
public:
    EpollCtrl(int size);
    ~EpollCtrl();
    void AddEvent(int fd,int event,CALL_BACK_FUNC call_back,void *data,int timeout = -1);
    void DeleteEvent(int fd);
    void RunEvent(int waitTime);
    void DeleteAll();
protected:
    struct epollfd_info *FindFd(int fd);
private:
    list<struct epollfd_info*> m_infoList;
    int m_epollFd;
    int m_maxSize;
    list<struct epollfd_info*>::iterator  m_timeoutIr;
};

#endif

