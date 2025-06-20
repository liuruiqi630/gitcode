#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
class ThreadPool
{
    public:
    //线程池构造函数
    ThreadPool(int numThreads):stop(false)
    {
        //为线程组添加指定个数的线程
        for(int i=0;i<numThreads;i++)
        {
            threads.emplace_back([this]{//lembda表达式
                //线程的实际操作
                while(1)
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    //lock是unique_lock 类型
                    condition.wait(lock,[this]{
                        //lembda表达式 this捕获外部类即ThreadPool false则阻塞
                        return !tasks.empty()||stop;}
                    );//任务队列不为空再继续线程
                    if(stop&&tasks.empty())
                    {
                        return;
                    }
                    //执行任务
                    std::function<void()> task(std::move(tasks.front()));//这里传入的参数有什么用呢？
                    tasks.pop();
                    lock.unlock();
                    task();  
                }
            });
        }
    }
    //析构
    ~ThreadPool()
    {
        {
        std::unique_lock<std::mutex> lock(mtx);
        stop=true;
        }
        //在析构时保证队列中的任务全部完成
        condition.notify_all();//通知全部线程不再阻塞
        for(auto&t:threads)
        {
            t.join();
        }
    }
    //任务函数
    template<class F,class...Args>//参数扩展
    //任务队列添加任务
    void enqueue(F&& f,Args&&... args)//右值引用+函数模板=万能引用
    {
        std::function<void()> task=
            std::bind(std::forward<F>(f),std::forward<Args>(args)...);
            /*bind函数适配器用于固定一些参数以减少传入的参数量
            bind返回是_Binder类型再转化为function
            当函数绑定参数后，void（）里不需要传参
            forward完美转化器用于保留传入参数的左右值特性避免拷贝*/
            {
            std::unique_lock<std::mutex> lock(mtx);
            //对资源都需要互斥访问
            tasks.emplace(std::move(task));
            }
            condition.notify_one();
    }
    private:
    bool stop;
    std::mutex mtx;
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> threads;
    std::condition_variable condition;
};
int main()
{
    ThreadPool pool(4);
    for(int i=0;i<10;i++)
    {
        pool.enqueue([i]{//传入lembda表达式
            std::cout<<"完成任务"<<i<<std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout<<"结束任务"<<i<<std::endl;
        });
    }
}