// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (giantchen at gmail dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include "Mutex.h"

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <errno.h>

namespace muduo //命名空间muduo
{

class Condition : boost::noncopyable//不允许拷贝
{
 public:
  /* 构造函数，传入一个互斥所，并且初始化条件变量 */
  explicit Condition(MutexLock& mutex) : mutex_(mutex)
  {
    pthread_cond_init(&pcond_, NULL);
  }

  ~Condition()
  {
    pthread_cond_destroy(&pcond_);
  }

  void wait()
  {
    pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
  }

  /* // returns true if time out, false otherwise. */
  /* pthread_cond_timedwait(3C) 的用法与 pthread_cond_wait() 的用法基本相同，区别在于在由 abstime 指定的时间之后 pthread_cond_timedwait() 不再被阻塞。 */
  /* pthread_cond_timedwait() 函数会一直阻塞，直到该条件获得信号，或者最后一个参数所指定的时间已过为止。 */
    
  /* 被唤醒，或者到达指定时间之后 */
  bool waitForSeconds(int seconds)
  {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += seconds;
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
  }

  void notify()//一个生产者，一个消费者，或者生产者一次生产一个产品
  {
    pthread_cond_signal(&pcond_);
  }

  void notifyAll()//一次生产多个产品，或者消费者有多个
  {
    pthread_cond_broadcast(&pcond_);
  }

 private:
  MutexLock& mutex_;
  pthread_cond_t pcond_;
};

}
#endif  // MUDUO_BASE_CONDITION_H
