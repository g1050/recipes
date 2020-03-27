#include <iostream>
#include <threads.h>
#include <unistd.h>


void*  func(void *arg)
{
    (void)arg;
    sleep(1);
    printf("func working\n");
    return NULL;
}

void* stage(void *arg)
{
    (void)arg;

    pthread_t th;
    pthread_create(&th,NULL,&func,NULL);
    /* pthread_join(th,NULL); */
    return NULL;
}

int main()
{
    sleep(10);
    pthread_t th;
    pthread_create(&th,NULL,&stage,NULL);
    pthread_join(th,NULL);
    sleep(10);
    printf("Main thread working\n");
    return 0;
}

