#ifndef __CLIENTDEVICE_H__
#define __CLIENTDEVICE_H__

#include <netinet/in.h>
#include <string>
#include <list>
#include <pthread.h>
#include "clientconnect.h"
#include "typedefine.h"

using namespace std;

#define MAX_CONNECT    8

class ClientDevice
{
public:
    ClientDevice(string clientName);
    int StartDevice(const char ip[],uint16_t port);
    int StopDevice();
    int SendData(const uint8_t buff[],int len);
    int ReceiveData(uint8_t buff[],int len,sockaddr_in *addr = NULL);
    ClientConnect *CreatClientConnect(uint16_t port,string remoteClientName,int isTcp = 0);
    sockaddr_in GetServerSocketAddr(void);
    sockaddr_in GetLocalSocketAddr(void);
    string GetName(void);
    int GetRegFlag(void);
    uint32_t GetClientId(void);
    void SetConnectedCallBack(void* p);
    int GetClientConnectNum(void);
    ClientConnect *GetClientConnect(int id);
protected:
    static void *P2PclientThread(void*arg);
    void GetLocalAddr(void);
    int  RegisterDevice(void);
    void HandleReceiveData(void);
private:
    string m_clientName;
    ClientConnect *m_clientConnect[MAX_CONNECT];
    uint32_t m_connectedFlag[MAX_CONNECT] ;
    //uint32_t m_localIp;
    uint32_t m_lastSendTime;
    int m_runFlag;
    int m_connectNum;
    int m_serverSocket;
    int m_regClientFlag;
    uint32_t m_clientId;
    pthread_t m_threadID;
    pthread_mutex_t m_sendlock;
    sockaddr_in m_serverAddr;
    sockaddr_in m_localAddr;
    sockaddr_in m_interAddr;
    void*m_connectedCallBack;
};

#endif
