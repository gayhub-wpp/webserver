#ifndef LST_TIMER
#define LST_TIMER
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#define BUFFER_SIZE 64

using namespace std;
class util_timer;

struct client_data{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
};

//定时器类
class util_timer
{    
public:
    util_timer():prev(NULL), next(NULL){}
    
public:
    time_t expire; //超时时间
    void (*cb_func)(client_data*);  //任务回调函数
    //回调函数处理的客户数据
    client_data* user_data;
    util_timer* prev;   //指向前一个定时器
    util_timer* next;   //指向下一个定时器
};

//定时器链表，一个升序、双向链表，带头接点尾接点
class sort_timer_lst
{
private:
    util_timer* head;
    util_timer* tail; 
public:
    sort_timer_lst() :head(NULL), tail(NULL){}

    ~sort_timer_lst() {
        util_timer* tmp = head;
        while (tmp)
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    /*将目标定时器timer添加到链表中*/
    void add_timer(util_timer* timer){
        if (!timer)
        {
            return;
        }
        if (!head)
        {
            head = tail = timer;
            return;
        }
        if (timer->expire < head->expire)
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer(timer, head); //重载，插入链表中合适的位置
    }
    //调整定时器在链表中的位置，只考虑超时时间延长，往链表尾部移动
    void adjust_timer(util_timer* timer){
        if(!timer){
            return;
        }
        util_timer* tmp = timer->next;
        if(!tmp ||tmp->expire > timer->expire)
        {
            return;
        }
        if (timer == head)
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer(timer, head);
        }
        else{
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer(timer, timer->next);
        }
    }
    //删除定时器
    void del_timer(util_timer* timer){
        if (!timer)
        {
            return;
        }
        if (timer ==head && timer == tail)
        {
            delete timer;
            head = nullptr;
            tail = nullptr;
            return;
        }
        if (timer == head)
        {
            head = head->next;
            head->prev = nullptr;
            delete timer;
            return;
        }
        if (timer == tail)
        {
            tail = tail->prev;
            tail->next = nullptr;
            delete timer;
            return;
        }
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    
    //SIGALRM信号每次触发就在其信号处理函数中执行一次tick函数，以处理链表上到期的任务
    void tick(){
        if (!head)
        {
            return;
        }
        printf("time tick\n");
        time_t cur = time(NULL);
        util_timer* tmp = head;
        //从头接点开始处理每个定时器，直到遇到一个尚未到期的定时器
        while (tmp)
        {
            if (cur < tmp->expire)
            {
                return;
            }
            //调用回调函数，执行定时任务
            tmp->cb_func(tmp->user_data);
            head = tmp->next;
            if (head)
            {
                head->prev = nullptr;
            }
            delete tmp;
            tmp = head;
        }
    }


private:
    //插入头接点之后的链表中
    void add_timer(util_timer* timer, util_timer* lst_head){
        util_timer* prev = lst_head;
        util_timer* tmp = prev->next;
        while (tmp)
        {
            //当遍历到tmp的超时时间大于timer的超时时间，timer插入到prev 、tmp中间
            if (timer->expire < tmp->expire)
            {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        if (!tmp)
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
                
    }
};

#endif