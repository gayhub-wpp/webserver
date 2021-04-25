#include <sys/types.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <assert.h>
// #include <unistd.h>
// #include <stdio.h>
// #include <errno.h>
// #include <string.h>
// #include <fcntl.h>
// #include <sys/epoll.h>
// #include <signal.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>>


using namespace std;

//线程安全懒汉，双检测锁模式 
// class single
// {
// private:
//     static single *p; //静态锁
//     static pthread_mutex_t lock;

//     single(){
//         pthread_mutex_init(&lock, NULL);
//     }

//     ~single() {}

// public:
//     static single* getinstance();
// };

// pthread_mutex_t single::lock;

// single* single::p = NULL;
// single* single::getinstance()
// {
//     if (p == NULL)
//     {
//         pthread_mutex_lock(&lock);
//         if (p == NULL)
//         {
//             p = new single;
//         }
//         pthread_mutex_unlock(&lock);
//     }
//     return p;
    
// }


//静态
class single
{
private:
    static single* p;
    static pthread_mutex_t lock;
    single(){}
    ~single(){}
public:
    static single* getinstance();
};

single* single::p = NULL;
pthread_mutex_t single::lock;

// c++11要求编译器保证内部静态变量的线程安全性
single* single::getinstance()
{
    static single obj;
    return &obj;
}