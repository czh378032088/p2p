
#ifndef __CONNECTCHANNEL_H__
#define __CONNECTCHANNEL_H__

#include <netinet/in.h>
#include "typedefine.h"


#define MAX_BUFF_SIZE 0x1000
#define MAX_BUFF_NUM  0x10

class ClientConnect;
class ConnectChannel;

typedef int (*ChannelCallBack)(ConnectChannel *pChannel,uint8_t data[],int len);

class ConnectChannel
{
public:
    ConnectChannel(ClientConnect *pConnect,int conId,int isTcp);
    ~ConnectChannel();
    int BindPort(int port);
    int ConnectAddr(const char ip[],int port);
    int ReplayRemotePacket(uint8_t buff[],int len);
    int SendLocalPacket(void);
    int LocalSendData(uint8_t buff[],int len);
    int LocalReceiveData(uint8_t buff[],int len);
    void SetCallBack(ChannelCallBack lo_rem_func,ChannelCallBack rem_lo_func);
    ClientConnect* GetClientConnect(void);
    uint32_t GetTotalLocalLen(void);
    uint32_t GetTotalRemoteLen(void);
    void ResendLocalPacket(uint8_t resendNum[],int size);
protected:
    void SendResendPacket(uint8_t resendNum[],int size);
private:
    uint8_t m_remoteBuff[MAX_BUFF_NUM][MAX_BUFF_SIZE];
    uint8_t m_localBuff[MAX_BUFF_NUM][MAX_BUFF_SIZE];
    int m_remoteLen[MAX_BUFF_NUM];
    int m_localLen[MAX_BUFF_NUM];
    uint32_t m_totalLocalLen;
    uint32_t m_totalRemoteLen;
    uint8_t m_localIndex;
    uint8_t m_remoteIndex;
    uint8_t m_remoteLastIndex;
    int m_localSocket;
    int m_listenSocket;
    int m_channelNum;
    int m_isTcp;
    int m_isConnected;
    ClientConnect*m_pConnect;
    sockaddr_in m_localAddr;
    ChannelCallBack m_localToRemoteFunc;
    ChannelCallBack m_remoteToLocalFunc;
};

#endif
