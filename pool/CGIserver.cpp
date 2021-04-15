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

    //处理客户请求
    void process()
    {
        int idx = 0;   
        int ret = -1;
        //循环读取和分析客户数据
        while (true)
        {
            idx = m_read_idx;
            ret = recv(m_sockfd, m_buf+idx, BUFFER_SIZE-1-idx, 0);
            //读发生错误，关闭客户连接，但如果是暂时无数据可读，则退出循环
            if (ret < 0)
            {
                if (errno != EAGAIN)
                {
                    removefd(m_epollfd, m_sockfd);
                }
                break;
            }
            //客户关闭连接，则服务器也关闭连接
            else if (ret == 0)
            {
                removefd(m_epollfd, m_sockfd);
                break;
            }
            //分析读到的数据
            else
            {
                //m_read_idx指向读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
                m_read_idx += ret;
                printf("user content is: %s\n", m_buf);
                //查看当前缓冲区有没有遇到\r\n，有则break开始处理客户请求
                for ( ; idx < m_read_idx; idx++)
                {
                    if ((idx >= 1) && (m_buf[idx-1] == '\r') && (m_buf[idx] == '\n') )
                    {
                        break;
                    }
                }
                //如果没有遇到，则需要继续循环读取更多的数据
                if (idx ==m_read_idx)
                {
                    continue;
                }
                
                //开始处理客户请求
                m_buf[idx-1] = '\0';

                char* filename = m_buf;
                //判断客户要运行的CGI程序是否存在
                if (access(filename, F_OK) == -1)    //F_OK判断文件是否存在
                {
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                //创建子进程来执行CGI程序
                ret = fork();
                if (ret == -1)   
                {
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                //父进程只需关闭连接
                else if (ret > 0)
                {
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                //子进程将标准输出定向到m_sockfd，并执行CGI程序
                else{
                    close(STDOUT_FILENO);
                    dup(m_sockfd);
                    execl(m_buf, m_buf, 0);
                    exit(0);
                }
                
            }            
        }
    }
};
int cgi_conn::m_epollfd = -1;

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0])); //basename(argv[0]：文件名
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    processpool<cgi_conn>* pool = processpool<cgi_conn>::create(listenfd);
    if (pool)
    {
        pool->run();
        delete pool;
    }
    close(listenfd);  //main函数创建了listenfd，那么就由main关闭

    return 0;
}

