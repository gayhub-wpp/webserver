# 阻塞队列
## key
### wait
阻塞当前进程，unlock锁，供其他线程操作，直到其他线程notify唤醒该线程再lock
### notify_all()
会唤醒所有等待队列中阻塞的线程
### notify_one()
只唤醒等待队列中的第一个线程
## 成员变量
std::deque<T> deq_;   双向队列
size_t capacity_;     容量
bool isClose_;        是否关闭
std::condition_variable condConsumer_; 消费者条件变量
std::condition_variable condProducer_; 生产者消费变量

## 成员函数
### BlockQueue(size_t MaxCapacity = 1000);
构造函数
### ~BlockQueue();
析构
### void clear();
清空队列
### bool empty();
判空
### bool full();
判满
### void Close();
清空并关闭
### size_t size();
### size_t capacity();
### T front();
### T back();
### void push_back(const T& item);
### void push_front(const T& item);
### bool pop(T &item);
### bool pop(T &item, int timeout);
### void flush();