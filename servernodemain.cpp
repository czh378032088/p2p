#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "debug.h"
#include "commonfuc.h"
#include "p2pserver.h"
#include "clientdevice.h"
#include "debug.h"




char savePort[32];
bool setupFlag1 = false,setupFlag2 = false;
uint32_t lastTime = 0;
uint32_t lastLen = 0;

int ServerLtoMCallBack(ConnectChannel *pChannel,uint8_t data[],int len)
{
    int newLen = len;
    data[len] = 0;
    if(setupFlag2)
    {
        int port1,port2;
        CommonFuc::GetPortNum((char *)data,"server_port=",savePort,len);
        sscanf((char*)savePort,"%d-%d",&port1,&port2);
        Debug_Printf("port:%d,%d\n",port1,port2);
        ConnectChannel *pChannel1 = pChannel->GetClientConnect()->CreateConnectChannel(0);
        pChannel1->BindPort(8014);
        pChannel1->ConnectAddr("127.0.0.1",port1);
        ConnectChannel *pChannel2 = pChannel->GetClientConnect()->CreateConnectChannel(0);
        pChannel2->BindPort(8015);
        pChannel2->ConnectAddr("127.0.0.1",port2);
        setupFlag2 = false;
    }
    data[newLen] = 0;
    return newLen;
}

int ServerMtoLCallBack(ConnectChannel *pChannel,uint8_t data[],int len)
{
    int newLen = len;
    data[len] = 0;
    newLen = CommonFuc::ReplaceString((char *)data,"rtsp://","/","127.0.0.1:554",len);

    if(CommonFuc::IsRequestType((char *)data,"SETUP"))
    {
        //int port1,port2;
        newLen = CommonFuc::ReplacePortNum((char *)data,"client_port=","8014-8015",newLen);
        setupFlag2 = true;
    }

    data[newLen] = 0;
    Debug_Printf("%s\n",data);
    return newLen;
}

int ServerConnectCallBack(ClientConnect*pclent)
{
    Debug_Printf("ServerConnectCallBack %s\n",pclent->GetClientDevice()->GetName().c_str());
    ConnectChannel *pChannel = pclent->CreateConnectChannel(1);
    pChannel->ConnectAddr("127.0.0.1",554);
    pChannel->SetCallBack(ServerLtoMCallBack,ServerMtoLCallBack);
    return 0;
}

int main(int argc, char* argv[])
{
    ClientDevice client2("GM8136cammera");

    client2.StartDevice("120.77.147.40",8010);
    client2.SetConnectedCallBack((void*)&ServerConnectCallBack);
    //client1.CreatClientConnect(8010,"clienttest2");


    while(1)
    {
        char ret = getchar();
        if(ret == 'q' || ret == 'Q')
        {
            client2.StopDevice();
            exit(0);
        }

        usleep(10000);
    }

    return 0;
}
