
#ifndef __CLIENTCONNECT_H__
#define __CLIENTCONNECT_H__

#include "connectchannel.h"
#include "typedefine.h"
#include <string>
#include <netinet/in.h>

#define MAX_CHANNEL  16



using namespace std;

class ClientDevice;
class ClientConnect;

typedef int (*ConnectedCallBack)(ClientConnect *);

class ClientConnect
{
public:
    ClientConnect(ClientDevice*p_client,int id,int isServer = 0);
    ~ClientConnect();
    int StartConnect(uint16_t port,string &remoteClientName,int isTcp = 0);
    int SendData(const uint8_t buff[],int len,sockaddr_in *addr = NULL);
    int ReceiveData(uint8_t buff[],int len,sockaddr_in *addr = NULL);
    int StopConnect(void);
    void SetRemoteClientId(uint32_t id);
    void SetRemoteConnectIdAddr(uint8_t conMode,int id,void *addr);
    void SetConnectedCallBack(ConnectedCallBack p);
    ClientDevice *GetClientDevice(void);
    uint32_t GetLocalId(void);
    uint32_t GetRemoteId(void);
    ConnectChannel *CreateConnectChannel(int isTcp);
    int GetChannelNum(void);
    ConnectChannel *GetChannel(int id);
    void ResetConnect(void);
protected:
    static void *P2PconnectThread(void*arg);
    void GetOwnerAddr(void);
    void GetRemoteClientId(void);
    void TryConnectRomoteClient(void);
    void CreatServerConnect(void);
    void RunAfterConnected(void);
private:
    ConnectChannel *m_connectChannel[MAX_CHANNEL];
    string m_remoteClientName;
    uint32_t m_lastSendTime;
    int m_channelNum;
    int m_runFlag;
    int m_id;
    int m_remoteId;
    int m_connectSocket;
    int m_isTcp;
    int m_isServer ;
    int m_connectedFlag;
    int m_connectMode;
    int m_getRemoteIdFlag;
    uint32_t m_ownerId;
    uint32_t m_remoteClientId;
    uint32_t m_remoteConnectId;
    pthread_t m_threadID;
    ClientDevice *m_pClientDevice;
    sockaddr_in m_serverAddr;
    sockaddr_in m_localAddr;
    sockaddr_in m_interAddr;
    sockaddr_in m_connectAddr;

    ConnectedCallBack m_connectedCallBack;
};

#endif
