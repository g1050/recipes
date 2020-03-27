// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (giantchen at gmail dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "./Atomic.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <pthread.h>

namespace muduo
{

/* muduo库对thread的封装 */
class Thread : boost::noncopyable
{
 public:
  typedef boost::function<void ()> ThreadFunc;//函数模板类

  /* 构造函数传入一个回调函数和name ,name默认值是一个串*/
  explicit Thread(const ThreadFunc&, const std::string& name = std::string());
  ~Thread();

  void start();
  void join();

  bool started() const { return started_; }
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return *tid_; }
  const std::string& name() const { return name_; }

  //计数器,所有通过Thread类来创建的线程都会+1
  static int numCreated() { return numCreated_.get(); }

 private:
  bool        started_;//
  bool        joined_;
  pthread_t   pthreadId_;//类型不缺定,且不唯一
  boost::shared_ptr<pid_t> tid_;//pid_t的智能指针
  ThreadFunc  func_;//回调函数
  std::string name_;//啥名字啊?

  static AtomicInt32 numCreated_;//静态对象
};

namespace CurrentThread
{
  pid_t tid();
  const char* name();
  bool isMainThread();
}

}

#endif
