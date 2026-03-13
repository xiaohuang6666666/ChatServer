#include "chatserver.hpp"
#include <signal.h>
#include <iostream>
using namespace std;


void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}
int main(int argc, char **argv)
{
    // 处理 ctrl+c 结束后,重置user的状态信息
    signal(SIGINT,resetHandler);
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"ChatServer");
    server.start();
    loop.loop();
    return 0;
}