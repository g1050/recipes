#include "../Thread.h"

#include <string>
#include <boost/bind.hpp>
#include <stdio.h>

void threadFunc()
{
  printf("tid=%d\n", muduo::CurrentThread::tid());
}

void threadFunc2(int x)
{
  printf("tid=%d, x=%d\n", muduo::CurrentThread::tid(), x);
}

void threadFunc3()
{
  printf("tid=%d\n", muduo::CurrentThread::tid());
  sleep(1);
}

class Foo
{
 public:
  explicit Foo(double x)
    : x_(x)
  {
  }

  void memberFunc()
  {
    printf("tid=%d, Foo::x_=%f\n", muduo::CurrentThread::tid(), x_);
    printf("%s",muduo::CurrentThread::name());
  }

  void memberFunc2(const std::string& text)
  {
    printf("tid=%d, Foo::x_=%f, text=%s\n", muduo::CurrentThread::tid(), x_, text.c_str());
  }

 private:
  double x_;
};

/* Thread类的使用测试 */
int main()
{

/* C++另外有一种匿名的命名空间,来保证生成的符号是局部的,这样对于匿名空间中的变量等,外部都是不可见的. */
/* 获取当前进程的ID,线程ID */
  printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());


  {
  muduo::Thread t1(threadFunc);//创建一个线程
  t1.start();//新线程跳转到回调函数
  t1.join();//等待创建的子线程结束
  }

  /* 应用函数模板 */
  //bind函数???
  muduo::Thread t2(boost::bind(threadFunc2, 42),
                   "thread for free function with argument");
  t2.start();
  t2.join();

  Foo foo(87.53);
  muduo::Thread t3(boost::bind(&Foo::memberFunc, &foo),
                   "thread for member function without argument");
  t3.start();
  t3.join();

  muduo::Thread t4(boost::bind(&Foo::memberFunc2, boost::ref(foo), std::string("Shuo Chen")),"member func with argument");
  t4.start();
  t4.join();

  {
    muduo::Thread t5(threadFunc3);
    t5.start();
  }

  sleep(2);

  printf("number of created threads %d\n", muduo::Thread::numCreated());

}
