#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>

using namespace std;

class Buffer
{
public:
    Buffer(int initBuffsize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;                 //缓存区中还可以读取的字节数
    size_t ReadableBytes() const ;                 //缓存区中还可以写入的字节数
    size_t PrependableBytes() const;               //缓存区中已经读取的字节数

    const char* Peek() const;                      //缓冲区起始地址+读位置，可读地址
    void EnsureWriteable(size_t len);              //保证能够写入超过现有容量的数据
    void HasWritten(size_t len);                   //

    void Retrieve(size_t len);                     //读指定长度后读指针的的更新方法
    void RetrieveUntil(const char* end);           //将读指针移到指定位置的方法
    void RetrieveAll() ;
    std::string RetrieveAllToStr();


    void updateReadPtr(size_t len);                //写入指定长度后写指针的更新方法
    void initPtr();                                //读写指针初始化的方法。

    const char* curReadPtr() const;                //获取当前读指针
    const char* BeginWriteConst() const;          //获取当前写指针
    char* BeginWrite();

    void Append(const char* str, size_t len);  //向buffer缓冲区中添加数据
    void Append(const std::string& str);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buffer);

    ssize_t ReadFd(int fd, int* Errno);   //与客户端直接IO的读写接口
    ssize_t WriteFd(int fd, int* Errno);

 

private:
    const char* BeginPtr_() const;                 //返回指向缓冲区初始位置的指针
    char* BeginPtr_();

    void MakeSpace_(size_t len);          //分配新的空间

    vector<char> buffer_;
    std::atomic<std::size_t> readPos_;    //分别指示读写下标
    std::atomic<std::size_t> writePos_;   //原子数据类型，能直接在多线程中使用，不必加锁，
};

#endif