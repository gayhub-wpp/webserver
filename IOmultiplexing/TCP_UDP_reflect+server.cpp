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
#include <sys/epoll.h>
#define MAX_EVENTNUM 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFERR_SIZE 1024

int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));   //basename(argv[0]：文件名  
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    
    //tcp
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >=0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert( ret != -1);

    ret = listen(listenfd, 5);
    assert( ret != -1);

    //udp
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(udpfd >=0);

    ret = bind(udpfd, (struct sockaddr*)&address, sizeof(address));
    assert( ret != -1);

    //往epoll中注册tcp、udp socket上的可读事件
    epoll_event events[MAX_EVENTNUM];
    int epollfd = epoll_create(5);
    assert(epollfd >= 0);
    addfd(epollfd, listenfd);
    addfd(epollfd, udpfd);

    while (1)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENTNUM, -1);
        if (number < 0)
        {
            printf("epoll failure \n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                int connfd = accept(listenfd, (sockaddr*) & client_address, &len);
                addfd(epollfd, connfd);
            }
            else if (sockfd == udpfd)
            {
                char buf[UDP_BUFERR_SIZE];
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                ret = recvfrom(listenfd, buf, UDP_BUFERR_SIZE-1, 0, (sockaddr*) & client_address, &len);
                if (ret > 0)
                {
                    sendto(listenfd, buf, UDP_BUFERR_SIZE-1, 0, (sockaddr*) & client_address, len);
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                char buf[TCP_BUFFER_SIZE];
                while (1)
                {
                    memset(buf, '\0', TCP_BUFFER_SIZE);
                    ret = recv(sockfd, buf, TCP_BUFFER_SIZE-1, 0);
                    if (ret < 0)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            break;
                        }
                        close(sockfd);
                        break;
                    }
                    else if (ret == 0)
                    {
                        close(sockfd);
                    }
                    else {
                        send(sockfd, buf, TCP_BUFFER_SIZE-1, 0);
                    }
                                        
                }
                
            }
            else{
                printf("something else happened\n");
            }
        }
    }
    close(listenfd);
    close(udpfd);
    
    return 0;
}
