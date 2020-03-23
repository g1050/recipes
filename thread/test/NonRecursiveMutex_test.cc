/* #include "../Mutex.h" */
/* #include "../Thread.h" */

#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <vector>
#include <stdio.h>

using namespace muduo;

class Foo
{
 public:
  void doit() const;
};

MutexLock mutex;
std::vector<Foo> foos;//全局对象foos

void post(const Foo& f)
{
  MutexLockGuard lock(mutex);
  foos.push_back(f);
}

void traverse()
{
  MutexLockGuard lock(mutex);
  for (std::vector<Foo>::const_iterator it = foos.begin();
      it != foos.end(); ++it)
  {
    it->doit();//做法一,记录需要操作的vector,待traverse结束后
    //做法二:写时复制
  }
}

void Foo::doit() const
{
  Foo f;
  post(f);
}

int main()
{
  Foo f;
  post(f);//死锁,同一线程对同一个锁两次尝试加锁
  traverse();
}

