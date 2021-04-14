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

//用于处理客户CGI请求的类，作为processpool类的模板参数
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
    cgi_conn() {}
    ~cgi_conn() {}
    //初始化客户连接，清空读缓冲区
    void init(int epollfd, int sockfd, const sockaddr_in & client_addr)
    {
        m_epollfd = epollfd;
        m_sockfd = sockfd;
        m_address = client_addr;
        memset(m_buf, '\0', BUFFER_SIZE);
        m_read_idx = 0;
    }

    void
    
};
