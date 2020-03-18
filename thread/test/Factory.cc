#include <map>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Mutex.h>
/* #include "../Mutex.h" */

#include <assert.h>
#include <stdio.h>

using std::string;

//股票类定义
class Stock : boost::noncopyable
{
public:
    Stock(const string& name)
        : name_(name)
    {
        printf(" Stock[%p] %s\n", this, name_.c_str());//打印地址和股票名字
    }

    ~Stock()
    {
        printf("~Stock[%p] %s\n", this, name_.c_str());
    }

    const string& key() const { return name_; }

private:
    string name_;
};

/********************版本一*********************/
namespace version1
{

// questionable code
class StockFactory : boost::noncopyable
    {
    public:

        boost::shared_ptr<Stock> get(const string& key)
        {
            muduo::MutexLockGuard lock(mutex_);
            boost::shared_ptr<Stock>& pStock = stocks_[key];
            //如果股票不存在会默认构造一个
            if (!pStock)
            {
                pStock.reset(new Stock(key));
            }
            return pStock;
        }


    private:
        mutable muduo::MutexLock mutex_;
        /* 此处有问题,如果所有都用不到股票应该被析构 */
        std::map<string, boost::shared_ptr<Stock> > stocks_;//sp智能指针
    };

}


/********************版本二*********************/
namespace version2
{
/* 轻微内存泄露,stocks_大小只增不减 */
class StockFactory : boost::noncopyable
    {
    public:
        boost::shared_ptr<Stock> get(const string& key)
        {
            boost::shared_ptr<Stock> pStock;//定义一个空的sp
            muduo::MutexLockGuard lock(mutex_);
            boost::weak_ptr<Stock>& wkStock = stocks_[key];
            pStock = wkStock.lock();
            //不存在就创建
            if (!pStock)
            {
                pStock.reset(new Stock(key));
                wkStock = pStock;
            }
            return pStock;
        }

        int getSize(){
            return stocks_.size();
        }

    private:
        mutable muduo::MutexLock mutex_;
        std::map<string, boost::weak_ptr<Stock> > stocks_;
    };

}

namespace version3
{

class StockFactory : boost::noncopyable
    {
    public:

        boost::shared_ptr<Stock> get(const string& key)
        {
            boost::shared_ptr<Stock> pStock;
            muduo::MutexLockGuard lock(mutex_);
            boost::weak_ptr<Stock>& wkStock = stocks_[key];
            pStock = wkStock.lock();
            if (!pStock)
            {
                pStock.reset(new Stock(key),
                             boost::bind(&StockFactory::deleteStock, this, _1));//把this指针给了function,如果StackFactory先于Stock析构,会出问题
                wkStock = pStock;//更新了map里的stock
            }
            return pStock;
        }
        int getSize(){
            return stocks_.size();
        }
    private:

        void deleteStock(Stock* stock)
        {
            printf("deleteStock[%p]\n", stock);
            if (stock)
            {
                muduo::MutexLockGuard lock(mutex_);
                //从map中移除了析构的对象
                stocks_.erase(stock->key());  // This is wrong, see removeStock below for correct implementation.
            }
            delete stock;  // sorry, I lied
        }

        mutable muduo::MutexLock mutex_;
        std::map<string, boost::weak_ptr<Stock> > stocks_;
    };

}

namespace version4
{

class StockFactory : public boost::enable_shared_from_this<StockFactory>,
    boost::noncopyable
    {
    public:

        boost::shared_ptr<Stock> get(const string& key)
        {
            boost::shared_ptr<Stock> pStock;
            muduo::MutexLockGuard lock(mutex_);
            boost::weak_ptr<Stock>& wkStock = stocks_[key];
            pStock = wkStock.lock();
            if (!pStock)
            {
                pStock.reset(new Stock(key),
                             boost::bind(&StockFactory::deleteStock,
                                         shared_from_this(),
                                         _1));
                wkStock = pStock;
            }
            return pStock;
        }

    private:

        void deleteStock(Stock* stock)
        {
            printf("deleteStock[%p]\n", stock);
            if (stock)
            {
                muduo::MutexLockGuard lock(mutex_);
                stocks_.erase(stock->key());  // This is wrong, see removeStock below for correct implementation.
            }
            delete stock;  // sorry, I lied
        }
        mutable muduo::MutexLock mutex_;
        std::map<string, boost::weak_ptr<Stock> > stocks_;
    };

}

class StockFactory : public boost::enable_shared_from_this<StockFactory>,
    boost::noncopyable
{
public:
    boost::shared_ptr<Stock> get(const string& key)
    {
        boost::shared_ptr<Stock> pStock;
        muduo::MutexLockGuard lock(mutex_);
        boost::weak_ptr<Stock>& wkStock = stocks_[key];
        pStock = wkStock.lock();
        if (!pStock)
        {
            pStock.reset(new Stock(key),
                         boost::bind(&StockFactory::weakDeleteCallback,
                                     boost::weak_ptr<StockFactory>(shared_from_this()),
                                     _1));
            wkStock = pStock;
        }
        return pStock;
    }

private:
    static void weakDeleteCallback(const boost::weak_ptr<StockFactory>& wkFactory,
                                   Stock* stock)
    {
        printf("weakDeleteStock[%p]\n", stock);
        boost::shared_ptr<StockFactory> factory(wkFactory.lock());
        if (factory)
        {
            factory->removeStock(stock);
        }
        else
        {
            printf("factory died.\n");
        }
        delete stock;  // sorry, I lied
    }

    void removeStock(Stock* stock)
    {
        if (stock)
        {
            muduo::MutexLockGuard lock(mutex_);
            auto it = stocks_.find(stock->key());
            assert(it != stocks_.end());
            if (it->second.expired())
            {
                stocks_.erase(stock->key());
            }
        }
    }

private:
    mutable muduo::MutexLock mutex_;
    std::map<string, boost::weak_ptr<Stock> > stocks_;
};

void testLongLifeFactory()
{
    boost::shared_ptr<StockFactory> factory(new StockFactory);
    {
        boost::shared_ptr<Stock> stock = factory->get("NYSE:IBM");
        boost::shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
        assert(stock == stock2);
        // stock destructs here
    }
    // factory destructs here
}

void testShortLifeFactory()
{
    boost::shared_ptr<Stock> stock;
    {
        boost::shared_ptr<StockFactory> factory(new StockFactory);
        stock = factory->get("NYSE:IBM");
        boost::shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
        assert(stock == stock2);
        // factory destructs here
    }
    // stock destructs here
}

int main()
{
    version1::StockFactory sf1;
    version2::StockFactory sf2;
    version3::StockFactory sf3;

    /* boost::shared_ptr<version3::StockFactory> sf4(new version3::StockFactory); */
    /* boost::shared_ptr<StockFactory> sf5(new StockFactory); */

    {
        boost::shared_ptr<Stock> s2 = sf2.get("stock2");
        //stock一离开作用域,map中存储的股票会析构
    }
    //对象析构之后大小仍然是1
    std::cout << "sf2: size=" <<sf2.getSize() << std::endl;

    {
        boost::shared_ptr<Stock> s1 = sf1.get("stock1");
        //离开作用域也不析构存在内存泄露问题
    }

    /* { */
    /*   std::cout << "search again" << std::endl; */
    /* boost::shared_ptr<Stock> s1 = sf1.get("stock1"); */
    /* } */


    {
        boost::shared_ptr<Stock> s3 = sf3.get("stock3");
    }
    std::cout << "sf3: size=" << sf3.getSize() << std::endl;

    /* { */
    /* boost::shared_ptr<Stock> s4 = sf4->get("stock4"); */
    /* } */

    /* { */
    /* boost::shared_ptr<Stock> s5 = sf5->get("stock5"); */
    /* } */

    /* testLongLifeFactory(); */
    /* testShortLifeFactory(); */

}
