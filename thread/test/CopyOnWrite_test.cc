/* #include "../Mutex.h" */
/* #include "../Thread.h" */
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

#include <vector>
#include <boost/shared_ptr.hpp>
#include <stdio.h>

using namespace muduo;

class Foo
{
 public:
  void doit() const;
};

typedef std::vector<Foo> FooList;
typedef boost::shared_ptr<FooList> FooListPtr;//利用智能指针的引用计数记录观察者
FooListPtr g_foos;
MutexLock mutex;

//写端
void post(const Foo& f)
{
  printf("post\n");

  //临界区
  MutexLockGuard lock(mutex);
  if (!g_foos.unique())
  {//不是唯一的一个观察者
    g_foos.reset(new FooList(*g_foos));//复制一份新的,原来vector在第二个shared_ptr析构后也析构
    printf("copy the whole list\n");
  }

  //只有唯一观察者的时候直接加入f
  assert(g_foos.unique());
  g_foos->push_back(f);
}

//读端,直接给引用结束加一
void traverse()
{
  FooListPtr foos;//添加一个观察者,下面是临界区
  {
    MutexLockGuard lock(mutex);
    foos = g_foos;//并发读shared_ptr必须保护起来
    assert(!g_foos.unique());//此时观察者不唯一
  }

  // assert(!foos.unique()); this may not hold

  //读前给引用计数+1  
  for (std::vector<Foo>::const_iterator it = foos->begin();
      it != foos->end(); ++it)
  {
    it->doit();//利用新的观察者doit()
  }
 
  //观察者析构后引用计数-1,阻止了并发写
}

//doit 修改vector
void Foo::doit() const
{
  Foo f;
  post(f);
}

int main()
{
  g_foos.reset(new FooList);//全局变量g_foos

  //先来一个写操作,此时只有一个观察者
  Foo f;
  post(f);//将post加入观察者list中

  //再来读,读又包含了写
  traverse();

}

