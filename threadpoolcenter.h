#ifndef THREADPOOLCENTER_H
#define THREADPOOLCENTER_H

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <type_traits>


class ThreadPoolCenter
{
public:
    // 默认获取核心数std::thread::hardware_concurrency()获取核心数
    explicit ThreadPoolCenter(size_t num_threads = std::thread::hardware_concurrency()) : stop(false)
    {
        // 线程初始化即启动，当没有任务condition.wait()使它睡眠，
        // 当被condition.notify_one()唤醒，检查条件通过之后执行，进入下一次循环，如果没有任务就睡觉
        for (size_t i = 0; i < num_threads; ++i)
        {
            // 捕获的this可以理解为一个ThreadPoolCenter *p;
            workers.emplace_back([this] {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        // 当stop为真或者tasks不为空时，才会继续执行
                        this->condition.wait(lock, [this] {return this->stop || !this->tasks.empty();});
                        // 检查退出条件：如果收到停止信号且任务队列为空，线程退出，确保所有任务都完成后再退出线程
                        if (this->stop && this->tasks.empty())
                            return;
                        // 从任务队列中取出任务
                        // 使用move语义避免不必要的拷贝
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    // 尾随返回类型，等价于std::future<std::invoke_result_t<F, Args...>> enqueue(F&& f, Args&&... args)
    // 据说是可读性更好，无所谓咯，这样写就这样写咯
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
    {
        using return_type = std::invoke_result_t<F, Args...>;

        // std::bind(std::forward<F>(f), std::forward<Args>(args)...)将函数f和参数args绑定在一起，创建一个新的可调用对象
        // std::packaged_task<return_type()>包装一个可调用对象，并提供future用于获取结果, return_type() 表示返回return_type且无参数的函数签名
        // std::make_shared<...>(...)在堆上创建对象并返回shared_ptr,因为packaged_task不可拷贝，但需要延长生命周期
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        // task的类型是std::shared_ptr<std::packaged_task<return_type()>>，在这里相当于(*task).，相当于访问std::packaged_task<return_type()>对象成员
        // get_future()是std::packaged_task<return_type()>类的成员方法，用于返回future<return_type()>对象
        // em...怎么说呢，功能越来越多，也越来越复杂，下次尽量不要这样子
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) throw std::runtime_error("Enqueue on stopped ThreadPool");
            // 将任务包装为无参函数
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPoolCenter()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

/**
 * 简单使用方法：ThreadPool pool(4); auto future = pool.enqueue(add, 10, 20); std::cout << "add(10, 20) = " << future.get() << std::endl;
 * 多参数方法：auto future = pool.enqueue([](int a, double b, const std::string& c) {std::cout << "a=" << a << ", b=" << b << ", c=" << c << std::endl; return a + static_cast<int>(b);}
 * , 10, 3.14, "pi");   std::cout << "Result: " << future.get() << std::endl;
 *
**/

#endif // THREADPOOLCENTER_H
