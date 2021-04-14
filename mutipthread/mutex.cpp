#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

int a = 0;
int b = 0;

pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;

void* another(void* arg){
    pthread_mutex_lock(&mutex_b);
    
    printf("in child thread ,got mutex b, waiting for mutex a\n");
    sleep(5);
    b++;
    pthread_mutex_lock(&mutex_a);
    b+= a++;
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    pthread_t id;
    pthread_mutex_init(&mutex_a, NULL); //初始化两个互斥锁
    pthread_mutex_init(&mutex_b, NULL);
    
    pthread_create(&id, NULL, another, NULL);    //子线程

    pthread_mutex_lock(&mutex_a);      //占有互斥锁a
    printf("in parent thread ,got mutex a, waiting for mutex b\n");
    sleep(5);
    a++;                               //操作a
    pthread_mutex_lock(&mutex_b);      //占有b
    a+= b++;                           //操作a、b
    pthread_mutex_unlock(&mutex_b);    //释放b
    pthread_mutex_unlock(&mutex_a);    //释放a

    pthread_join(id, NULL);
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);


    return 0;
}


