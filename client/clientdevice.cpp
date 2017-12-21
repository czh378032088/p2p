#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "clientdevice.h"
#include "debug.h"
#include "datapacket.h"
#include "commonfuc.h"

ClientDevice::ClientDevice(string clientName)
{
    m_clientName = clientName;
    for(int i = 0 ; i < MAX_CONNECT ; i ++)
    {
        m_clientConnect[i] = NULL;
        m_connectedFlag[i] = 0;
    }
        
    m_connectNum = 0;
    m_runFlag = 0;
    m_regClientFlag = 0;
    m_clientId = 0;
    m_connectedCallBack = NULL;
    pthread_mutex_init(&m_sendlock, NULL);
    //m_localIp = 0;
}

int ClientDevice::StartDevice(const char ip[],uint16_t port)
{
    if(m_runFlag)
        return -1;
    m_serverSocket = socket(AF_INET, SOCK_DGRAM, 0);  
    if(m_serverSocket < 0)
    {
        Debug_Printf("%s creat tcp socket failed\n",__FUNCTION__);
        return -1;
    }  
    
    m_serverAddr.sin_family = AF_INET;  
    m_serverAddr.sin_port   = htons(port);  
    m_serverAddr.sin_addr.s_addr = inet_addr(ip);
    bzero(m_serverAddr.sin_zero,8);  

    m_runFlag = 1;
    int ret = pthread_create(&m_threadID,NULL,P2PclientThread,this); 
    return ret;
}

int ClientDevice::StopDevice()
{
    for(int i = 0 ; i < m_connectNum ; i ++)
    {
        m_clientConnect[i]->StopConnect();
        delete m_clientConnect[i];
        m_clientConnect[i] = 0;
    }
    m_connectNum = 0;
    this->m_runFlag = 0;
    return 0;
}

int ClientDevice::SendData(const uint8_t buff[],int len)
{
    //Debug_Printf("ClientDevice::SendData\n");
    pthread_mutex_lock(&m_sendlock); 
    int ret = sendto(m_serverSocket,buff,len,MSG_DONTWAIT,(struct sockaddr *) &m_serverAddr,sizeof (m_serverAddr));
    m_lastSendTime = CommonFuc::GetTimeS();
    pthread_mutex_unlock(&m_sendlock); 
    //Debug_ShowIp(m_serverAddr.sin_addr.s_addr,m_serverAddr.sin_port);
    //Debug_Printf("ClientDevice::SendData end\n");
    return ret;
}

int ClientDevice::ReceiveData(uint8_t buff[],int len,sockaddr_in *addr)
{
    if(addr == NULL)
        return recv(m_serverSocket,buff,len,MSG_DONTWAIT);
    else 
    {
        socklen_t len = sizeof(struct sockaddr_in);
        return recvfrom(m_serverSocket,buff,len,MSG_DONTWAIT,(struct sockaddr *) addr,&len);
    }
        
}

ClientConnect *ClientDevice::CreatClientConnect(uint16_t port,string remoteClientName,int isTcp)
{
    ClientConnect *p;
    if(m_connectNum >= MAX_CONNECT)
        return NULL;
    p = new ClientConnect(this,m_connectNum);
    if( p == NULL)
        return NULL;
    m_clientConnect[m_connectNum] = p;
    m_connectNum ++;
    if(m_connectedCallBack) 
        p->SetConnectedCallBack((ConnectedCallBack)m_connectedCallBack);
    p->StartConnect(port,remoteClientName,isTcp);

    return p;
}

sockaddr_in ClientDevice::GetServerSocketAddr(void)
{
    return m_serverAddr;
}

sockaddr_in ClientDevice::GetLocalSocketAddr(void)
{
    return m_localAddr;
}

string ClientDevice::GetName(void)
{
    return m_clientName;
}

int ClientDevice::GetRegFlag(void)
{
    return m_regClientFlag;
}

uint32_t ClientDevice::GetClientId(void)
{
    return m_clientId;
}

void ClientDevice::SetConnectedCallBack(void* p)
{
    m_connectedCallBack = p;
}

int ClientDevice::GetClientConnectNum(void)
{
    return m_connectNum;
}

ClientConnect *ClientDevice::GetClientConnect(int id)
{
    return m_clientConnect[id];
}

void *ClientDevice::P2PclientThread(void*arg)
{
    ClientDevice*p_this = (ClientDevice*)arg;
    //uint8_t rxBuff[2048];
    uint8_t txBuff[2048];
    //int  rxLen = 0;
    int  txLen = 0;
    //DataPacket rxPacket(rxBuff,0);
    DataPacket txPacket(txBuff,0);

    p_this->GetLocalAddr();

    while(p_this->m_runFlag)
    {
        if(!p_this->m_regClientFlag)
        {
            p_this->RegisterDevice();
            continue;
        }
        
        if(CommonFuc::GetTimeS() - p_this->m_lastSendTime > 20)
        {
            txLen = txPacket.GenNonePacket(p_this->m_clientId,0);
            p_this->SendData(txBuff,txLen);
            Debug_Printf("SendData None\n");
        }

        p_this->HandleReceiveData();
        usleep(10000);
    }
    return NULL;
}

void ClientDevice::GetLocalAddr(void)
{
    int opt = 1;
    char cmbuf[100];
    char buffer[256];
    struct iovec iov[1];
    int n = 0;
    socklen_t len = sizeof(struct sockaddr_in);
    
    //Debug_Printf("%s\n",__FUNCTION__);
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof ( buffer );

    struct msghdr mh ;

    mh.msg_name = &m_serverAddr;
    mh.msg_namelen = sizeof ( m_serverAddr );
    mh.msg_control = cmbuf;
    mh.msg_controllen = sizeof (cmbuf );
    mh.msg_iov=iov;                                                           
    mh.msg_iovlen=1;
    setsockopt(m_serverSocket,IPPROTO_IP, IP_PKTINFO, &opt, sizeof ( opt ) );
    while(m_runFlag)
    {
        SendData((const uint8_t*)"getaddr",7);
        Debug_Printf("%s,%d\n",__FUNCTION__,__LINE__);
        getsockname(m_serverSocket,(struct sockaddr*)&m_localAddr, &len);
        for(int i = 0 ; i < 100 ; i ++)
        {
            n=recvmsg (m_serverSocket, &mh, MSG_DONTWAIT );
            if(n >= 0)
                break;
            usleep(10000);
        }
        if(n < 0)
            continue;
        cmbuf[n]=0;
        struct cmsghdr *cmsg ;
        for ( cmsg = CMSG_FIRSTHDR ( &mh );cmsg != NULL;cmsg = CMSG_NXTHDR ( &mh, cmsg ) )
        {
            if ( cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_PKTINFO )
            {
                continue;
            }
            struct in_pktinfo *pi = (struct in_pktinfo *)CMSG_DATA ( cmsg );    

            if((pi->ipi_addr.s_addr & 0xff) != 0)
                m_localAddr.sin_addr.s_addr = pi->ipi_addr.s_addr;
            else if((pi->ipi_spec_dst.s_addr & 0xff) != 0)
                m_localAddr.sin_addr.s_addr = pi->ipi_spec_dst.s_addr;

            Debug_Printf("本地ip地址");
            Debug_ShowIp(m_localAddr.sin_addr.s_addr,m_localAddr.sin_port);
            Debug_Printf("\n");

            goto Finished;
        }   
    }
Finished:
    opt = 0;
    setsockopt(m_serverSocket,IPPROTO_IP, IP_PKTINFO, &opt, sizeof ( opt ) );
}

int  ClientDevice::RegisterDevice(void)
{
    uint8_t rxBuff[2048];
    uint8_t txBuff[2048];
    int  rxLen = 0;
    int  txLen = 0;
    DataPacket rxPacket(rxBuff,0);
    DataPacket txPacket(txBuff,0);

    //Debug_Printf("%s,%d\n",__FUNCTION__,__LINE__);
    txLen = txPacket.GenRegClPacket(m_clientName.c_str());
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
                if(rxPacket.GetPacketCmd() == ReturnCmd && rxPacket.GetPacketRetCmd() == RegClCmd && rxPacket.GetErrorCode() == NoError)
                {
                    m_clientId = rxPacket.GetDestinatId();
                    m_regClientFlag = 1;
                    Debug_Printf("Register %s as %d\n",m_clientName.c_str(),m_clientId);
                    return 0;
                }
            }
            usleep(10000);
        }
    }
    return 0;
}

void ClientDevice::HandleReceiveData(void)
{
    uint8_t rxBuff[2048];
    //uint8_t txBuff[2048];
    int  rxLen = 0;
    //int  txLen = 0;
    DataPacket rxPacket(rxBuff,0);
    //DataPacket txPacket(txBuff,0);

    rxLen = ReceiveData(rxBuff,2048);

    if(rxLen > 0 )
    {
        //Debug_ShowHex(rxBuff,rxLen);
        //Debug_Printf("error code = %d\n",rxPacket.GetErrorCode());
        rxPacket.UpdateLength(rxLen);
        PacketType ptype = rxPacket.GetPacketType();
        CmdType cmd = rxPacket.GetPacketCmd();
        if(ptype == CL_TO_CL)
        {
            if(cmd == ReqCnCmd)
            {
                int srcconnectid = rxPacket.GetSrcConnectId();
                int disconnectid = rxPacket.GetDisConnectId();
                uint32_t srcClientId = rxPacket.GetSourceId();
                uint8_t conMod;
                sockaddr_in addr;
                uint32_t srcConnectId ;

                rxPacket.PopData(&conMod,1);
                if(conMod != 2)
                    rxPacket.PopData((uint8_t*)&addr,sizeof(sockaddr_in));
                else 
                    rxPacket.PopData((uint8_t*)&srcConnectId,sizeof(srcConnectId));
                if(disconnectid == 0x0f)
                {
                    uint32_t flag = srcClientId ^ (srcconnectid << 28);
                    for(int i = 0 ; i < m_connectNum ; i ++)
                    {
                        if(m_connectedFlag[i] == flag)
                        {
                            disconnectid = i;
                            m_clientConnect[disconnectid]->ResetConnect();
                        }
                        break;
                    }
                }
                if(disconnectid > m_connectNum || m_clientConnect[disconnectid] == NULL)
                {
                    Debug_Printf("m_connectNum = %d\n",m_connectNum);
                    string temp;
             
                    if(m_connectNum == MAX_CONNECT)
                        return;
                    m_clientConnect[m_connectNum] = new ClientConnect(this,m_connectNum,1);
                    m_clientConnect[m_connectNum]->SetRemoteClientId(srcClientId);
                    if(conMod != 2)
                        m_clientConnect[m_connectNum]->SetRemoteConnectIdAddr(conMod,srcconnectid,&addr);
                    else
                        m_clientConnect[m_connectNum]->SetRemoteConnectIdAddr(conMod,srcconnectid,&srcConnectId);
                    if(m_connectedCallBack) 
                        m_clientConnect[m_connectNum]->SetConnectedCallBack((ConnectedCallBack)m_connectedCallBack);
                    m_connectedFlag[m_connectNum] = srcClientId ^ (m_connectNum << 28);
                    m_clientConnect[m_connectNum]->StartConnect(htons(m_serverAddr.sin_port),temp,0);
                    disconnectid = m_connectNum;
                    m_connectNum ++;
                }
                else
                {
                    if(conMod != 2)
                        m_clientConnect[disconnectid]->SetRemoteConnectIdAddr(conMod,srcconnectid,&addr);
                    else
                        m_clientConnect[disconnectid]->SetRemoteConnectIdAddr(conMod,srcconnectid,&srcConnectId);
                }
            }
            else if(cmd == ReturnCmd)
            {

            }
        }
        else if(ptype == SV_TO_CL)
        {
            if(cmd == ReturnCmd)
            {
                CmdType retCmd = rxPacket.GetPacketRetCmd();
                int   errorCode = rxPacket.GetErrorCode();
                if(retCmd == ReqClIdCmd && errorCode == NoError)
                {
                    int connectid = rxPacket.GetDisConnectId();
                    if(connectid < m_connectNum && m_clientConnect[connectid] != NULL)
                    {
                        m_clientConnect[connectid]->SetRemoteClientId(rxPacket.GetSourceId());
                    }
                }
            }
        }
        else if(ptype == SV_TO_NO)
        {

        }
    }
}
