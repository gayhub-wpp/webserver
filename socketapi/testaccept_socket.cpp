#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    if(argc <= 2){
        printf("usage: %s ip_address port_number\n", basename(argv[0]));   //basename(argv[0]：文件名  
    }

    //取参数
    const char* ip = argv[1];    
    int port = atoi(argv[2]);

    //创建socket （监听socket/服务器socket）
    struct sockaddr_in address;
    bzero(&address , sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock>=0);
    //绑定
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    ret = listen(sock, 3);
    assert(ret != -1);
    //暂停20秒以等待客户端连接和相关操作
    sleep(20);
    //客户端socket
    struct sockaddr_in cliet;
    //服务器socket接受客户端socket连接
    socklen_t cliet_addrlen = sizeof(cliet);
    int connfd = accept(sock, (struct sockaddr*)&cliet, &cliet_addrlen);
    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
    }
    else{
        char remote[INET_ADDRSTRLEN];
        printf("Connected with ip: %s and port: %d\n", inet_ntop(AF_INET, &cliet.sin_addr, remote, INET_ADDRSTRLEN), ntohs(cliet.sin_port));
        close(connfd);
    }
    close(sock);
    return 0;
}



