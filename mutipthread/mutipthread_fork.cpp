#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <wait.h>
#include <stdlib.h>

pthread_mutex_t mutex;
//子线程函数，首先获得互斥锁、暂停5s，再释放
void* another(void* arg)
{
    printf("in child thread, lock the mutex\n");
    pthread_mutex_lock(&mutex);
    sleep(5);
    pthread_mutex_unlock(&mutex);
}

void prepare(){
    pthread_mutex_lock(&mutex);
}

void infork(){
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char const *argv[])
{
    pthread_mutex_init(&mutex, NULL);
    pthread_t id;
    pthread_create(&id, NULL, another, NULL);
    sleep(1);
    pthread_atfork(prepare, infork, infork); 
    int pid = fork();      
    if (pid < 0)
    {
        pthread_join(id, NULL);
        pthread_mutex_destroy(&mutex);
        return 1;
    }
    else if (pid == 0)
    {
        printf("in the child, get the lock\n");
        pthread_mutex_lock(&mutex);
        printf("..\n");
        pthread_mutex_unlock(&mutex);
        exit(0);    //退出进程
    }
    else{
        wait(NULL);
    }
    pthread_join(id, NULL);
    pthread_mutex_destroy(&mutex);
    
    return 0;
}

