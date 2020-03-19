#include <algorithm>
#include <vector>
#include <stdio.h>
#include "muduo/base/Mutex.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


class Observable;

class Observer : public boost::enable_shared_from_this<Observer>
{
 public:
  virtual ~Observer();
  virtual void update() = 0;

  void observe(Observable* s);

 protected:
  Observable* subject_;
};

class Observable
{
 public:
  void register_(boost::weak_ptr<Observer> x);
  // void unregister(boost::weak_ptr<Observer> x);

  void notifyObservers()
  {
    muduo::MutexLockGuard lock(mutex_);//加锁，解决问题2
    Iterator it = observers_.begin();
    while (it != observers_.end())
    {
      boost::shared_ptr<Observer> obj(it->lock());
      if (obj)
      {
        obj->update();
        ++it;
      }
      else
      {
        printf("notifyObservers() erase\n");
        it = observers_.erase(it);//从vector中删除当前观察者,观察者已经析构
      }
    }
  }

 private:
  mutable muduo::MutexLock mutex_;//多了一个锁，解决静态问题
  std::vector<boost::weak_ptr<Observer> > observers_;
  typedef std::vector<boost::weak_ptr<Observer> >::iterator Iterator;
};

//借助shared_ptr/weaak_ptr整个Observer不需要unregister,通过wea_ptr探究Observer的生死
Observer::~Observer()//改变了在析构函数里调用unregister
{
  // subject_->unregister(this);
}

void Observer::observe(Observable* s)
{
  s->register_(shared_from_this());//返回shared_ptr指针,给了目标一个shared_ptr类型的观察者指针
  subject_ = s;
}

void Observable::register_(boost::weak_ptr<Observer> x)//转化为weak_ptr
{
  observers_.push_back(x);
}

//void Observable::unregister(boost::weak_ptr<Observer> x)
//{
//  Iterator it = std::find(observers_.begin(), observers_.end(), x);
//  observers_.erase(it);
//}

// ---------------------

class Foo : public Observer
{
  virtual void update()
  {
    printf("Foo::update() %p\n", this);
  }
};

int main()
{
  Observable subject;
  {
    boost::shared_ptr<Foo> p(new Foo);//创建观察者,只要观察者不析构，目标就会通知
    p->observe(&subject);//订阅
    subject.notifyObservers();
  }
  subject.notifyObservers();
}

