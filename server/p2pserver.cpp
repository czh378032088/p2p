#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "p2pserver.h"
#include "debug.h"
#include "datapacket.h"
#include "commonfuc.h"


#define MAX_PACKET_SIZE   0x10000

P2pServerClass::P2pServerClass()
{
    m_runFlag = false;
    m_tcpSocket = -1;
    m_udpSocket = -1;
    m_threadID = -1;
    srand(CommonFuc::GetTimeUs());
    m_numId = rand();
}

P2pServerClass::~P2pServerClass()
{

}

int P2pServerClass::SetUdpPort(int port)
{
    struct sockaddr_in adr_inet;
    if(m_runFlag)
        return -1;
    m_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);  
    if(m_udpSocket < 0)
    {
        Debug_Printf("creat udp socket failed\n");
        return -1;
    }  
    adr_inet.sin_family = AF_INET;  
    adr_inet.sin_port   = htons(port);  
    adr_inet.sin_addr.s_addr = htonl(INADDR_ANY);  
    bzero(adr_inet.sin_zero,8);

    int ret = bind (m_udpSocket, (struct sockaddr *) &adr_inet, sizeof (adr_inet));  

    if(ret < 0)
    {   
        close(m_udpSocket);
        Debug_Printf("bind udp port %d failed\n",port);
        return - 1;
    }
    return 0;
}

int P2pServerClass::SetTcpPort(int port)
{
    struct sockaddr_in adr_inet;
    if(m_runFlag)
        return -1;
    
    m_tcpSocket = socket(AF_INET, SOCK_STREAM, 0);  
    if(m_tcpSocket < 0)
    {
        Debug_Printf("creat tcp socket failed\n");
        return -1;
    }  
    adr_inet.sin_family = AF_INET;  
    adr_inet.sin_port   = htons(port);  
    adr_inet.sin_addr.s_addr = htonl(INADDR_ANY);  
    bzero(adr_inet.sin_zero,8);

    int ret = bind (m_tcpSocket, (struct sockaddr *) &adr_inet, sizeof (adr_inet));  

    if(ret < 0)
    {   
        close(m_tcpSocket);
        Debug_Printf("bind tcp port %d failed\n",port);
        return - 1;
    }
    listen(m_tcpSocket,5);
    return 0;
}

int P2pServerClass::RunServer()
{
    if(m_udpSocket < 0)
    {
        Debug_Printf("The UDP port is not set \n");
        return -1;
    }
    m_runFlag = 1;
    int ret = pthread_create(&m_threadID,NULL,P2PserverThread,this); 
    return ret;
}

int P2pServerClass::StopServer()
{
    m_runFlag = false;
    return 0;
}

ClientNode* P2pServerClass::GetClientNode(string&str)
{
    uint32_t id = m_nameToIdMap[str];
    if(id != 0)
        return GetClientNode(id);
    return NULL;
}

ClientNode* P2pServerClass::GetClientNode(uint32_t id)
{
    return m_idToClientObjMap[id];
}

ConnectNode*P2pServerClass::GetConnectNode(uint32_t id)
{
    return m_idToConnectObjMap[id];
}

ClientNode* P2pServerClass::AddClientNode(ClientNode*pNode)
{
    m_nameToIdMap[pNode->GetClientName()] = pNode->GetClientId();
    m_idToClientObjMap[pNode->GetClientId()] = pNode;
    m_clientNodeList.push_back(pNode);
    return pNode;
}

ClientNode* P2pServerClass::DeleteClientNode(ClientNode*pNode)
{
    m_nameToIdMap.erase(pNode->GetClientName());
    m_idToClientObjMap.erase(pNode->GetClientId());
    m_clientNodeList.remove(pNode);
    delete pNode;
    return 0;
}

ConnectNode* P2pServerClass::AddConnectNode(ClientNode* pClient,ConnectNode*pConnect)
{
    pClient->AddConnectNode(pConnect);
    m_connectNodeList.push_back(pConnect);
    m_idToConnectObjMap[pConnect->GetConnectId()] = pConnect;
    return NULL;
    //m_idToConnectObjMap[]
}

ConnectNode* P2pServerClass::DeleteConnectNode(ClientNode* pClient,ConnectNode*pConnect)
{
    return NULL;
}

int P2pServerClass::receive_CL_TO_CL(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr)
{
    ClientNode*pClientNode = GetClientNode(rxPacket.GetDestinatId());
    Debug_Printf("%s\n",__FUNCTION__);
    if(pClientNode != NULL)
    {
        pClientNode->Senddata(rxPacket.GetDataBuff(),rxPacket.GetLength());
        return 0;
    }
    else
    {
        int ret = txPacket.GenErrorRetPacket(rxPacket,NoDisClientError);
        return ret;
    }
}

int P2pServerClass::receive_CL_TO_SV(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr)
{
    ClientNode*pClientNode = GetClientNode(rxPacket.GetSourceId());
    int ret = 0;

    if(pClientNode == NULL)
    {
        int ret = txPacket.GenErrorRetPacket(rxPacket,NoSrcClientError);
        return ret;
    }
    CmdType cmd = rxPacket.GetPacketCmd();

    Debug_Printf("%s:%d\n",__FUNCTION__,cmd);
    if(cmd == ReqClIdCmd)
    {
        string name = string(rxPacket.GetClentName());
        ClientNode *pdisClientNode = GetClientNode(name);
        if(pdisClientNode == NULL)
        {
            ret = txPacket.GenErrorRetPacket(rxPacket,NoDisClientError);
        }
        else
        {
            uint32_t clientId = pdisClientNode->GetClientId();
            txPacket.GenStandardRetPacket(rxPacket);
            txPacket.SetSourceId(clientId);
            txPacket.SetConnectId(rxPacket.GetDisConnectId(),rxPacket.GetSrcConnectId());
            ret = txPacket.GetLength();
        }
    }
    else if(cmd ==NoneCmd)
    {

    }
    else 
    {
        ret = txPacket.GenErrorRetPacket(rxPacket,CmdNotSupportError);
    }

    return ret;
}

int P2pServerClass::receive_CN_TO_CN(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr)
{
    ConnectNode*pConnectNode = GetConnectNode(rxPacket.GetDestinatId());
    if(pConnectNode != NULL)
    {
        //Debug_Printf("receive_CN_TO_CN%d,%d\n",rxPacket.GetSourceId(),rxPacket.GetDestinatId());
        pConnectNode->Senddata(rxPacket.GetDataBuff(),rxPacket.GetLength());
        return 0;
    }
    else
    {
        //Debug_Printf("receive_CN_TO_CN%d,%d Error\n",rxPacket.GetSourceId(),rxPacket.GetDestinatId());
        int ret = txPacket.GenErrorRetPacket(rxPacket,NoDisConnectError);
        return ret;
    }
    return 0;
}

int P2pServerClass::receive_CN_TO_SV(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr)
{
    return 0;
}

int P2pServerClass::receive_NO_TO_SV(DataPacket &rxPacket,DataPacket &txPacket,int fd,sockaddr_in &addr)
{
    CmdType cmd = rxPacket.GetPacketCmd();
    int ret = 0;

    Debug_Printf("%s:%d\n",__FUNCTION__,cmd);

    if(cmd == RegClCmd)
    {
        string name = string(rxPacket.GetClentName());
        ClientNode*pnode = GetClientNode(name);
        if(pnode == 0)
        {
            pnode = AddClientNode(new ClientNode(name,AllotID()));
        }
        pnode->SetIntAddr(addr);
        pnode->SetSocketHandle(fd);
        txPacket.GenStandardRetPacket(rxPacket);
        txPacket.SetDestinatId(pnode->GetClientId());
        txPacket.PushData((uint8_t*)&addr,sizeof(sockaddr_in));
        ret = txPacket.GetLength();
    }
    else if(cmd == RegCnCmd)
    {
        ClientNode*pnode = GetClientNode(rxPacket.GetSourceId());
        if(pnode == NULL)
        {
            ret = txPacket.GenErrorRetPacket(rxPacket,NoSrcClientError);
            return ret;
        }
        ConnectNode*pConnect = AddConnectNode(pnode,new ConnectNode(AllotID(),0));
        pConnect->SetIntAddr(addr);
        pConnect->SetSocketHandle(fd);
        pConnect->SetClientNode(pnode);
        txPacket.GenStandardRetPacket(rxPacket);
        txPacket.SetDestinatId(pConnect->GetConnectId());
        txPacket.PushData((uint8_t*)&addr,sizeof(sockaddr_in));
        ret = txPacket.GetLength();
        Debug_Printf("注册连接：%d\n",pConnect->GetConnectId());
    }
    else if(cmd == ReqIpCmd)
    {
        txPacket.GenStandardRetPacket(rxPacket);
        txPacket.PushData((uint8_t*)&addr,sizeof(sockaddr_in));
        ret = txPacket.GetLength();
    }
    else 
    {
        ret = txPacket.GenErrorRetPacket(rxPacket,CmdNotSupportError);
    }

    return ret;
}

uint32_t P2pServerClass::AllotID(void)
{
    m_numId ++;
    if(m_numId == 0)
        m_numId ++;
    return m_numId;
}

void *P2pServerClass::P2PserverThread(void*arg)
{
    P2pServerClass *p_this = (P2pServerClass *)arg;
    EpollCtrl epollCtrl(2048);

    epollCtrl.AddEvent(p_this->m_udpSocket,EPOLLIN,UdpReceiveCallBack,p_this);
    if(p_this->m_tcpSocket > 0)
        epollCtrl.AddEvent(p_this->m_tcpSocket,EPOLLIN,TcpAcceptCallBack,p_this);

    Debug_Printf("%s begin\n",__FUNCTION__);

    while(p_this->m_runFlag == 1)
    {
        epollCtrl.RunEvent(10);
    }

    Debug_Printf("%s end\n",__FUNCTION__);
    return NULL;
}

void P2pServerClass::UdpReceiveCallBack(EpollCtrl* p_epoll,void* p_data,int events)
{
    uint8_t rxBuff[MAX_PACKET_SIZE];
    uint8_t txBuff[MAX_PACKET_SIZE];
    int  rxLen = 0;
    int  txLen = 0;
    struct sockaddr_in src_inet;
    socklen_t len = sizeof(struct sockaddr_in);
    P2pServerClass *p_this = (P2pServerClass *)p_data;
    DataPacket rxPacket(rxBuff,0);
    DataPacket txPacket(txBuff,0);

    //Debug_Printf("%s\n",__FUNCTION__);
    rxLen = recvfrom(p_this->m_udpSocket,rxBuff,MAX_PACKET_SIZE,MSG_DONTWAIT, (struct sockaddr *) &src_inet,&len);
    //Debug_Printf("rxLen = %d\n",rxLen);
    rxPacket.UpdateLength(rxLen);
    txLen = txPacket.GenAutoErrorRetPacket(rxPacket);
    if(txLen > 0)
    {
        Debug_Printf("Error:%d\n",txPacket.GetErrorCode());
        sendto(p_this->m_udpSocket,txBuff,txLen,MSG_DONTWAIT,(struct sockaddr *) &src_inet,len);
        return;
    }
    PacketType ptype = rxPacket.GetPacketType();
    //Debug_Printf("ptype=%d\n",ptype);
    switch(ptype)
    {
        case CL_TO_CL:
            txLen = p_this->receive_CL_TO_CL(rxPacket,txPacket,p_this->m_udpSocket,src_inet);
            break;
        case CL_TO_SV:
            txLen = p_this->receive_CL_TO_SV(rxPacket,txPacket,p_this->m_udpSocket,src_inet);
            break;
        case CN_TO_CN:
            txLen = p_this->receive_CN_TO_CN(rxPacket,txPacket,p_this->m_udpSocket,src_inet);
            break;
        case CN_TO_SV:
            txLen = p_this->receive_CN_TO_SV(rxPacket,txPacket,p_this->m_udpSocket,src_inet);
            break;
        case NO_TO_SV:
            txLen = p_this->receive_NO_TO_SV(rxPacket,txPacket,p_this->m_udpSocket,src_inet);
            break;
        default:
            txLen = txPacket.GenErrorRetPacket(rxPacket,PacketTypeNotSupportError);
            break;
    }

    if(txLen > 0)
    {
        //Debug_ShowHex(rxBuff,rxLen);
        sendto(p_this->m_udpSocket,txBuff,txLen,MSG_DONTWAIT,(struct sockaddr *) &src_inet,len);
    }
}

void P2pServerClass::TcpAcceptCallBack(EpollCtrl* p_epoll,void* p_data,int events)
{
    Debug_Printf("%s\n",__FUNCTION__);
    P2pServerClass *p_this = (P2pServerClass *)p_data;
    struct sockaddr_in adr_inet;
    int  handle;
    socklen_t len;

    handle = accept(p_this->m_tcpSocket, (struct sockaddr *) &adr_inet,&len);

    if(handle > 0)
    {
        Debug_Printf("tcp handle = %x\n",handle);
        //pEpollCtrl->AddEvent(handle,EPOLLIN,TcpReceiveCallBack);
        //tcpClientAddrMap[handle] = adr_inet;
    }   
}

void P2pServerClass::TcpReceiveCallBack(EpollCtrl* p_epoll,void* p_data,int events)
{
    Debug_Printf("%s\n",__FUNCTION__);
}
