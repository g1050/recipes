/* #include "../Mutex.h" */
/* #include "../Thread.h" */

#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <boost/functional.hpp>
#include <set>
#include <stdio.h>

class Request;

class Inventory
{
 public:
  void add(Request* req)
  {
    muduo::MutexLockGuard lock(mutex_);
    requests_.insert(req);
  }

  void remove(Request* req) __attribute__ ((noinline))//Inventory加锁
  {
    muduo::MutexLockGuard lock(mutex_);
    requests_.erase(req);
  }

  void printAll() const;

 private:
  mutable muduo::MutexLock mutex_;
  std::set<Request*> requests_;
};

Inventory g_inventory;

class Request
{
 public:
  void process() // __attribute__ ((noinline))
  {
    muduo::MutexLockGuard lock(mutex_);
    g_inventory.add(this);
    // ...
  }

  ~Request() __attribute__ ((noinline))
  {
    g_inventory.remove(this);//先remove暂时解决了死锁问题
    muduo::MutexLockGuard lock(mutex_);//Request加锁
    sleep(1);
  }

  void print() const __attribute__ ((noinline))//Request的print方法加锁
  {
    muduo::MutexLockGuard lock(mutex_);
    // ...
  }

 private:
  mutable muduo::MutexLock mutex_;
};

void Inventory::printAll() const
{
  muduo::MutexLockGuard lock(mutex_);
  sleep(1);
  for (std::set<Request*>::const_iterator it = requests_.begin();
      it != requests_.end();
      ++it)
  {
    (*it)->print();//又调用了request的print方法
  }
  printf("Inventory::printAll() unlocked\n");
}

/*
void Inventory::printAll() const
{
  std::set<Request*> requests
  {
    muduo::MutexLockGuard lock(mutex_);
    requests = requests_;
  }
  for (std::set<Request*>::const_iterator it = requests.begin();
      it != requests.end();
      ++it)
  {
    (*it)->print();
  }
}
*/

void threadFunc()
{
  Request* req = new Request;
  req->process();
  delete req;
}

int main()
{
  /* main线程和threadfunc线程互相锁住 */
  muduo::Thread thread(threadFunc);
//Inventory加锁
  thread.start();
  usleep(500 * 1000);
  g_inventory.printAll();
  thread.join();
}
