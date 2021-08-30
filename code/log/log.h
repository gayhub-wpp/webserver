#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
    void init(int level, const char* path = "./log",    
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    static Log* Instance();       //单例模式创建实例
    static void FlushLogThread();  //异步写日志公有方法，调用私有方法AsyncWrite_()

    void write(int level, const char *format,...);   //将输出内容按照标准格式整理·
    void flush();  //强制刷新缓冲区

    int GetLevel();   //获取日志等级
    void SetLevel(int level);  //设置日志等级
    bool IsOpen() { return isOpen_; }

private:
    Log();   
    void AppendLogLevelTitle_(int level);   //日志头
    virtual ~Log();       
    void AsyncWrite_();   //异步写日志方法

private:
    static const int LOG_PATH_LEN = 256;  
    static const int LOG_NAME_LEN = 256;   
    static const int MAX_LINES = 50000;  //日志最大行数

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;

    int lineCount_;
    int toDay_;

    bool isOpen_;
 
    Buffer buff_;     //缓冲区
    int level_;
    bool isAsync_;   //是否同步标志位

    FILE* fp_;    //打开log的文件指针
    std::unique_ptr<BlockDeque<std::string>> deque_;   //阻塞队列
    std::unique_ptr<std::thread> writeThread_;   //写线程
    std::mutex mtx_;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

//这四个宏定义在其他文件中使用，主要用于不同类型的日志输出
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H