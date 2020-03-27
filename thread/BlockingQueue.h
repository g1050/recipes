// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (giantchen at gmail dot com)

#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

/* #include "Condition.h" */
/* #include "Mutex.h" */

#include <muduo/base/Mutex.h>
#include <muduo/base/Condition.h>
#include <boost/noncopyable.hpp>
#include <deque>
#include <assert.h>

/* BlockingQueue,阻塞队列 */
namespace muduo
{

template<typename T>
class BlockingQueue : boost::noncopyable
{
 public:
  BlockingQueue()
    : mutex_(),
      notEmpty_(mutex_),
      queue_()
  {
  }

  //向任务队列中加入任务
  void put(const T& x)
  {//线程安全,临界区资源
    MutexLockGuard lock(mutex_);
    queue_.push_back(x);
    notEmpty_.notify();//放进去之后唤醒线程
  }

  //从任务队列中取出任务
  T take()
  {
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    // 防止虚假唤醒
    while (queue_.empty())
    {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    T front(queue_.front());//定义一个T类型的对象叫front,用队首元素来构造front
    queue_.pop_front();
    return front;
  }

  size_t size() const
  {//线程安全
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable MutexLock mutex_;//锁
  Condition         notEmpty_;//条件变量
  std::deque<T>     queue_;//双端队列
};

}

#endif  // MUDUO_BASE_BLOCKINGQUEUE_H
