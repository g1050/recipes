#include "../Mutex.h"
#include "../Thread.h"
#include <set>
#include <boost/shared_ptr.hpp>
#include <stdio.h>

class Request;

class Inventory
{
 public:
  Inventory()
    : requests_(new RequestList)
  {
  }

  //操作requests_,清单类自己加锁
  void add(Request* req)
  {//写时复制
    muduo::MutexLockGuard lock(mutex_);
    if (!requests_.unique())
    {
      requests_.reset(new RequestList(*requests_));
      printf("Inventory::add() copy the whole list\n");
    }
    assert(requests_.unique());
    requests_->insert(req);
  }

  void remove(Request* req) // __attribute__ ((noinline))
  {//写时复制
    muduo::MutexLockGuard lock(mutex_);
    if (!requests_.unique())
    {
      requests_.reset(new RequestList(*requests_));
      printf("Inventory::remove() copy the whole list\n");
    }
    assert(requests_.unique());
    requests_->erase(req);
  }

  void printAll() const;

 private:
  typedef std::set<Request*> RequestList;
  typedef boost::shared_ptr<RequestList> RequestListPtr;

  RequestListPtr getData() const
  {
    muduo::MutexLockGuard lock(mutex_);
    return requests_;
  }

  mutable muduo::MutexLock mutex_;
  RequestListPtr requests_;
};

Inventory g_inventory;

class Request
{
 public:
  Request()
    : x_(0)
  {
  }

  ~Request() __attribute__ ((noinline))
  {
    muduo::MutexLockGuard lock(mutex_);
    x_ = -1;
    sleep(1);
    g_inventory.remove(this);
  }

  //向全局添加自己
  //需要给自己加锁
  void process() // __attribute__ ((noinline))
  {
    muduo::MutexLockGuard lock(mutex_);
    g_inventory.add(this);
    // ...
  }

  //请求类自己加锁
  void print() const __attribute__ ((noinline))
  {
    muduo::MutexLockGuard lock(mutex_);
    // ...
    printf("print Request %p x=%d\n", this, x_);
  }

 private:
  mutable muduo::MutexLock mutex_;
  int x_;
};

void Inventory::printAll() const
{
  RequestListPtr requests = getData();
  sleep(1);
  for (std::set<Request*>::const_iterator it = requests->begin();
      it != requests->end();
      ++it)
  {
    (*it)->print();
  }
}

//子线程从此处执行
void threadFunc()
{
  Request* req = new Request;
  req->process();
  delete req;
}

/* 两个线程加锁顺序相反,互相锁住 */
/* Request对象析构时候的race condition? */
int main()
{
  muduo::Thread thread(threadFunc);
  thread.start();
  usleep(500*1000);
  g_inventory.printAll();
  thread.join();
}
