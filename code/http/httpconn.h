#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);

    ssize_t read(int* saveErrno);   //读写接口

    ssize_t write(int* saveErrno);

    void Close();

    int GetFd() const;  //获取HTTP连接的描述符，也就是唯一标志

    int GetPort() const;  //获取port信息

    const char* GetIP() const;  //获取IP信息
    
    sockaddr_in GetAddr() const;   //获取addr_
    
    bool process();    //完成解析请求和响应请求
 
    //获取已经写入的数据长度
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    //获取这个HTTP连接KeepAlive的状态
    bool IsKeepAlive() const {    
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;   //当前目录的路径
    static std::atomic<int> userCount;   //HTTP连接的个数，也就是用户的个数
    
private:
   
    int fd_;   //HTTP连接对应的描述符
    struct  sockaddr_in addr_;   

    bool isClose_;   //标记是否关闭连接
    
    int iovCnt_;
    struct iovec iov_[2];
    
    Buffer readBuff_; // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;    //解析请求
    HttpResponse response_;   //响应请求
};


#endif //HTTP_CONN_H