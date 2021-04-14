#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;

static void handle_term(int sig)
{
    stop = true;
}

int main(int argc, char const *argv[])
{
    signal(SIGTERM, handle_term);

    if (argc < 3)
    {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));   //basename(argv[0]：文件名  
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    
    //创建ipv4 socket地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));   //void bzero（void *s, int n）；将字符串s的前n个字节置为0，一般来说n通常取sizeof(s),将整块空间清零。
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    //绑定
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    //监听
    ret = listen(sock, backlog);
    assert(ret != -1);
    //循环等待连接，直到有SIGTERM信号将他中断
    while (!stop)
    {
        sleep(1);
    }
    close(sock);
    


    return 0;
}

