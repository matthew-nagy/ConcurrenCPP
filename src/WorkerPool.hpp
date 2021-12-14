#pragma once

#include "channel.hpp"
#include "select.hpp"
#include "WaitGroup.hpp"
#include <functional>


class WorkerPool{
    //A task that can be run by a worker
    class Executable_Task{
    public:
        void operator()(){
            func();
        }

        Executable_Task(const std::function<void()>& func):
            func(func)
        {}
    private:
        std::function<void()> func;
    };
public:

    //Adds a new task to the queue to be executed
    template< class Function, class... Args >
    void addTask(Function&& func, Args... arguments){
        m_workComplete.Add();
        m_tasks << new Executable_Task([func, arguments...]{func(arguments...);});
    }

    //Shorthand for calling addTask, adds the given task to the queue to be executed
    template< class Function, class... Args >
    void operator()(Function&& func, Args&&... arguments){
        m_workComplete.Add();
        m_tasks << new Executable_Task([func, arguments...]{func(arguments...);});
    }

    void startWorkers(unsigned number);
    void shutdownWorkers(unsigned number);
    void setWorkers(unsigned number);
    unsigned getNumberOfWorkers()const;

    void waitUntilComplete()const;

    WorkerPool(unsigned startNumber);

    ~WorkerPool();

private:


    queue_channel<Executable_Task*> m_tasks;
    queue_channel<Executable_Task*> m_killSignal;

    WaitGroup m_workerShutdown;
    WaitGroup m_workComplete;

    unsigned m_activeWorkers;

    void startWorker();

    static void work(queue_channel<Executable_Task*>* tasks, queue_channel<Executable_Task*>* killSignal, WaitGroup* completionGroup, WaitGroup* shutdownGroup);
};
