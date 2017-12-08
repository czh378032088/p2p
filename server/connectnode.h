

#ifndef __CONNECTNODE__H__
#define __CONNECTNODE__H__

#include <netinet/in.h>
#include "typedefine.h"
//#include "clientnode.h"

class ClientNode;

class ConnectNode
{
public:
    ConnectNode(uint32_t id,int isTcp);
    int Senddata(uint8_t buff[],int size);
    void SetIntAddr(sockaddr_in &addr);
    void SetSocketHandle(int handle);
    ClientNode *SetClientNode(ClientNode *pNode);

    uint32_t GetConnectId(void);
    ClientNode *SetClientNode(void);

protected:
private:
    sockaddr_in m_int_addr;
    int m_socket;
    int m_isTcp;
    ClientNode *m_pClientNode;
    uint32_t m_clientId;
};

#endif
