#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "connectchannel.h"
#include "clientconnect.h"
#include "datapacket.h"
#include "debug.h"

ConnectChannel::ConnectChannel(ClientConnect *pConnect,int conId,int isTcp)
{
    m_channelNum = conId;
    if(isTcp)
    {
        m_localSocket = socket(AF_INET, SOCK_STREAM, 0);  
        //m_listenSocket = m_localSocket;
        int flags = fcntl(m_localSocket, F_GETFL, 0);  
        fcntl(m_localSocket, F_SETFL, flags | O_NONBLOCK);
    }       
    else
    {
        m_localSocket = socket(AF_INET, SOCK_DGRAM, 0);  
    }
    m_isTcp = isTcp;
    m_isConnected = 0;
    m_localIndex = m_remoteIndex = 0;
    m_pConnect = pConnect;

    for(int i = 0 ; i < MAX_BUFF_NUM ; i ++)
    {
        DataPacket packet(m_localBuff[i],0);
        packet.GenReplayPacket(m_pConnect->GetLocalId(),m_pConnect->GetRemoteId(),m_channelNum);
        m_remoteLen[i] = 0;
        m_localLen[i] = 0;
    }
    m_localToRemoteFunc = NULL;
    m_remoteToLocalFunc = NULL;
    m_totalLocalLen = m_totalRemoteLen = 0;
}

ConnectChannel::~ConnectChannel()
{
    if(m_localSocket > 0)
        close(m_localSocket);
    if(m_listenSocket > 0) 
        close(m_listenSocket);
}

int ConnectChannel::BindPort(int port)
{
    struct sockaddr_in adr_inet;
    int ret;
    int iSockOptVal = 1;

    adr_inet.sin_family = AF_INET;  
    adr_inet.sin_port   = htons(port);  
    adr_inet.sin_addr.s_addr = htonl(INADDR_ANY);  
    bzero(adr_inet.sin_zero,8);
    setsockopt(m_localSocket, SOL_SOCKET, SO_REUSEADDR, &iSockOptVal, sizeof(iSockOptVal)) ;
    ret = bind (m_localSocket, (struct sockaddr *) &adr_inet, sizeof (adr_inet));  
    if(ret < 0)
    {
        Debug_Printf("ConnectChannel::BindPort error\n");
    }

    if(m_isTcp)
    {
        listen(m_localSocket,5);
        m_listenSocket = m_localSocket;
        m_localSocket = - 1 ;
    }
    return 0;
}

int ConnectChannel::ConnectAddr(const char ip[],int port)
{
    m_localAddr.sin_family = AF_INET;  
    m_localAddr.sin_port   = htons(port);  
    m_localAddr.sin_addr.s_addr = inet_addr(ip);  
    bzero(m_localAddr.sin_zero,8);
    
    if(m_isTcp == 0)
    {
        m_isConnected = 1;
    }
    else 
    {
        m_listenSocket = - 1;
    }
    return 0;
}

int ConnectChannel::ReplayRemotePacket(uint8_t buff[],int len)
{
    //Debug_Printf("ReplayRemotePacket\n");
    if(m_isConnected == 0)
        return -1;
    //Debug_Printf("ReplayRemotePacket1\n");
    DataPacket rxPacket(buff,len);
    int packetNum = rxPacket.GetSerialNum();
    if(packetNum == m_remoteIndex)
    {
        LocalSendData(rxPacket.GetDataField(),rxPacket.GetDataFieldSize());
        m_remoteIndex ++;
        for(int i = 0 ; i < MAX_BUFF_NUM / 2 ; i ++)
        {
            uint8_t dataIndex = m_remoteIndex & (MAX_BUFF_NUM - 1);
            DataPacket bfPacket(m_remoteBuff[dataIndex],m_remoteLen[dataIndex]);

            if(m_remoteLen[dataIndex] > 0 && bfPacket.GetSerialNum() == m_remoteIndex)
            {
                LocalSendData(bfPacket.GetDataField(),bfPacket.GetDataFieldSize());
                m_remoteIndex ++;
            }
            else 
            {
                m_remoteLen[dataIndex] = 0;
                break;
            }
        }
    }
    else 
    {
        uint8_t disNum = (packetNum + 256) - m_remoteIndex;
        Debug_Printf("packetNum %d ！= m_remoteIndex %d\n",packetNum,m_remoteIndex);
        if(disNum < MAX_BUFF_NUM / 2)
        {
            uint8_t mark = MAX_BUFF_NUM - 1;
            uint8_t dataIndex = packetNum & mark;
            uint8_t buffIndex = m_remoteIndex & mark;
            uint8_t indexBuff[MAX_BUFF_NUM];
            int num = 0;

            memcpy(m_remoteBuff[dataIndex],buff,len);
            m_remoteLen[dataIndex] = len;
            if(dataIndex > buffIndex)
            {
                for(int i = buffIndex ; i < dataIndex ; i ++)
                {
                    if(m_remoteLen[i] == 0)
                    {
                        indexBuff[num] = m_remoteIndex + i - buffIndex;
                        num ++;
                    } 
                }
            }
            else
            {
                for(int i = buffIndex ; i < dataIndex + MAX_BUFF_NUM; i ++)
                {
                    if(m_remoteLen[i & mark] == 0)
                    {
                        indexBuff[num] = m_remoteIndex + i - buffIndex;
                        num ++;
                    } 
                }
            }
            if(num > 0)
                SendResendPacket(indexBuff,num);
        }
        else if(disNum < MAX_BUFF_NUM)
        {
            for(int i = 0 ; i <=  (disNum - MAX_BUFF_NUM / 2) ; i ++)
            {
                uint8_t dataIndex = m_remoteIndex & (MAX_BUFF_NUM - 1);
                DataPacket bfPacket(m_remoteBuff[dataIndex],m_remoteLen[dataIndex]);

                if(m_remoteLen[dataIndex] > 0 && bfPacket.GetSerialNum() == m_remoteIndex)
                {
                    LocalSendData(bfPacket.GetDataField(),bfPacket.GetDataFieldSize());
                }
                m_remoteLen[dataIndex] = 0;
                m_remoteIndex ++;
            }

            for(int i = 0 ; i < MAX_BUFF_NUM / 2 ; i ++)
            {
                uint8_t dataIndex = m_remoteIndex & (MAX_BUFF_NUM - 1);
                DataPacket bfPacket(m_remoteBuff[dataIndex],m_remoteLen[dataIndex]);

                if(m_remoteLen[dataIndex] > 0 && bfPacket.GetSerialNum() == m_remoteIndex)
                {
                    LocalSendData(bfPacket.GetDataField(),bfPacket.GetDataFieldSize());
                    m_remoteIndex ++;
                }
                else 
                {
                    m_remoteLen[dataIndex] = 0;
                    break;
                }
            }
            uint8_t mark = MAX_BUFF_NUM - 1;
            uint8_t dataIndex = packetNum & mark;
            uint8_t buffIndex = m_remoteIndex & mark;
            uint8_t indexBuff[MAX_BUFF_NUM];
            int num = 0;

            memcpy(m_remoteBuff[dataIndex],buff,len);
            m_remoteLen[dataIndex] = len;
            if(dataIndex > buffIndex)
            {
                for(int i = buffIndex ; i < dataIndex ; i ++)
                {
                    if(m_remoteLen[i] == 0)
                    {
                        indexBuff[num] = m_remoteIndex + i - buffIndex;
                        num ++;
                    } 
                }
            }
            else
            {
                for(int i = buffIndex ; i < dataIndex + MAX_BUFF_NUM; i ++)
                {
                    if(m_remoteLen[i & mark] == 0)
                    {
                        indexBuff[num] = m_remoteIndex + i - buffIndex;
                        num ++;
                    } 
                }
            }
            if(num > 0)
                SendResendPacket(indexBuff,num);
        }
        else if(disNum < (256 - MAX_BUFF_NUM))
        {
            for(int i = 0 ; i < MAX_BUFF_NUM ; i ++)
            {
                m_remoteLen[i] = 0;
            }
            LocalSendData(rxPacket.GetDataField(),rxPacket.GetDataFieldSize());
            m_remoteIndex = packetNum + 1;
        }
    }
    return 0;
}

int ConnectChannel::SendLocalPacket(void)
{
    uint8_t mark = MAX_BUFF_NUM - 1;
    int packetIndex = m_localIndex & mark;
    DataPacket rxPacket(m_localBuff[packetIndex],0);
    int len = LocalReceiveData(rxPacket.GetDataField(),MAX_BUFF_SIZE);
    if(len > 0)
    {
        len += rxPacket.GetPacketHeadSize();
        //rxPacket.UpdateLength(len + rxPacket.GetPacketHeadSize());
        rxPacket.SetSerialNum(m_localIndex);
        m_localLen[packetIndex] = len;
        m_localIndex ++;
        //if(packetIndex != 8 &&packetIndex != 9 &&packetIndex != 10 && packetIndex != 11)
        ///if((rand() % 3) != 1)
        m_pConnect->SendData(m_localBuff[packetIndex],len);
    }

    return len;
}

int ConnectChannel::LocalSendData(uint8_t buff[],int len)
{
   int ret = -1;
    //Debug_Printf((char*)buff);
    if(m_isConnected == 0)
        return -1;
    
    if(m_remoteToLocalFunc != NULL)
        len = m_remoteToLocalFunc(this,buff,len);
    m_totalRemoteLen += len;
    if(m_isTcp)
    {
        ret = send(m_localSocket,buff,len,MSG_DONTWAIT);
    }
    else 
    {
        ret = sendto(m_localSocket,buff,len,MSG_DONTWAIT,(struct sockaddr *) &m_localAddr,sizeof(m_localAddr));
    }
    return ret;
}

int ConnectChannel::LocalReceiveData(uint8_t buff[],int len)
{
    int ret = -1;
    
    if(m_isTcp)
    {
        if(m_listenSocket > 0 )
        {
            sockaddr_in addr;
            socklen_t addrLen = sizeof(sockaddr_in);
            int tempSocket = accept(m_listenSocket,(struct sockaddr *) &addr,&addrLen);
            if(tempSocket > 0 )
            {
                m_localSocket = tempSocket;
                m_localAddr = addr;
                m_isConnected = 1;
            }
        }
        else if(m_isConnected == 0)
        {
            if(connect(m_localSocket,(const struct sockaddr *)(&m_localAddr) ,sizeof(m_localAddr)) == 0)
            {
                m_isConnected = 1;
            }
        }

        if(m_isConnected && m_localSocket > 0)
        {
            ret = recv(m_localSocket,buff,len,MSG_DONTWAIT);
            /*if(ret > 0)
                Debug_Printf((char*)buff);*/
        }
    }
    else 
    {
        if(m_localSocket > 0)
        {
            socklen_t addrLen = sizeof(sockaddr_in);
            ret = recvfrom(m_localSocket,buff,len,MSG_DONTWAIT, (struct sockaddr *) &m_localAddr,&addrLen);
            if(ret > 0)
            {
                m_isConnected = 1;
            }
        }
    }

    if(ret > 0)
    {
        m_totalLocalLen += ret;
        if(m_localToRemoteFunc != NULL)
            ret = m_localToRemoteFunc(this,buff,ret);
    }
        
    return ret;
}

void ConnectChannel::SetCallBack(ChannelCallBack lo_rem_func,ChannelCallBack rem_lo_func)
{
    m_localToRemoteFunc = lo_rem_func;
    m_remoteToLocalFunc = rem_lo_func;
}

ClientConnect* ConnectChannel::GetClientConnect(void)
{
    return m_pConnect;
}

uint32_t ConnectChannel::GetTotalLocalLen(void)
{
    return m_totalLocalLen;
}

uint32_t ConnectChannel::GetTotalRemoteLen(void)
{
    return m_totalRemoteLen;
}

void ConnectChannel::ResendLocalPacket(uint8_t resendNum[],int size)
{
    for(int i = 0 ; i < size ; i ++)
    {
        uint8_t dataIndex = resendNum[i] & (MAX_BUFF_NUM - 1);
        DataPacket bfPacket(m_localBuff[dataIndex],m_localLen[dataIndex]);
        if(m_localLen[dataIndex] > 0 && bfPacket.GetSerialNum() == resendNum[i])
        {
            m_pConnect->SendData(m_localBuff[dataIndex],m_localLen[dataIndex]);
        }
    }
}

void ConnectChannel::SendResendPacket(uint8_t resendNum[],int size)
{
    uint8_t txBuff[2048];
    int  txLen = 0;
    DataPacket txPacket(txBuff,0);
    txLen = txPacket.GenReSendPacket(m_pConnect->GetLocalId(),m_pConnect->GetRemoteId(),m_channelNum,resendNum,size);
    m_pConnect->SendData(txBuff,txLen);
}
