#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "clientconnect.h"
#include "clientdevice.h"
#include "commonfuc.h"
#include "debug.h"
#include "datapacket.h"

ClientConnect::ClientConnect(ClientDevice*p_client,int id,int isServer)
{
    m_pClientDevice = p_client;
    m_remoteClientId = 0;
    m_id = id;
    m_isServer = isServer;
    m_remoteId = 0xf;
    m_connectedFlag = 0;
    m_connectMode = 0;
    m_getRemoteIdFlag = 0;
    m_ownerId = 0;
    m_connectedCallBack = NULL;
    m_channelNum = 0;
}

ClientConnect::~ClientConnect()
{
    m_runFlag = false;
    ResetConnect();        
}

int ClientConnect::StartConnect(uint16_t port,string &remoteClientName,int isTcp)
{
    //socklen_t len = sizeof(struct sockaddr_in);
    //while(m_pClientDevice->GetRegFlag() == 0);

    m_serverAddr = m_pClientDevice->GetServerSocketAddr();
    m_serverAddr.sin_port = htons(port);
/*
    if(m_isServer)
    {
        Debug_Printf("服务器地址为");
        Debug_ShowIp(m_serverAddr.sin_addr.s_addr,m_serverAddr.sin_port);
        Debug_Printf("\n");
    }*/
    //m_connectAddr = m_serverAddr;

    m_remoteClientName = remoteClientName;
    m_isTcp = isTcp;
    if(m_isTcp)
    {

    }
    else 
    {
        m_connectSocket = socket(AF_INET, SOCK_DGRAM, 0);  
    }
    /*getsockname(m_connectSocket,(struct sockaddr*)&m_localAddr, &len);
    m_localAddr.sin_addr.s_addr = m_pClientDevice->GetLocalSocketAddr().sin_addr.s_addr;

    Debug_Printf("%s connect %d addr",m_pClientDevice->GetName().c_str(),m_id);
    Debug_ShowIp(m_localAddr.sin_addr.s_addr,m_localAddr.sin_port);
    Debug_Printf("\n");*/

    m_runFlag = 1;
    int ret = pthread_create(&m_threadID,NULL,P2PconnectThread,this); 
    return ret;
}

int ClientConnect::SendData(const uint8_t buff[],int len,sockaddr_in *addr)
{
    int ret ;
    if(addr == NULL)
    {
        ret = sendto(m_connectSocket,buff,len,MSG_DONTWAIT,(struct sockaddr *) &m_connectAddr,sizeof (m_connectAddr));
        //Debug_ShowIp(m_connectAddr.sin_addr.s_addr,m_connectAddr.sin_port);
        //Debug_Printf("\n");
    }
    else 
    {
        ret = sendto(m_connectSocket,buff,len,MSG_DONTWAIT,(struct sockaddr *) addr,sizeof (sockaddr_in));
    }
        
    m_lastSendTime = CommonFuc::GetTimeS();
    return ret;
}

int ClientConnect::ReceiveData(uint8_t buff[],int len,sockaddr_in *addr)
{
    if(addr == NULL)
        return recv(m_connectSocket,buff,len,MSG_DONTWAIT);
    else 
    {
        socklen_t len = sizeof(struct sockaddr_in);
        return recvfrom(m_connectSocket,buff,len,MSG_DONTWAIT,(struct sockaddr *) addr,&len);
    }
}

int ClientConnect::StopConnect(void)
{
    m_runFlag = 0;
    return 0;
}

void ClientConnect::SetRemoteClientId(uint32_t id)
{
    m_remoteClientId = id;
}

void ClientConnect::SetRemoteConnectIdAddr(uint8_t conMode,int id,void *addr)
{
    if(m_connectedFlag)
    {
        uint8_t txBuff[2048];
        int  txLen = 0;

        DataPacket txPacket(txBuff,0);

        Debug_Printf("connected receive ReqCnCmd\n");
        txLen = txPacket.GenReqConnectPacket1(m_ownerId,m_remoteConnectId,m_id,m_remoteId,m_connectMode,(uint8_t*)&m_ownerId,sizeof(m_ownerId));
        SendData(txBuff,txLen);
        return;
    }
    if(m_isServer)
    {
        m_connectMode = conMode;
    }
    else if(m_connectMode != conMode)
    {
        return;
    }
    m_remoteId = id;
    if(conMode != 2)
    {
        memcpy(&m_connectAddr,addr,sizeof(m_connectAddr));
    }        
    else
    {
        memcpy(&m_remoteConnectId,addr,sizeof(m_remoteConnectId));
        m_connectAddr = m_serverAddr;
        Debug_Printf("%s m_remoteConnectId = %d\n",m_pClientDevice->GetName().c_str(),m_remoteConnectId);
    }
        
    //m_connectAddr = *addr;
    m_getRemoteIdFlag = 1;
    //SendData((uint8_t*)"Hello",6);
}

void ClientConnect::SetConnectedCallBack(ConnectedCallBack p)
{
    m_connectedCallBack = p;
}

ClientDevice *ClientConnect::GetClientDevice(void)
{
    return m_pClientDevice;
}

uint32_t ClientConnect::GetLocalId(void)
{
    return m_ownerId;
}

uint32_t ClientConnect::GetRemoteId(void)
{
    return m_remoteConnectId;
}

ConnectChannel *ClientConnect::CreateConnectChannel(int isTcp)
{
    if(m_channelNum >= MAX_CHANNEL)
        return NULL;
    ConnectChannel *pChannel = new ConnectChannel(this,m_channelNum,isTcp);
    m_connectChannel[m_channelNum] = pChannel;
    m_channelNum ++;
    return pChannel;
}

int ClientConnect::GetChannelNum(void)
{
    return m_channelNum;
}

ConnectChannel *ClientConnect::GetChannel(int id)
{
    return m_connectChannel[id];
}

void ClientConnect::ResetConnect(void)
{
    for(int i = 0 ; i < m_channelNum ; i ++)
    {
        if(m_connectChannel[i] !=  NULL)
        {
            delete m_connectChannel[i];
            m_connectChannel[i] = NULL;
        }
    }
    m_channelNum = 0;
    m_connectedFlag = 0;
    m_connectMode = 0;
}

void *ClientConnect::P2PconnectThread(void*arg)
{
    ClientConnect *p_this = (ClientConnect *)arg;

    p_this->GetOwnerAddr();

    while(p_this->m_runFlag)
    {
        if(p_this->m_remoteClientId == 0)
        {
            p_this->GetRemoteClientId();
        }
        if(p_this->m_connectedFlag == 0)
        {
            p_this->TryConnectRomoteClient();
        }
        else
        {
            //int retLen =  ReceiveData(retbuff,128);
            p_this->RunAfterConnected();
        }
        usleep(100);
    }
    return NULL;
}

void ClientConnect::GetOwnerAddr(void)
{
    socklen_t len = sizeof(struct sockaddr_in);
    uint8_t rxBuff[2048];
    uint8_t txBuff[2048];
    int  rxLen = 0;
    int  txLen = 0;
    DataPacket rxPacket(rxBuff,0);
    DataPacket txPacket(txBuff,0);

    txLen = txPacket.GenGetIPPacket();

    while(m_runFlag)
    {
        SendData(txBuff,txLen,&m_serverAddr);
        for(int i = 0 ; i < 100 ; i ++)
        {
            rxLen = ReceiveData(rxBuff,2048);
            if(rxLen > 0)
            {
                //Debug_ShowHex(rxBuff,rxLen);
                //Debug_Printf("error code = %d\n",rxPacket.GetErrorCode() );
                rxPacket.UpdateLength(rxLen);
                if(rxPacket.GetPacketCmd() == ReturnCmd && rxPacket.GetPacketRetCmd() == ReqIpCmd && rxPacket.GetErrorCode() == NoError)
                {
                    rxPacket.PopData((uint8_t*)&m_interAddr,sizeof(sockaddr_in));
                    Debug_Printf("%s connect %d inter addr",m_pClientDevice->GetName().c_str(),m_id);
                    Debug_ShowIp(m_interAddr.sin_addr.s_addr,m_interAddr.sin_port);
                    Debug_Printf("\n");
                    goto CNTE;
                }
            }
            usleep(10000);
        }
    }

CNTE:
    getsockname(m_connectSocket,(struct sockaddr*)&m_localAddr, &len);
    while(m_runFlag && m_pClientDevice->GetRegFlag() == 0);
    m_localAddr.sin_addr.s_addr = m_pClientDevice->GetLocalSocketAddr().sin_addr.s_addr;
    Debug_Printf("%s connect %d local addr",m_pClientDevice->GetName().c_str(),m_id);
    Debug_ShowIp(m_localAddr.sin_addr.s_addr,m_localAddr.sin_port);
    Debug_Printf("\n");
    //getsockname(m_connectSocket,(struct sockaddr*)&m_localAddr, &len);
}

void ClientConnect::GetRemoteClientId(void)
{
    uint8_t txBuff[2048];
    int  txLen = 0;
    DataPacket txPacket(txBuff,0);

    while(m_runFlag && m_pClientDevice->GetRegFlag() == 0);
    txLen = txPacket.GenReqClientIdPacket(m_pClientDevice->GetClientId(),m_id,m_remoteClientName.c_str());

    while(m_runFlag)
    {
        m_pClientDevice->SendData(txBuff,txLen);
        for(int i = 0 ; i < 100 ; i ++)
        {
            if(m_remoteClientId)
            {
                Debug_Printf("m_remoteClientId = %d\n",m_remoteClientId);
                return ;
            }
            usleep(10000);
        }
    }
}

void ClientConnect::TryConnectRomoteClient(void)
{
    uint8_t txBuff[2048];
    uint8_t rBuff[2048];
    uint8_t tBuff[2048];
    int  txLen = 0;
    int  rLen = 0 ;
    int tLen = 0;
    sockaddr_in rec_addr;

    DataPacket txPacket(txBuff,0);
    DataPacket rPacket(rBuff,0);
    DataPacket tPacket(tBuff,0);

    Debug_Printf("TryConnectRomoteClient...\n");
    
    while(m_runFlag)
    {
        if(m_connectMode == 0)
        {
            //m_serverAddr.sin_port = 1234;
            txLen = txPacket.GenReqConnectPacket(m_pClientDevice->GetClientId(),m_remoteClientId,m_id,m_remoteId,m_connectMode,(uint8_t*)&m_interAddr,sizeof(struct sockaddr_in));
        }
        else if(m_connectMode == 1)
        {
            txLen = txPacket.GenReqConnectPacket(m_pClientDevice->GetClientId(),m_remoteClientId,m_id,m_remoteId,m_connectMode,(uint8_t*)&m_localAddr,sizeof(struct sockaddr_in));
        }
        else 
        {
            m_connectAddr = m_serverAddr;
            CreatServerConnect();
            txLen = txPacket.GenReqConnectPacket(m_pClientDevice->GetClientId(),m_remoteClientId,m_id,m_remoteId,m_connectMode,(uint8_t*)&m_ownerId,sizeof(m_ownerId));
        }
        if(m_isServer == 0)
        {
            m_getRemoteIdFlag = 0;
            while(m_runFlag && m_getRemoteIdFlag == 0)
            {
                m_pClientDevice->SendData(txBuff,txLen);
                Debug_Printf("m_pClientDevice->SendData\n");
                for(int i = 0 ; i < 300 ; i ++)
                {
                    if(m_getRemoteIdFlag)
                    {
                        Debug_Printf("%s m_getRemoteIdFlag == 1+++++++++++++++++++++++%d\n",m_pClientDevice->GetName().c_str(),m_connectMode);
                        break;
                    } 
                    usleep(10000);
                }
            }
        }
        if(m_getRemoteIdFlag == 1)
            m_getRemoteIdFlag = 0;
        else 
            continue;
        //Debug_Printf("%s ********m_ownerConnectId = %d***************************\n",m_pClientDevice->GetName().c_str(),m_ownerId);
        tLen = tPacket.GenReqConnectPacket1(m_ownerId,m_remoteConnectId,m_id,m_remoteId,m_connectMode,(uint8_t*)&m_ownerId,sizeof(m_ownerId));
        //Debug_ShowHex(tBuff,tLen);
        
        while(1)
        {
            if(m_isServer == 1)
            {
                m_pClientDevice->SendData(txBuff,txLen);
            }

            if(m_getRemoteIdFlag)
                break;
            
            for(int i = 0 ; i < 10 ; i ++)
            {
                if(m_getRemoteIdFlag)
                    break;

                SendData(tBuff,tLen);
                rLen =  ReceiveData(rBuff,2048,&rec_addr);
                rPacket.UpdateLength(rLen);

                //Debug_ShowHex(rBuff,rLen);
                if(rLen > 0 && rPacket.GetPacketCmd() == ReqCnCmd)
                {
                    //Debug_ShowHex(retbuff,retLen);
                    Debug_Printf("%s connect %d connected mode %d!\n",m_pClientDevice->GetName().c_str(),m_id,m_connectMode);
                    Debug_ShowIp(rec_addr.sin_addr.s_addr,rec_addr.sin_port);
                    Debug_Printf("\n");
                    m_connectAddr = rec_addr;
                    m_connectedFlag = 1;
                    for(i = 0 ; i < 3 ; i ++)
                    {
                        //Debug_Printf("SendData(tBuff,tLen);%d!\n",i);
                        SendData(tBuff,tLen);
                        usleep(100000);
                    }
                    if(m_connectedCallBack)
                        m_connectedCallBack(this);
                    return;
                }
                usleep(100000);
            }
            if(m_isServer == 0)
                break;
        }

        if(m_isServer == 0)
        {
            m_connectMode ++;
            if(m_connectMode == 3)
                m_connectMode = 0;
        }
        
    }
}

void ClientConnect::CreatServerConnect(void)
{
    if(m_ownerId != 0)
        return;
    uint8_t rxBuff[2048];
    uint8_t txBuff[2048];
    int  rxLen = 0;
    int  txLen = 0;
    DataPacket rxPacket(rxBuff,0);
    DataPacket txPacket(txBuff,0);

    //Debug_Printf("%s,%d\n",__FUNCTION__,__LINE__);
    txLen = txPacket.GenRegCnPacket(m_pClientDevice->GetClientId());
    //Debug_ShowHex(txBuff,txLen);
    while(m_runFlag)
    {
        SendData(txBuff,txLen);
        for(int i = 0 ; i < 100 ; i ++)
        {
            rxLen = ReceiveData(rxBuff,2048);
            if(rxLen > 0)
            {
                //Debug_ShowHex(rxBuff,rxLen);
                //Debug_Printf("error code = %d\n",rxPacket.GetErrorCode() );
                rxPacket.UpdateLength(rxLen);
                if(rxPacket.GetPacketCmd() == ReturnCmd && rxPacket.GetPacketRetCmd() == RegCnCmd && rxPacket.GetErrorCode() == NoError)
                {
                    m_ownerId = rxPacket.GetDestinatId();
                    //m_clientId = rxPacket.GetDestinatId();
                    //m_regClientFlag = 1;
                    Debug_Printf("++++++++++++++++Register %s connect as %d\n",m_pClientDevice->GetName().c_str(),m_ownerId);
                    return ;
                }
            }
            usleep(10000);
        }
    }
}

void ClientConnect::RunAfterConnected(void)
{
    uint8_t rxBuff[4096];
    int  rxLen = 0;
    DataPacket rxPacket(rxBuff,0);

    rxLen = ReceiveData(rxBuff,4096);
    if(rxLen > 0)
    {
        rxPacket.UpdateLength(rxLen);
        PacketType ptype = rxPacket.GetPacketType();
        CmdType cmd = rxPacket.GetPacketCmd();
        
        if(ptype == CN_TO_CN)
        {
            if(cmd == RelDataCmd)
            {
                uint8_t channel = rxPacket.GetChannel();
                //Debug_Printf("receive remote packet %d,%d,%d,%d\n",ptype,cmd,channel,m_channelNum);
                if(channel < m_channelNum && m_connectChannel[channel] != NULL)
                    m_connectChannel[channel]->ReplayRemotePacket(rxBuff,rxLen);
            }
            else if(cmd == ReqResendCmd)
            {
                uint8_t buff[256];
                uint8_t channel = rxPacket.GetChannel();
                int size = rxPacket.GetResendNum(buff);
                if(channel < m_channelNum && m_connectChannel[channel] != NULL)
                    m_connectChannel[channel]->ResendLocalPacket(buff,size);
            }
            else if(cmd == ReqCnCmd)
            {
                /*uint8_t txBuff[2048];
                int  txLen = 0;

                DataPacket txPacket(txBuff,0);*/

                //Debug_Printf("connected receive ReqCnCmd\n");
                //txLen = txPacket.GenReqConnectPacket1(m_ownerId,m_remoteConnectId,m_id,m_remoteId,m_connectMode,(uint8_t*)&m_ownerId,sizeof(m_ownerId));
                //SendData(txBuff,txLen);
            }
        }
    }

    for(int i = 0 ; i < m_channelNum ; i ++)
    {
        if(m_connectChannel[i] != NULL)
            m_connectChannel[i]->SendLocalPacket();
    }
}
