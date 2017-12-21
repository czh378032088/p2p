#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include "clientnode.h"

ClientNode::ClientNode(string &name,uint32_t id)
{
    m_nodeName = name;
    m_clientId = id;
}

int ClientNode::Senddata(uint8_t buff[],int size)
{
    return sendto(this->m_socket,buff,size,MSG_DONTWAIT,(struct sockaddr *) &m_int_addr,sizeof(sockaddr_in));
}

void ClientNode::SetIntAddr(sockaddr_in &addr)
{
    m_int_addr = addr;
}

void ClientNode::SetSocketHandle(int handle)
{
    m_socket = handle;
}
    
string ClientNode::GetClientName(void)
{
    return m_nodeName;
}

uint32_t ClientNode::GetClientId(void)
{
    return m_clientId;
}

ConnectNode* ClientNode::AddConnectNode(ConnectNode*pNode)
{
    m_connectNodeList.push_back(pNode);    
    return pNode;
}

ConnectNode* ClientNode::DeleteConnectNode(ConnectNode*pNode)
{
    m_connectNodeList.remove(pNode);
    return NULL;
}
