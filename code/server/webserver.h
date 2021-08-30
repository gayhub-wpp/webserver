#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <string>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer
{
    
public:
    WebServer(
         /* 端口 ET模式 timeoutMs 优雅退出  */
        const char* ip, int port, int trigMode, int timeoutMS, bool OptLinger,   
        /* Mysql配置 */
        int sqlPort, const char* sqlUser, const  char* sqlPwd, const char* dbName, 
         /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
        int connPoolNum, int threadNum, bool openLog, int logLevel, int logQueSize); 
    ~WebServer();
    void Start();

private:
    bool InitSocket_(); //socket连接的建立过程
    void InitEventMode_(int trigMode); //事件模式的初始化
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    const char* ip_;
    int port_;   
    bool openLinger_;
    int timeoutMS_;  /* 毫秒MS , 定时器的默认过期时间*/
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;
    
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
 
};




#endif