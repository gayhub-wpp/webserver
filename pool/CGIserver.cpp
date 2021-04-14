#include <sys/types.h>
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
#include <sys/stat.h>
#include <sys/wait.h>

#include "processpool.h"

class cgi_conn
{
private:
    //读缓冲区大小
    static const int BUFFER_SIZE = 1024;
    static int m_epollfd;
    int m_sockfd;
    sockaddr_in m_address;
    char m_buf[BUFFER_SIZE];
    //标记读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
    int m_read_idx;
public:
    cgi_conn(/* args */) {}
    ~cgi_conn() {}
};