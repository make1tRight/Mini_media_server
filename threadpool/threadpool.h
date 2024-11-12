#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <mutex>
#include <queue>
#include <condition_variable>
#include "mythread.h"

// struct MyTask { //任务处理回调函数
// public:
//     // 函数指针类型, 接受void*参数, 返回void的函数
//     typedef void (*TaskCallback)(void*);

//     MyTask(): m_task_callback(nullptr), myArg(nullptr) {}

//     void set_task_callback(TaskCallback cb, void* arg) {
//         m_task_callback = cb;
//         myArg = arg;
//     }

//     void handle() {
//         if (m_task_callback) { //回调函数不为空则调用
//             m_task_callback(myArg);
//         }
//     }

//     bool operator=(const MyTask &task) {
//         this->m_task_callback = task.m_task_callback;
//         this->myArg = task.myArg;
//     }

// private:
//     TaskCallback m_task_callback;
//     void* myArg;
// };


class Threadpool {
public:
    static Threadpool *append(int num);
    explicit Threadpool(int num); //不能隐式转换成其他的类
    ~Threadpool();

    // void add_task(MyTask &task);

    struct MyTask { //任务处理回调函数
    public:
        // 函数指针类型, 接受void*参数, 返回void的函数
        typedef void (*TaskCallback)(void*);

        MyTask(): m_task_callback(nullptr), myArg(nullptr) {}

        void set_task_callback(TaskCallback cb, void* arg) {
            m_task_callback = cb;
            myArg = arg;
        }

        void handle() {
            if (m_task_callback) { //回调函数不为空则调用
                m_task_callback(myArg);
            }
        }

        bool operator=(const MyTask &task) {
            this->m_task_callback = task.m_task_callback;
            this->myArg = task.myArg;
        }

    private:
        TaskCallback m_task_callback;
        void* myArg;
    };

    void add_task(MyTask &task);

private:
    void loop();
    void create_threads();
    void cancel_threads();

    class MediaThread : public MyThread {
    protected:
        virtual void run(void *arg);
    };

private:
    std::queue<MyTask> m_workqueue;
    std::mutex m_queuelocker;
    std::condition_variable m_queuestat;

    std::vector<MediaThread> m_threads;

    bool m_quit;
};
#endif //THREADPOOL_H