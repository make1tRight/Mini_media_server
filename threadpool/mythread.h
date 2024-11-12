#ifndef MYTHREAD_H
#define MYTHREAD_H
#include <thread>
class MyThread {
public:
    // 指向派生类对象的时候由派生类的析构函数销毁自己的对象
    // 如果是基类的析构函数来删除, 派生类很可能有基类所没有的对象, 造成资源泄漏
    virtual ~MyThread();

    bool start(void *arg);
    bool detach();
    bool join();

protected:
    MyThread();
    // 每个派生类都必须有自己的run实现逻辑
    // 根据不同需求创建功能不同的线程
    virtual void run(void *arg) = 0;

private:
    // static使其作为线程的入口函数
    static void *thread_run(void *);

private:
    void *m_arg;
    bool m_isStart;
    bool m_isDetach;
    std::thread m_thread_id;
};
#endif //MYTHREAD_H