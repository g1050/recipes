#include <algorithm>
#include <vector>
#include <stdio.h>

class Observable;

class Observer//观察者基类
{
 public:
  virtual ~Observer();
  virtual void update() = 0;

  void observe(Observable* s);

 protected:
  Observable* subject_;//观察者中保存着目标
};

class Observable//目标
{
 public:
  ~Observable(){printf("%p　目标析构\n",this);}
  void register_(Observer* x);//观察者订阅
  void unregister(Observer* x);//观察者解除订阅

  void notifyObservers()//自动通知所有观察者
  {
    for (size_t i = 0; i < observers_.size(); ++i)
    {
      Observer* x = observers_[i];
      if (x) {
        x->update(); // (3)
      }
    }
  }

 private:
  std::vector<Observer*> observers_;
};

Observer::~Observer()//析构的时候取消注册
{
  //有问题,1.如何得知subject_对象是否还存活
  //2.析构的同时,有其他线程调用了update怎么办
  printf("%p 析构\n",this);
  /* printf("目标%p \n",subject_); */
  subject_->unregister(this);
}

void Observer::observe(Observable* s)//观察者observe方法,初始化目标，并且订阅
{
  s->register_(this);
  subject_ = s;
}

void Observable::register_(Observer* x)
{
  observers_.push_back(x);
}

void Observable::unregister(Observer* x)
{
  std::vector<Observer*>::iterator it = std::find(observers_.begin(), observers_.end(), x);
  if (it != observers_.end())
  {
    std::swap(*it, observers_.back());//换到最后，然后弹出,妙!
    observers_.pop_back();
  }
}

// ---------------------
/* 自定义观察者 */
class Foo : public Observer
{
  virtual void update()//目标通知观察者的具体操作
  {
    printf("Foo::update() %p\n", this);
  }
};

class Foo2 : public Observer
{
    virtual void update(){
        printf("Foo2::update %p\n",this);
    }
};

int main()
{
  Foo* p = new Foo;//一个观察者
  Foo2* q = new Foo2;

  Observable *sub ;

  {
  Observable subject;//目标
  sub = &subject;

  p->observe(&subject);//给观察者初始化目标
  q->observe(&subject);

  subject.notifyObservers();
  delete p;//delete观察者对象,此时subject任然存活
  subject.notifyObservers();
  }

  /* sub->notifyObservers(); */
  delete q;//subject已经析构,有问题

}
