#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;
struct TimerNode
{
    int id;                //用来标记定时器
    TimeStamp expires;     //设置过期时间
    TimeoutCallBack cb;    //设置一个回调函数用来方便删除定时器时将对应的HTTP连接关闭
    bool operator<(const TimerNode& t){   //为了便于定时器结点的比较，主要是后续堆结构的实现方便，我们还需要重载比较运算符
        return expires < t.expires;
    }
};

class HeapTimer
{
public:
    HeapTimer(){heap_.reserve(64);} //预先分配64大小

    ~HeapTimer(){clear();}

    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const TimeoutCallBack& cb);   //添加一个定时器

    void doWork(int id);    //删除制定id节点，并且用指针触发处理函数

    void clear();

    void tick();      // 清除超时结点 */

    void pop();

    int GetNextTick();   //下一次处理过期定时器的时间

private:
    void del_(size_t i);                     //删除指定定时器

    void siftup_(size_t i);                  //向上调整

    bool siftdown_(size_t index, size_t n);  //向下调整

    void SwapNode_(size_t i, size_t j);      //交换两个结点位置

    std::vector<TimerNode> heap_;    

    std::unordered_map<int, size_t> ref_;    //映射一个fd对应的定时器在heap_中的位置

};




#endif