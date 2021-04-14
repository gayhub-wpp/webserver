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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 1024
#define FD_LIMIT 65535
#define MAX_EVENT_NUM 1024
#define PROCESS_LIMIT 65536

//处理一个客户连接的必要数据
struct client_data
{
    sockaddr_in address; //客户端socket
    int connfd;          //socket文件描述符
    pid_t pid;           //处理这个连接的子进程的PID
    int pipefd[2];       //和父进程通信的管道
};

static const char *shm_name = "/my_shm";
int sig_pipefd[2];
int epollfd;
int listenfd;
int shmfd;
char *share_mem = 0;
client_data *users = 0; //客户连接数组。进程用客户连接的编号来索引这个数组，即可取得相关的客户连接数据。
int *sub_process = 0;   //子连接和客户连接的映射关系表。用进程的PID来索引这个数组，即可取得该进程所处理的客户连接的编号。
int user_count = 0;     //当前客户数量
bool stop_child = false;

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
//向epoll表注册文件描述符fd(并设置fd为非阻塞模式)
void addfd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    //把信号通过sig_pipefd管道发送给主进程
    send(sig_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号的处理函数，触发sig之后的动作
void addsig(int sig, void (*handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;  //添加信号处理函数
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);    //信号处理函数执行期间屏蔽所有信号
    assert(sigaction(sig, &sa, NULL) != -1);   //设置sig的自定义信号处理函数
}

void del_resource()
{
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(listenfd);
    close(epollfd);
    shm_unlink(shm_name);
    delete[] users;
    delete[] sub_process;
}

//停止一个子进程
void child_term_handler(int sig)
{
    stop_child = true;
}

//子进程运行的函数。idx:该子进程处理的客户连接的编号，users:保存所有客户连接数据的数组，share_mem：共享内存的地址
int run_child(int idx, client_data *users, char *share_mem)
{
    epoll_event events[MAX_EVENT_NUM];
    int child_epollfd = epoll_create(5);
    assert(child_epollfd != -1);
    int connfd = users[idx].connfd;      //将socket文件描述符和管道文件描述符写端加入epoll事件表
    addfd(child_epollfd, connfd);
    int pipefd = users[idx].pipefd[1];
    addfd(child_epollfd, pipefd);
    int ret;
    //子进程设置自己的信号处理函数
    addsig(SIGTERM, child_term_handler, false);

    while (!stop_child)
    {
        int number = epoll_wait(child_epollfd, events, MAX_EVENT_NUM, -1);
        if (number < 0 && errno != EINTR)
        {
            printf("1: epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            //客户端有数据到达
            if ((sockfd == connfd) && (events[i].events & EPOLLIN))
            {
                memset(share_mem + idx * BUFFER_SIZE, '\0', BUFFER_SIZE);
                ret = recv(connfd, share_mem + idx * BUFFER_SIZE, BUFFER_SIZE - 1, 0);
                if (ret < 0)
                {
                    if (errno != EAGAIN)
                    {
                        stop_child = true;
                    }
                }
                else if (ret == 0)
                {
                    stop_child = true;
                }
                else
                {
                    //成功读取客户数据通知主进程来处理（通过管道）
                    send(pipefd, (char *)&idx, sizeof(idx), 0);
                }
            }
            //主进程通知本进程将第client个客户的数据发送到本进程负责的客户端
            else if ((sockfd == pipefd) && (events[i].events & EPOLLIN))
            {
                int client = 0;
                ret = recv(sockfd, (char *)&client, sizeof(client), 0);
                if (ret < 0)
                {
                    if (errno != EAGAIN)
                    {
                        stop_child = true;
                    }
                }
                else if (ret == 0)
                {
                    stop_child = true;
                }
                else
                {
                    send(connfd, share_mem + client * BUFFER_SIZE, BUFFER_SIZE, 0);
                }
            }
            else
            {
                continue;
            }
        }
    }
    close(connfd);
    close(pipefd);
    close(child_epollfd);
    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0])); //basename(argv[0]：文件名
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);


    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);

    //初始化用户数组、子进程数组
    user_count = 0;
    users = new client_data[USER_LIMIT + 1];
    sub_process = new int[PROCESS_LIMIT];
    for (int i = 0; i < PROCESS_LIMIT; i++)
    {
        sub_process[i] = -1;
    }

    epoll_event events[MAX_EVENT_NUM];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);


    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    addfd(epollfd, sig_pipefd[0]);

    addsig(SIGCHLD, sig_handler);   //进程终止或者停止
    addsig(SIGTERM, sig_handler);   //Termination request
    addsig(SIGINT, sig_handler);    //Interactive attention signal
    addsig(SIGPIPE, SIG_IGN);       //SIG_IGN忽略信号
    bool stop_server = false;
    bool terminate = false;

    //创建共享内存，作为所有客户socket连接的读缓存
    shmfd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    assert(shmfd >= 0);
    ret = ftruncate(shmfd, USER_LIMIT * BUFFER_SIZE); //将shmfd改为USER_LIMIT * BUFFER_SIZE大小
    assert(ret >= 0);
    //申请一段内存空间，将shmfd映射到其中
    share_mem = (char *)mmap(NULL, USER_LIMIT * BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    assert(share_mem != MAP_FAILED);
    close(shmfd);

    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
        if (number < 0 && errno != EINTR)
        {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            //客户端有数据到达
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &len);
                if (connfd < 0)
                {
                    printf("errno is %d\n", errno);
                    continue;
                }
                if (user_count >= USER_LIMIT)
                {
                    const char *info = "too many users!\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }
                //保存第user_count个客户连接的相关数据
                users[user_count].address = client_address;
                users[user_count].connfd = connfd;
                //在主进程和子进程之间建立管道，以传递必要的数据
                ret = socketpair(PF_UNIX, SOCK_STREAM, 0, users[user_count].pipefd);
                assert(ret != -1);
                pid_t pid = fork();
                if (pid < 0)
                {
                    close(connfd);
                    continue;
                }
                else if (pid == 0) //子进程
                {
                    close(listenfd);
                    close(epollfd);
                    close(users[user_count].pipefd[0]);
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    run_child(user_count, users, share_mem);
                    munmap((void *)share_mem, USER_LIMIT * BUFFER_SIZE);
                    exit(0);
                }
                else               //父进程
                { 
                    close(connfd);
                    close(users[user_count].pipefd[1]);          //关闭写端
                    addfd(epollfd, users[user_count].pipefd[0]); //读端加入epoll事件表
                    users[user_count].pid = pid;
                    //记录新的客户连接在数组users中的索引值：user_count,建立进程pid和该索引之间的映射关系
                    sub_process[pid] = user_count;
                    user_count++;
                }
            }
            //处理信号事件
            else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0)
                {
                    continue;
                }
                else
                {
                    for (int i = 0; i < ret; i++)
                    {
                        switch (signals[i])
                        {
                        //子进程退出，表示有某个客户端关闭了连接
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                            {
                                //用子进程的pid取得被关闭的客户端连接的编号
                                int del_user = sub_process[pid];
                                sub_process[pid] = -1;
                                if ((del_user<0) || (del_user> USER_LIMIT))
                                {
                                    continue;
                                }
                                //清除第del_user个客户连接使用的相关数据
                                epoll_ctl(epollfd, EPOLL_CTL_DEL, users[del_user].pipefd[0], 0);
                                close(users[del_user].pipefd[0]);
                                users[del_user] = users[--user_count];

                                sub_process[users[del_user].pid] = del_user;
                            }
                            if (terminate && user_count == 0)
                            {
                                stop_server = true;
                            }
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            printf("kill all the child now\n");
                            if (user_count == 0)
                            {
                                stop_server = true;
                                break;
                            }
                            for (int i = 0; i < user_count; i++)
                            {
                                int pid = users[i].pid;
                                kill(pid, SIGTERM);
                            }
                            terminate = true;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                        }
                    }
                }
            }
            //某个子进程向父进程写入了数据
            else if (events[i].events & EPOLLIN)
            {
                int child = 0;
                ret = recv(sockfd, (char *)&child, sizeof(child), 0);
                printf("read data from child across pipe\n");
                if (ret <= 0)
                {
                    continue;
                }
                else
                {
                    //
                    for (int j = 0; j < user_count; j++)
                    {
                        if (users[j].pipefd[0] != sockfd)
                        {
                            printf("send data to child across pipe\n");
                            send(users[j].pipefd[0], (char *)&child, sizeof(child), 0);
                        }
                    }
                }
            }
        }
    }
    del_resource();
    return 0;
}
