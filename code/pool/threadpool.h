#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
using namespace std;
class ThreadPool
{
private:
    struct Pool{
        mutex mtx;
        condition_variable cond;
        bool isClosed;
        queue<function<void()>> tasks;
    };
    shared_ptr<Pool> pool_;
public:
    explicit ThreadPool(size_t threadCount = 8):pool_(make_shared<Pool>()){
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; i++)
        {
            thread([pool = pool_]{
                unique_lock<mutex> locker(pool->mtx);
                while(true)
                {
                    if(!pool->tasks.empty()){
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if(pool->isClosed) break;
                    else pool->cond.wait(locker);                  
                }
            }).detach();
        }
    };

    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& task){
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));    
        }
        pool_->cond.notify_one();
    }
};


#endif