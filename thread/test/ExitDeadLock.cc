#include <muduo/base/Mutex.h>

#include <stdio.h>
#include <stdlib.h>

using namespace muduo;

void someFunctionMayCallExit()
{
  exit(1);//exit除了结束进程还会析构全局对象
}

class GlobalObject
{
 public:
  void doit()
  {
    /* MutexLockGuard lock(mutex_); *///第一次加锁//第一次加锁
    someFunctionMayCallExit();
  }

  ~GlobalObject()
  {
    printf("GlobalObject:~GlobalObject\n");//析构
    MutexLockGuard g(mutex_);//加锁
    // clean up
    printf("GlobalObject:~GlobalObject cleanning\n");
  }

 private:
  MutexLock mutex_;
};

GlobalObject g_obj;//全局对象

int main()
{
  g_obj.doit();//死锁了
}
