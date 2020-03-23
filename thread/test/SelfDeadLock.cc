/* #include "../Mutex.h" */

#include <muduo/base/Mutex.h>

class Request
{
 public:
  void process() // __attribute__ ((noinline))
  {
    muduo::MutexLockGuard lock(mutex_);
    print();
  }

  void print() const // __attribute__ ((noinline))
  {
    muduo::MutexLockGuard lock(mutex_);//同一线程多次加锁，导致死锁
  }

 private:
  mutable muduo::MutexLock mutex_;
};

int main()
{
  Request req;
  req.process();
}
