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
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));   //basename(argv[0]：文件名  
        return 1;
    }
    
    const char* ip = argv[1];
    int port  = atoi(argv[2]);
    
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr *)&client_address, &len);
    if (connfd < 0)
    {
        printf("errno is : %d\n", errno);
        close(listenfd);
    }

    char buffer[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while (1)
    {
        memset(buffer, '\0', sizeof(buffer));
        //每次调用select之前都要重新在read_file exception_file中设置文件描述符connfd
        FD_SET(connfd, &read_fds);
        FD_SET(connfd, &exception_fds);

        ret = select(connfd+1, &read_fds, NULL, &exception_fds, NULL);
        if (ret < 0)
        {
            printf("selection failure\n");
            break;
        }
        if (FD_ISSET(connfd, &read_fds))
        {
            ret = recv(connfd, buffer, sizeof(buffer)-1, 0);
            if (ret <= 0)
            {
                break;
            }
            printf("get %d byte of normal data:  %s\n", ret , buffer);
            // break;
        }
        else if (FD_ISSET(connfd, &exception_fds))
        {
            ret = recv(connfd, buffer, sizeof(buffer)-1, MSG_OOB);
            if (ret <= 0)
            {
                break;
            }
            printf("get %d byte of oob data:  %s\n", ret , buffer);
            // break;
        }
    }
    close(connfd);
    close(listenfd);    

    return 0;
}
