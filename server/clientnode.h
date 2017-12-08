
#ifndef __CLIENTNODE__H__
#define __CLIENTNODE__H__

#include <netinet/in.h>
#include <string>
#include <list>
#include "connectnode.h"
#include "typedefine.h"
#include "datapacket.h"


using namespace std;

class ClientNode
{
public:
    ClientNode(string &name,uint32_t id);
    int Senddata(uint8_t buff[],int size);
    void SetIntAddr(sockaddr_in &addr);
    void SetSocketHandle(int handle);

    string GetClientName(void);
    uint32_t GetClientId(void);
    ConnectNode* AddConnectNode(ConnectNode*pNode);
    ConnectNode* DeleteConnectNode(ConnectNode*pNode);

protected:
private:
    string m_nodeName;
    sockaddr_in m_int_addr;
    list<ConnectNode*> m_connectNodeList;
    int m_socket;
    uint32_t m_clientId;
};

#endif
