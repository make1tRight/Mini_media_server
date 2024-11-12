#include "mythread.h"

// void *m_arg 
// bool m_isStart 
// bool m_isDetach 
// std::thread m_thread_id 

MyThread::MyThread(): m_arg(nullptr), m_isStart(false), m_isDetach(false) {}

// 实现过程中不要带virtual
MyThread::~MyThread() {
    if (m_isStart == true && m_isDetach == false) {
        detach();
    }
}

// 初始化线程参数, 创建线程并启动
bool MyThread::start(void *arg) {
    m_arg = arg;

    // 创建新的线程, 让线程可执行thread_run函数(this作为参数)
    m_thread_id = std::thread(&MyThread::thread_run, this);

    m_isStart = true;
    return m_isStart;
}

bool MyThread::detach() {
    if (m_isStart != true) {
        return false;
    }
    if (m_isDetach == true) {
        return true;
    }

    // 线程分离, 使得线程在完成任务后自动释放资源
    // 但是主线程也就没办法再控制分离线程, 比如不能用join
    m_thread_id.detach(); 
    m_isDetach = true;

    return m_isDetach;
}

// join确保一个线程等待另一个线程完成后, 自己再执行
bool MyThread::join() {
    // 没有启动的线程, 分离线程都不能调用join
    if (m_isStart != true || m_isDetach == true) {
        return false;
    }

    m_thread_id.join();
    return true;
}

// 静态函数作为线程入口
void *MyThread::thread_run(void *arg) {
    // 强制转换以调用MyThread类的成员
    MyThread *thread = (MyThread*) arg;
    // 调用thread对象的run方法并传入参数
    thread->run(thread->m_arg);
    return nullptr;
}