#include "WorkerPool.hpp"



void WorkerPool::startWorkers(unsigned number){
    for(unsigned i = 0; i < number; i++){
        startWorker();
        m_activeWorkers++;
    }
}
void WorkerPool::shutdownWorkers(unsigned number){
    for(unsigned i = 0; i < number; i++){
        m_killSignal << nullptr;
        m_activeWorkers--;
    }
}
void WorkerPool::setWorkers(unsigned number){
    if(number > m_activeWorkers)
        startWorkers(number - m_activeWorkers);
    else
        shutdownWorkers(m_activeWorkers - number);
}
unsigned WorkerPool::getNumberOfWorkers()const{
    return m_activeWorkers;
}

void WorkerPool::waitUntilComplete()const{
    m_workComplete.Wait();
}

WorkerPool::WorkerPool(unsigned startNumber):
    m_activeWorkers(startNumber)
{
    for(unsigned i = 0; i < startNumber; i++)
        startWorker();
}

WorkerPool::~WorkerPool(){
    shutdownWorkers(m_activeWorkers);
    m_workerShutdown.Wait();

    //Flush all of the cases
    m_tasks << nullptr;

    //Get rid of all tasks before shutting down
    std::optional<Executable_Task*> t = m_tasks.tryGet();
    while(t != std::nullopt){
        delete t.value();
        t = m_tasks.tryGet();
    }
}



void WorkerPool::startWorker(){
    m_workerShutdown.Add();
    std::thread(WorkerPool::work, &m_tasks, &m_killSignal, &m_workComplete, &m_workerShutdown).detach();
}

void WorkerPool::work(queue_channel<Executable_Task*>* tasks, queue_channel<Executable_Task*>* killSignal, WaitGroup* completionGroup, WaitGroup* shutdownGroup){
    bool active = true;
    Executable_Task* myTask;
    while(active){

        //Wait for the signal
        select<Executable_Task*, int>({
            {Case(myTask, *tasks), 0},
            {Case(myTask, *killSignal), 0}
        });

        //Shutdown signal
        if(myTask == nullptr){
            shutdownGroup->Done();
            active = false;
        }
        //Otherwise perform the task, say its done, then free the memory
        else{
            (*myTask)();
            completionGroup->Done();
            delete myTask;
        }

    }
}