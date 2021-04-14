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
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <poll.h>
#define _GNU_SOURCE 1
#define BUFFER_SIZE 64

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));   //basename(argv[0]：文件名  
        return 1;
    }
    
    const char* ip = argv[1];
    int port  = atoi(argv[2]);
    
    
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    if (connect(sock, (struct sockaddr* )&address, sizeof(address) ) < 0)
    {
        printf("connection failed\n");
        close(sock);
        return 1;
    }
    
    pollfd fds[2]; //fd[0]:文件描述符0（标准输入） fd[1]sock上的可读事件
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sock;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret != -1);

    while (1)
    {
        ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            printf("poll failure \n");
            break;
        }
        if (fds[1].revents & POLLRDHUP)
        {
            printf("server close connection\n");
            break;
        }
        else if (fds[1].revents & POLLIN)
        {
            memset(read_buf, '\0', BUFFER_SIZE);
            recv(fds[1].fd, read_buf, BUFFER_SIZE-1, 0);
            printf("%s\n", read_buf);
        }
        if (fds[0].revents & POLLIN)
        {
            ret = splice(0, NULL,  pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], NULL,  sock, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }    
    }
    close(sock);
    return 0;
}

