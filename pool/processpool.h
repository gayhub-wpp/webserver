#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>

class process
{
public:
    pid_t m_pid;
    int m_pipefd[2];
public:
    process() :m_pid(-1){}
};

template<typename T>
class processpool
{
private:
    //单例模式， 构造函数定义为私有, 只能通过下面的create静态函数来创建processpool
    processpool(int listenfd, int process_number = 8);
public:
    static processpool<T>* create(int listenfd, int process_number){
        if (!m_instance)
        {
            m_instance = new processpool<T>(listenfd, process_number);
        }
        return m_instance;
    }
    ~processpool() {
        delete [] m_sub_process;
    }
    void run();  //启动进程池
private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

private:
    static const int  MAX_PROCESS_NUMBER =16;   //进程池允许的最大子进程数量
    static const int  USER_PER_PROCESS = 65536; //每个子进程最多能处理的客户数量
    static const int  MAX_EVENT_NUMBER =10000;  //epoll最多能处理的事件数
    int m_process_number;        //进程池中的进程总数
    int m_idx;                   //子进程在池中的序号，从0开始
    int m_epollfd;               //epoll内核时间表
    int m_listenfd;              //监听socket
    int m_stop;                  //子进程通过m_stop来决定是否停止运行
    process* m_sub_process;      //保存所有子进程的描述信息
    static processpool<T>* m_instance;          //***进程池静态实例***
};

template<typename T>
processpool<T>* processpool<T>::m_instance = NULL;

static int sig_pipefd[2];    //处理信号的管道，以实现统一事件源，信号管道

static int setnonbolcking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

static void addfd(int epollfd, int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonbolcking(fd); 
}

static void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)msg, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void (handler)(int), bool restart = true){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//进程池构建函数，listenfd是监听socket，他必须在创建进程池之前被创建，否则子进程无法直接引用他，process_number 指定进程池中子进程的数量
template<typename T>
processpool<T>::processpool(int listenfd, int process_number)
    :m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false)    //
{
    assert((process_number >0) && (process_number <= MAX_PROCESS_NUMBER));
    m_sub_process = new process[process_number];
    assert(m_sub_process);

    //创建process_number个子进程，并建立它们和父进程的管道
    for (int i = 0; i < process_number; i++)
    {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);

        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);
        if (m_sub_process[i].m_pid > 0)  //父进程
        {
            close(m_sub_process[i].m_pipefd[1]);
            continue;
        }
        else{
            close(m_sub_process[i].m_pipefd[0]);
            m_idx = i;
            break;
        }
    }
}

template<typename T>
void processpool<T>::setup_sig_pipe()
{
    //创建epoll事件监听表和信号管道
    m_epollfd = epoll_create(5);
    assert(m_epollfd ! = -1);

    //使用socketpair创建管道，注册pipefd[0]上的可读事件
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);

    setnonbolcking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);

    //设置信号处理函数
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

//父进程m_idx = -1,子进程的idx大于等于0, 因此判断是父进程还是子进程
template<typename T>
void processpool<T>::run()
{
    if (m_idx != -1)
    {
        run_child();
        return;
    }
    run_parent();
}

template<typename T>
void processpool<T>::run_child()
{
    setup_sig_pipe();

    //子进程与父进程通信管道
    int pipefd = m_sub_process[m_idx].m_pipefd[1];
    //子进程通过pipefd通知子线程accept新连接
    addfd(m_epollfd, pipefd);
    
    epoll_event events[MAX_EVENT_NUMBER];
    T* users = new T[USER_PER_PROCESS];   //每个进程管理的用户
    assert(users);
    int number = 0;
    int ret = -1;

    while (!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            printf("1: epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            //与父进程的管道有数据到达，从管道读取数据，将结果保存到client中，读取成功则说明有新客户连接到来
            if ((sockfd == pipefd) && (events[i].events & EPOLLIN))
            {
                int client;
                ret = recv(sockfd, (char*)client, sizeof(client), 0);
                if ( ((ret<0) && (errno != EAGAIN)) || ret == 0)
                {
                    continue;
                }
                else{
                    struct sockaddr_in client_address;
                    socklen_t len = sizeof(client_address);
                    int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &len);
                    if (connfd < 0)
                    {
                        printf("errno is : %d\n", errno);
                        continue;
                    }
                    addfd(m_epollfd, connfd);
                    users[connfd].init(m_epollfd, connfd, client_address);  //?????????????????
                }
            }
            //有信号到达，接收信号
            else if (sockfd == sig_pipefd[0] && events[i].events & EPOLLIN)
            {
                int sig;
                char signal[1024];
                ret=recv(sig_pipefd[0], signal, sizeof(signal), 0);
                if (ret <= 0)
                {
                    continue;
                }
                else{
                    for (int i = 0; i < ret; i++)
                    {
                        
                    }
                    
                }
                
            }
                        
        }
        
        
    }
    



    epoll_wait(m_epollfd, __event)
}
template<typename T>
void processpool<T>::run_parent()
{
    
}

#endif