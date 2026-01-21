#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);
//非const静态变量，在头文件声明，源文件定义，避免多重定义

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        // 生命周期的解耦：Thread 对象的生命周期（作为句柄）和它所创建的后台线程的实际运行生命周期可以是不同的
        thread_->detach(); // thread类提供了设置分离线程的方法 线程运行后自动销毁（非阻塞）
    }
}

//调用start（）的是主线程，start（）中创建的是子线程
void Thread::start()  // 一个Thread对象 记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);                                               // false指的是 不设置进程间共享
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid_ = CurrentThread::tid();  // 查询线程的tid值
        sem_post(&sem);
        func_();  //one loop per thread

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);
}
/**
- 为什么需要semaphore：这里使用信号量作为一个“栅栏”，强制主线程等待子线程初始化完成
因为线程的创建是异步的。当 new std::thread 执行时，主线程继续往下走，而子线程可能还没被操作系统调度执行。
如果此时主线程直接返回，用户去访问 tid_ 成员变量时，可能子线程还没来得及赋值，导致读到 0


 */


// C++ std::thread 中join()和detach()的区别：https://blog.nowcoder.net/n/8fcd9bb6e2e94d9596cf0a45c8e5858a
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}