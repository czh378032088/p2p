#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include "debug.h"
#include "commonfuc.h"
#include "p2pserver.h"
#include "clientdevice.h"
#include "debug.h"

int main(int argc, char* argv[])
{
    P2pServerClass p2pServer;

    p2pServer.SetTcpPort(8010);
    p2pServer.SetUdpPort(8010);
    p2pServer.RunServer();

    while(1)
    {
        char ret = getchar();
        if(ret == 'q' || ret == 'Q')
        {
            p2pServer.StopServer();
            exit(0);
        }

        usleep(10000);
    }
    return 0;
}
