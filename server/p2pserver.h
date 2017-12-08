

#ifndef __P2PSERVER_H__
#define __P2PSERVER_H__

#include <netinet/in.h>
#include <pthread.h>
#include <map>
#include <list>
#include <string>
#include "epollctrl.h"
#include "clientnode.h"
#include "connectnode.h"
#include "typedefine.h"
#include "datapacket.h"


class P2pServerClass
{
public:
    P2pServerClass();
    ~P2pServerClass();
    int SetUdpPort(int port);
    int SetTcpPort(int port);
    int RunServer();
    int StopServer();
protected:
    ClientNode* GetClientNode(string&str);
    ClientNode* GetClientNode(uint32_t id);
    ConnectNode*GetConnectNode(uint32_t id);
    ClientNode* AddClientNode(ClientNode*pNode);
    ClientNode* DeleteClientNode(ClientNode*pNode);
    ConnectNode* AddConnectNode(ClientNode*pClient,ConnectNode*pNode);
    ConnectNode* DeleteConnectNode(ClientNode*pClient,ConnectNode*pNode);
    int receive_CL_TO_CL(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr);
    int receive_CL_TO_SV(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr);
    int receive_CN_TO_CN(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr);
    int receive_CN_TO_SV(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr);
    int receive_NO_TO_SV(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr);
    uint32_t AllotID(void);
    static void *P2PserverThread(void*arg);
    static void UdpReceiveCallBack(EpollCtrl* p_epoll,void* p_data,int events);
    static void TcpAcceptCallBack(EpollCtrl* p_epoll,void* p_data,int events);
    static void TcpReceiveCallBack(EpollCtrl* p_epoll,void* p_data,int events);
private:
    int m_runFlag;
    int m_tcpSocket;
    int m_udpSocket;
    uint32_t m_numId;
    pthread_t m_threadID;
    map<string,uint32_t> m_nameToIdMap;
    map<uint32_t,ClientNode*>m_idToClientObjMap;
    map<uint32_t,ConnectNode*>m_idToConnectObjMap;
    list<ClientNode*> m_clientNodeList;
    list<ConnectNode*> m_connectNodeList;
};


#endif //__P2PSERVER_H__
