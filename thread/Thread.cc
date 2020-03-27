// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (giantchen at gmail dot com)

#include "Thread.h"

#include <boost/weak_ptr.hpp>

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>


/* 发现陈大佬花括号都写下面 */
/* 命名空间里面套命名空间 */
namespace muduo
{

namespace CurrentThread
{
//__thread修饰的变量在多个线程之间互补影响
  __thread const char* t_threadName = "unknown";
}
}

//匿名空间仅在当前文件有用
namespace
{

__thread pid_t t_cachedTid = 0;

pid_t gettid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{
  t_cachedTid = gettid();
  muduo::CurrentThread::t_threadName = "main";
  // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

class ThreadNameInitializer
{
 public:
  ThreadNameInitializer()
  {
    muduo::CurrentThread::t_threadName = "main";
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

ThreadNameInitializer init;

//封装的线程信息结构体
struct ThreadData
{
  typedef muduo::Thread::ThreadFunc ThreadFunc;//函数模板
  ThreadFunc func_;
  std::string name_;
  boost::weak_ptr<pid_t> wkTid_;

  ThreadData(const ThreadFunc& func,
             const std::string& name,
             const boost::shared_ptr<pid_t>& tid)
    : func_(func),
      name_(name),
      wkTid_(tid)
  { }

  void runInThread()
  {
    pid_t tid = muduo::CurrentThread::tid();
    boost::shared_ptr<pid_t> ptid = wkTid_.lock();

    if (ptid)
    {
      *ptid = tid;
      ptid.reset();
    }

    muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
    ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
    func_(); // FIXME: surround with try-catch, see muduo
    muduo::CurrentThread::t_threadName = "finished";
  }
};

void* startThread(void* obj)
{
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->runInThread();
  delete data;
  return NULL;
}

}

using namespace muduo;

//namespace muduo 下的三个函数
pid_t CurrentThread::tid()
{
  //没有分配线程ID
  if (t_cachedTid == 0)
  {
    //Linux下获取线程ID的方法
    t_cachedTid = gettid();
  }
  return t_cachedTid;
}

const char* CurrentThread::name()
{
  return t_threadName;
}

bool CurrentThread::isMainThread()
{
  //拿线程ID和pid比较
  return tid() == ::getpid();
}

//静态对象定义
//好像是个计数器
AtomicInt32 Thread::numCreated_;

//完成初始化工作
//有默认的name
Thread::Thread(const ThreadFunc& func, const std::string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(new pid_t(0)),
    func_(func),
    name_(n)

{
  numCreated_.increment();
}


/* 为什么没有给计数器减少一个 */
Thread::~Thread()
{
  //启动且不是可结合的,就分离线程
  if (started_ && !joined_)
  {
    printf("in调用析构函数\n");
    pthread_detach(pthreadId_);
  }
  printf("out调用析构函数\n");
}

void Thread::start()
{
  assert(!started_);//断言线程未启动
  started_ = true;

  //利用Thread类里的func_,name_,tid_初始化,在堆区开辟
  ThreadData* data = new ThreadData(func_, name_, tid_);

  /* 四个参数含义,pthread_t,attribute,start_routine,arg */
  if (pthread_create(&pthreadId_, NULL, &startThread, data))
  {
    started_ = false;
    delete data;
    abort();//用于程序异常退出,并且关闭所有打开的流
  }

}

void Thread::join()
{
  assert(started_);
  assert(!joined_);
  joined_ = true;
  pthread_join(pthreadId_, NULL);//主线程等待该线程结束
}
