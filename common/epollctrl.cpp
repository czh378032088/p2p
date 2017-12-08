
#include "epollctrl.h"
#include <unistd.h>
#include <stdio.h>
#include "commonfuc.h"
#include "debug.h"
 
EpollCtrl::EpollCtrl(int size)
{
    this->m_epollFd = epoll_create(size);
    this->m_timeoutIr = m_infoList.begin();
}

EpollCtrl::~EpollCtrl()
{
    close(this->m_epollFd);
    DeleteAll();
}

void EpollCtrl::AddEvent(int fd,int event,CALL_BACK_FUNC call_back,void *data,int timeout)
{
    struct epollfd_info*p_info = FindFd(fd);
    struct epoll_event epv = {0, {0}};
    int op;

    if(p_info != NULL)
    {
        op = EPOLL_CTL_MOD;  
    }
    else
    {
        //printf("add a new handle:%d\n",fd);
        p_info = new struct epollfd_info;
        p_info->fd = fd;
        this->m_infoList.push_back(p_info);
        op = EPOLL_CTL_ADD;  
    }
    
    epv.data.ptr = p_info;
    epv.events = event;

    p_info->call_back = call_back;
    p_info->events =  event ;
    p_info->last_active_time = CommonFuc::GetLTimeS();
    p_info->timeout = timeout;
    p_info->p_func_data = data;

    epoll_ctl(this->m_epollFd, op, fd, &epv) ;
}

void EpollCtrl::DeleteEvent(int fd)
{
    struct epoll_event epv = {0, {0}};
    struct epollfd_info*p_info = FindFd(fd);
    this->m_infoList.remove(p_info);
    delete p_info;
    epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, fd, &epv);  
}

void EpollCtrl::RunEvent(int waitTime)
{
    struct epoll_event eventBuff[10];
    struct epollfd_info*p_info;
    int fds = epoll_wait(this->m_epollFd, eventBuff, 10, waitTime);  
    uint32_t now_time = CommonFuc::GetLTimeS(); 
    //Debug_Printf("%s,fds = %d\n",__FUNCTION__,fds);
    for(int i = 0 ; i < fds ; i ++)
    {
        p_info = (struct epollfd_info*)eventBuff[i].data.ptr;
        //Debug_Printf("%s,fds = %d\n",__FUNCTION__,p_info->call_back);
        if(p_info->call_back != NULL)
           p_info->call_back(this,p_info->p_func_data,p_info->events);
        p_info->last_active_time = now_time;
    }
    if(this->m_infoList.size() == 0)
        return;
    if(this->m_timeoutIr == this->m_infoList.end())
    {
        this->m_timeoutIr = this->m_infoList.begin(); 
    }
    p_info = *this->m_timeoutIr;
    if(p_info->timeout >= 0 && ((now_time - p_info->last_active_time) > p_info->timeout))
    {
        struct epoll_event epv = {0, {0}};
        epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, p_info->fd, &epv);
        this->m_infoList.remove(p_info);
        p_info->call_back(NULL,p_info->p_func_data,0);
        delete p_info; 
    }
    else
    {
        this->m_timeoutIr ++;
    }
}

void EpollCtrl::DeleteAll()
{
    struct epoll_event epv = {0, {0}};
    while(!this->m_infoList.empty())
    {
        epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, this->m_infoList.front()->fd, &epv); 
        delete this->m_infoList.front();
        this->m_infoList.pop_front();
    }
}

struct epollfd_info *EpollCtrl::FindFd(int fd)
{
    list<struct epollfd_info*>::reverse_iterator  ir;
    for(ir = this->m_infoList.rbegin() ; ir != this->m_infoList.rend() ; ir ++)
    {
        if((*ir)->fd == fd)
           return *ir;
    }
    return NULL;
}
