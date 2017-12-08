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
        Debug_Printf("ClientDataMtoLCallBack:%.2fkbps\n",(nowLen - lastLen) / disTime);
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
}

int main(int argc, char* argv[])
{
    ClientDevice client1("clienttest1");

    client1.StartDevice("120.77.147.40",8010);
    client1.SetConnectedCallBack((void*)&ClientConnectCallBack);
   

    ClientConnect *pConnect = client1.CreatClientConnect(8010,"clienttest2");
    //client1.CreatClientConnect(8010,"clienttest2");


    while(1)
    {
        char ret = getchar();
        if(ret == 'q' || ret == 'Q')
        {
            client1.StopDevice();
            exit(0);
        }

        usleep(10000);
    }
    return 0;
}
