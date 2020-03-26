#include <iostream>
#include <unistd.h>

int main()
{
    /* 系统调用打印当前线程ID */
    printf("%d",getpid());
    sleep(3);
    return 0;
}

