#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include "debug.h"
#include "commonfuc.h"
#include "p2pserver.h"
#include "clientdevice.h"
#include "debug.h"

char savePort[32];
bool setupFlag1 = false,setupFlag2 = false;
uint32_t lastTime = 0;
uint32_t lastLen = 0;
int ClientDataMtoLCallBack(ConnectChannel *pChannel,uint8_t data[],int len)
{
     //Debug_Printf("ClientDataLtoMCallBack%d\n",len);
    uint32_t nowTime = CommonFuc::GetTimeMs();
    if(nowTime - lastTime >= 1000)
    {
        float disTime = nowTime - lastTime;
        uint32_t nowLen = pChannel->GetTotalRemoteLen();
        Debug_Printf("ClientDataLtoMCallBack:%.2fkbps\n",(nowLen - lastLen) / disTime);
        lastLen = nowLen;
        lastTime = nowTime;
    }
    return len;
}

int ClientLtoMCallBack(ConnectChannel *pChannel,uint8_t data[],int len)
{
    int newLen = len;
    data[len] = 0;
    if(CommonFuc::IsRequestType((char *)data,"SETUP"))
    {
        int port1,port2;
        CommonFuc::GetPortNum((char *)data,"client_port=",savePort,len);
        sscanf((char*)savePort,"%d-%d",&port1,&port2);
        Debug_Printf("port:%d,%d\n",port1,port2);
        ConnectChannel *pChannel1 = pChannel->GetClientConnect()->CreateConnectChannel(0);
        pChannel1->BindPort(8012);
        pChannel1->ConnectAddr("127.0.0.1",port1);
        pChannel1->SetCallBack(NULL,ClientDataMtoLCallBack);
        ConnectChannel *pChannel2 = pChannel->GetClientConnect()->CreateConnectChannel(0);
        pChannel2->BindPort(8013);
        pChannel2->ConnectAddr("127.0.0.1",port2);
        setupFlag1 = true;
    }
    data[newLen] = 0;
    Debug_Printf("%s\n",data);
    return newLen;
}

int ClientMtoLCallBack(ConnectChannel *pChannel,uint8_t data[],int len)
{
    int newLen = len;
    data[len] = 0;
    newLen = CommonFuc::ReplaceString((char *)data,"rtsp://","/","127.0.0.1:8011",newLen);
    if(setupFlag1)
    {
        newLen = CommonFuc::ReplacePortNum((char *)data,"server_port=","8012-8013",newLen);
        newLen = CommonFuc::ReplacePortNum((char *)data,"client_port=",savePort,newLen);
        setupFlag1 = false;
    }
    data[newLen] = 0;
    Debug_Printf("%s\n",data);
    return newLen;
}

int ClientConnectCallBack(ClientConnect*pclent)
{
    Debug_Printf("ClientConnectCallBack %s\n",pclent->GetClientDevice()->GetName().c_str());
    ConnectChannel *pChannel = pclent->CreateConnectChannel(1);
    pChannel->BindPort(8011);
    pChannel->SetCallBack(ClientLtoMCallBack,ClientMtoLCallBack);
    return 0;
}


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
    newLen = CommonFuc::ReplaceString((char *)data,"rtsp://","/","127.0.0.1:8554",len);

    if(CommonFuc::IsRequestType((char *)data,"SETUP"))
    {
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
    pChannel->ConnectAddr("127.0.0.1",8554);
    pChannel->SetCallBack(ServerLtoMCallBack,ServerMtoLCallBack);
    return 0;
}

int main(int argc, char* argv[])
{
    /*char test[2048];
    char ret[128];
    strcpy(test,"sending response: RTSP/1.0 200 OK\nCSeq: 4\nDate: Tue, Dec 05 2017 02:46:41 GMT\nTransport: RTP/AVP;unicast;destination=127.0.0.1;source=127.0.0.1;client_port=45248-45249;server_port=6970-6971\nSession: B5D8AFC6;timeout=65");
    CommonFuc::GetPortNum(test,"client_port=",ret,2048);
    Debug_Printf("%s\n",ret);
    CommonFuc::GetPortNum(test,"server_port=",ret,2048);
    Debug_Printf("%s\n",ret);
    Debug_Printf("%s\n",test);
    CommonFuc::ReplacePortNum(test,"client_port=","1-2",2048);
    Debug_Printf("%s\n",test);
    CommonFuc::ReplacePortNum(test,"server_port=","123456-234567",2048);
    Debug_Printf("%s\n",test);*/
    
    P2pServerClass p2pServer;
    ClientDevice client1("clienttest1");
    ClientDevice client2("clienttest2");

    p2pServer.SetTcpPort(8010);
    p2pServer.SetUdpPort(8010);
    p2pServer.RunServer();

    client1.StartDevice("120.77.147.40",8010);
    client1.SetConnectedCallBack((void*)&ClientConnectCallBack);
    client2.StartDevice("120.77.147.40",8010);
    client2.SetConnectedCallBack((void*)&ServerConnectCallBack);

    client1.CreatClientConnect(8010,"clienttest2");
    //client1.CreatClientConnect(8010,"clienttest2");


    while(1)
    {
        char ret = getchar();
        if(ret == 'q' || ret == 'Q')
        {
            p2pServer.StopServer();
            client1.StopDevice();
            client2.StopDevice();
            exit(0);
        }

        usleep(10000);
    }
    return 0;
}
