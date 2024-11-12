#include "threadpool.h"
#include "../log/log.h"

Threadpool *Threadpool::append(int num) {
    return new Threadpool(num);
}

//不能隐式转换成其他的类
Threadpool::Threadpool(int num): m_threads(num), m_quit(false) {
    create_threads();
}

Threadpool::~Threadpool() {
    cancel_threads();
}

void Threadpool::add_task(Threadpool::MyTask &task) {
    std::unique_lock<std::mutex> ulock(m_queuelocker);
    m_workqueue.push(task);
    m_queuestat.notify_one(); //唤醒一个在等待任务队列中的任务到来的线程
}

void Threadpool::loop() {
    while (!m_quit) {
        // 加锁, ulock离开作用域自动解锁
        // RAII, loop函数末尾或条件变量被唤醒 -> 自动解锁
        std::unique_lock<std::mutex> ulock(m_queuelocker);
        if (m_workqueue.empty()) {  //任务队列为空
            m_queuestat.wait(ulock);//当前线程阻塞, 其他线程通知当前线程有任务到来
        }
        // 条件变量的wait方法唤醒线程后不能保证一定有任务可以处理, 可能被其他线程先处理了
        // 所以这里再加一次, 检查队列是否真的有任务可以handle
        if (m_workqueue.empty()) {
            continue;
        }

        MyTask task = m_workqueue.front();
        m_workqueue.pop();

        task.handle();
    }
}

void Threadpool::create_threads() {
    std::unique_lock<std::mutex> ulock(m_queuelocker);
    for (auto &m_thread : m_threads) {
        LOGINFO("create m_thread");
        m_thread.start(this);
    }
}

void Threadpool::cancel_threads() {
    std::unique_lock<std::mutex> ulock(m_queuelocker);
    m_quit = true;
    m_queuestat.notify_all();           //通知所有线程
    for (auto &m_thread : m_threads) {  //等待所有工作线程完成, 防止资源泄漏
        m_thread.join();
    }

    m_threads.clear();                  //所有工作线程处理完毕, 清空线程容器, 释放相关资源
}

void Threadpool::MediaThread::run(void *arg) { //MediaThread直接就执行loop这里没明白
    Threadpool *threadpool = (Threadpool*) arg;
    threadpool->loop();
}