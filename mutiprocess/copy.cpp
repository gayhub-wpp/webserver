#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/sendfile.h>
#include<sys/uio.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<pthread.h>
#include<sys/epoll.h>
#include<sys/wait.h>
#include<sys/mman.h>
#include<signal.h>
using namespace std;

#define USER_LIMIT 5
#define BUFFER_SIZE 1024
#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define PROCESS_LIMIT 65536

//处理一个客户连接的必要数据
struct client_data{
    sockaddr_in address;//客户端socket地址
    int connfd;//socket文件描述符
    pid_t pid;//处理这个连接的子进程PID
    int pipefd[2];//和父进程通信的管道:双向读写管道[主进程监听0端][子进程监听1端]
};

//共享内存名称
static const char* shm_name = "/my_shm";
int sig_pipefd[2];
int epollfd;
int listenfd;
int shmfd;
char* share_mem = 0;

//客户连接的数组 users[客户连接编号]=相关客户连接数据
client_data* users = 0;
//子进程和客户连接的映射关系 sub_process[进程的PID]=客户连接编号
int* sub_process = 0;
//当前客户数量
int user_count = 0;
bool stop_child = false;

//设置文件描述符为非阻塞
int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

//向epoll表注册文件描述符fd(并设置fd为非阻塞模式)
void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;//数据可读事件|边沿触发模式
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);//向epoll表注册事件
    setnonblocking(fd);
}

//信号处理函数
void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    //把信号通过sig_pipefd管道发送给主进程
    send(sig_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;//保持可重入性
}

//添加handler(int)函数用于处理sig信号
void addsig(int sig,void(*handler)(int),bool restart = true){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;//添加信号处理函数
    if(restart){
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);//信号处理函数执行期间屏蔽所有信号
    assert(sigaction(sig,&sa,NULL) != -1);//设置sig的自定义信号处理函数
}

//清除所有资源
void del_resource(){
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(listenfd);
    close(epollfd);
    shm_unlink(shm_name);//关闭POSIX共享内存
    delete[] users;
    delete[] sub_process;
}

//停止一个子进程
void child_term_handler(int sig){
    stop_child = true;
}

//运行子进程
int run_child(int idx,client_data* users,char* share_mem){
    /*idx:客户连接编号
    users:保存所有客户连接数据的数据
    share_mem:共享内存的起始地址*/
    epoll_event events[MAX_EVENT_NUMBER];
    //子进程使用I/O复用技术同时监听:客户连接socket 同父进程通信的管道文件描述符
    int child_epollfd = epoll_create(5);
    assert(child_epollfd != -1);
    //注册对 客户连接socket 的epoll监听
    int connfd = users[idx].connfd;
    addfd(child_epollfd,connfd);
    //注册对 父进程通信管道文件描述符 的epoll监听
    int pipefd = users[idx].pipefd[1];
    addfd(child_epollfd,pipefd);

    int ret;
    //子进程设置自己的信号处理函数:SIGTERM信号用于停止子进程
    addsig(SIGTERM,child_term_handler,false);

    while(!stop_child){
        //非阻塞epoll_wait()
        int number = epoll_wait(child_epollfd,events,MAX_EVENT_NUMBER,-1);
        if( (number < 0) && (errno != EINTR) ){
            printf("epoll failure\n");
            break;
        }

        for(int i=0;i<number;i++){
            int sockfd = events[i].data.fd;
            /* 某一子进程接收到数据 => 该子进程把数据写入自己的共享内存里 =>
                该子进程通过管道通知主进程自己的编号 => 主进程通过管道向其他进程通知该编号
                => 其他进程依次访问该共享内存中的数据并将其发送给自己的客户端 */
            if((sockfd == connfd) && (events[i].events & EPOLLIN)){//监听客户
                //只对共享内存中的一块内存进行初始化
                memset(share_mem + idx*BUFFER_SIZE,'\0',BUFFER_SIZE);
                //将数据读入到这块读内存当中
                ret = recv(connfd,share_mem+idx*BUFFER_SIZE,BUFFER_SIZE-1,0);
                if(ret < 0){
                    if(errno != EAGAIN){
                        stop_child = true;
                    }
                    //errno==EAGAIN:提示稍后再试
                    //一次读取无法完全读取留到下一次
                }else if(ret == 0){
                    stop_child = true;
                }else{
                    //通知主进程接收到新消息的当前子进程的编号
                    send(pipefd,(char*)&idx,sizeof(idx),0);
                }
            }else if( (sockfd == pipefd) && (events[i].events & EPOLLIN) ){//监听管道
                int client = 0;
                //接收主进程发来的编号
                ret = recv(sockfd,(char*)&client,sizeof(client),0);//这里是将数据接收到一个int中
                //则 需要发送的数据就在 编号为client子进程对应的共享内存段 中
                if(ret < 0){
                    if(errno != EAGAIN){
                        stop_child = true;
                    }
                    //errno==EAGAIN:提示稍后再试
                    //一次读取无法完全读取留到下一次
                }else if(ret == 0){
                    stop_child = true;
                }else{//将client子进程收到的聊天信息发送给当前子进程对应的客户端
                    send(connfd,share_mem+client*BUFFER_SIZE,BUFFER_SIZE,0);
                }
            }else{
                continue;
            }
        }//for
    }//while
    close(connfd);
    close(pipefd);
    close(child_epollfd);
    return 0;
}

// g++ chat.cpp -o chat -lrt
// ./chat 172.17.0.6 12345
// telnet 172.17.0.6 12345
// telnet 111.229.237.34 12345

int main(int argc,char* argv[]){
    if(argc <= 2){
        printf("usage: %s ip_address port_number\n",basename(argv[0]));
        return 1;
    }
    //初始化主进程监听socket
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd >= 0);

    ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd,5);
    assert(ret != -1);

    //初始化
    user_count = 0;
    users = new client_data[USER_LIMIT + 1];
    sub_process = new int[PROCESS_LIMIT];
    for(int i=0;i<PROCESS_LIMIT;++i){
        sub_process[i] = -1;
    }

    //创建epoll表并注册主进程socket的监听事件
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd,listenfd);

    //初始化父子进程间的通信管道
    ret = socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);//可读写管道
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    addfd(epollfd,sig_pipefd[0]);//父进程通过epoll监听来自子进程的数据

    addsig(SIGCHLD,sig_handler);//子进程退出或暂停信号
    addsig(SIGTERM,sig_handler);//进程终止信号
    addsig(SIGINT,sig_handler);//Ctrl+C终止信号
    addsig(SIGPIPE,SIG_IGN);//忽略管道引起的信号
    bool stop_sever = false;
    bool terminate = false;

    //创建所有客户socket连接的共享读缓存
    shmfd = shm_open(shm_name,O_CREAT|O_RDWR,0666);
    assert(shmfd != -1);
    ret = ftruncate(shmfd,USER_LIMIT*BUFFER_SIZE);//设置缓存区大小
    assert(ret != -1);
    //用mmap申请一段内存使其同shmfd对应的内存关联
    share_mem = (char*)mmap(NULL,USER_LIMIT*BUFFER_SIZE,    \
                PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
    /*这个函数让我们在 操作用户区的文件或POSIX共享内存 时,使用操作内存的方法,而不是读写文件的方法
    void *mmap(void *addr, size_t length, int prot, int flags,int fd, off_t offset);
        addr == NULL:此时系统会自动分配一块内存
        length:这是内存段的大小
        prot == PROT_READ(可读)|PROT_WRITE(可写)
        flags == MAP_SHARED:在进程间共享该内存.对该内存段的修改将反映到被映射的文件中
        fd == shmfd:关联的文件描述符
        offset == 0:内存偏移
    */
    assert(share_mem != MAP_FAILED);
    close(shmfd);//为什么要关闭
    
    //监听开始
    while(!stop_sever){
        int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if((number < 0) && (errno != EINTR)){
            printf("epoll failure\n");
            break;
        }

        //处理所有存在响应的事件
        for(int i=0;i<number;i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){//主进程的响应
                //获取客户端连接-文件描述符
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
                if(connfd < 0){
                    printf("errno is: %d\n",errno);
                    continue;
                }
                //用户量过大不予处理
                if(user_count >= USER_LIMIT){
                    const char* info = "too many users\n";
                    printf("%s",info);
                    send(connfd,info,strlen(info),0);
                    close(connfd);
                    continue;
                }
                //保存第user_count个客户连接的相关数据
                users[user_count].address = client_address;
                users[user_count].connfd = connfd;
                //在主进程和子进程间建立管道以传递必要数据:
                //1.子进程向主进程汇报自己已经接收了新数据到自己的共享内存段=>接收聊天信息
                //2.主进程要求子进程向各自客户端发送指定的共享内存中的数据=>将信息同步给客户
                //这个管道必须是双向的:主进程需要读写 子进程也需要读写
                ret = socketpair(PF_UNIX,SOCK_STREAM,0,users[user_count].pipefd);
                assert(ret != -1);

                //生成子进程
                pid_t pid = fork();
                if(pid < 0){//失败
                    close(connfd);
                    continue;
                }else if(pid == 0){//子进程
                    //关闭所有的父进程维护资源
                    close(epollfd);
                    close(listenfd);
                    //这里关闭了子进程中的管道0端
                    //但是父进程依旧可以从0端进行读写 管道并未完全关闭
                    close(users[user_count].pipefd[0]);
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    run_child(user_count,users,share_mem);
                    munmap((void*)share_mem,USER_LIMIT*BUFFER_SIZE);//释放子进程的共享POSIX
                    exit(0);
                }else{//父进程
                    close(connfd);
                    close(users[user_count].pipefd[1]);
                    addfd(epollfd,users[user_count].pipefd[0]);//开始监听子进程对应的管道
                    users[user_count].pid = pid;
                    sub_process[pid] = user_count;
                    user_count++;
                }
            }else if( (sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN) ){//来自子进程的信号
                //子进程捕获了内核发来的信号 并通过sig_pipefd管道将信号转发给主进程
                int sig;
                char signals[1024];//由于管道内可能会积累很多信号值 所以这里采用了数组统一大量接收
                ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
                if(ret == -1){
                    continue;
                }else if(ret == 0){
                    continue;
                }else{//确实接收到了信号
                    //由于每一个信号值正好是4个字节(一个int或一个char)所以逐个遍历
                    for(int i=0;i<ret;i++){
                        switch(signals[i]){
                            case SIGCHLD:{//某个子进程结束 等待父进程读取其状态 现在是僵尸进程
                                pid_t pid;
                                int stat;
                                while( (pid = waitpid(-1,&stat,WNOHANG)) > 0 ){
                                    int del_user = sub_process[pid];
                                    sub_process[pid] = -1;
                                    if( (del_user < 0) || (del_user > USER_LIMIT) ){
                                        continue;
                                    }
                                    //清除第del_user个客户连接使用的相关数据
                                    epoll_ctl(epollfd,EPOLL_CTL_DEL,users[del_user].pipefd[0],0);
                                    close(users[del_user].pipefd[0]);
                                    users[del_user] = users[--user_count];
                                    sub_process[users[del_user].pid] = del_user;
                                }
                                if(terminate && user_count == 0){
                                    //terminate用于排除服务器空转时的情况
                                    //terminate反映了人是否要求服务器停止
                                    stop_sever = true;
                                }
                            }break;
                            //下面是要求主进程结束整个服务器程序的信号
                            case SIGTERM:
                            case SIGINT:{
                                printf("kill all the child now\n");
                                if(user_count == 0){
                                    stop_sever = true;
                                    break;
                                }
                                for(int i=0;i<user_count;i++){
                                    int pid = users[i].pid;
                                    kill(pid,SIGTERM);
                                    //发送给子进程的SIGTERM信号并不会回馈到主进程
                                    //他们将单独被子进程捕获并促使子进程结束自己
                                    //子进程结束后 内核向主进程发送SIGCHLD要求其释放记录
                                    //主进程释放各个子进程的记录并不断减小user_count
                                }
                                terminate = true;//设置为true user_count为0时服务器就结束了
                            }break;
                            default:break;
                        }
                    }
                }

            }else if(events[i].events & EPOLLIN){//监听到聊天信息的发送
                //这里主进程监听的都是 同各个子进程通信的双向管道的0端
                int child = 0;
                ret = recv(sockfd,(char*)&child,sizeof(child),0);
                printf("read data from child accross pipe\n");
                if(ret == -1){
                    continue;
                }else if(ret == 0){
                    continue;
                }else{
                    //向除负责处理第child个客户连接的子进程之外的其他子进程发送消息
                    //通知他们有数据要发给各自的客户端 数据在编号child子进程对应的共享内存中
                    for(int j=0;j<user_count;j++){
                        if(users[j].pipefd[0] != sockfd){
                            printf("send data to child accross pipe\n");
                            send(users[j].pipefd[0],(char*)&child,sizeof(child),0);
                        }
                    }
                }
            }
        }
    }
    del_resource();
    return 0;
}
