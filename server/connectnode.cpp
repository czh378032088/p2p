#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include "connectnode.h"
#include "debug.h"

ConnectNode::ConnectNode(uint32_t id,int isTcp)
{
    m_clientId = id;
    m_isTcp = isTcp;
}

int ConnectNode::Senddata(uint8_t buff[],int size)
{
    //Debug_ShowIp(m_int_addr.sin_addr.s_addr,m_int_addr.sin_port);
    //Debug_Printf("ConnectNode::Senddata%d\n",m_socket);
   
    if(m_isTcp)
        return send(m_socket,buff,size,MSG_DONTWAIT);
    else
        return sendto(this->m_socket,buff,size,MSG_DONTWAIT,(struct sockaddr *) &m_int_addr,sizeof(sockaddr_in));
    //Debug_Printf("ConnectNode::Senddata end\n");
}

void ConnectNode::SetIntAddr(sockaddr_in &addr)
{
    m_int_addr = addr;
}

void ConnectNode::SetSocketHandle(int handle)
{
    m_socket = handle;
}

ClientNode *ConnectNode::SetClientNode(ClientNode *pNode)
{
    m_pClientNode = pNode;
    return m_pClientNode;
}

uint32_t ConnectNode::GetConnectId(void)
{
    return m_clientId;
}

ClientNode *ConnectNode::SetClientNode(void)
{
    return m_pClientNode;
}
