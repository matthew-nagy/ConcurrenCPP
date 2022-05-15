#pragma once
#include "base.hpp"

class Semaphore {
public:
    //Incriments the resources held by the semaphore
    //Releases any threads that were waiting for resources
    void incriment(unsigned newResources = 1);

    //Decreases the number of resources held by the semaphore
    //Blocks until the semaphore has a resource to be taken
    void decriment();


    //Used to prevent any access to the semaphore for a time
    void blockAccess();
    //Lets other threads access the semaphore again
    void allowAccess();

    unsigned getResourceNumber()const;
    bool hasResources()const;

    unsigned getNumberOfWaitingThreads()const;
    bool hasThreadsWaiting()const;


    Semaphore(unsigned startResources = 0);

    Semaphore(const Semaphore&) = delete;

    ~Semaphore();

private:
    //How many resources a semaphore has. This decreases with each thread let through, but if it is 0, 
    //blocks that thread instead
    unsigned m_currentResources;

    //These two variables handle the blocking and waking up of threads
    mutable std::mutex m_internalMutex;
    std::condition_variable m_resourceWaiter;

    //Tells the semaphore how many times to notify the condition variable before it starts adding to currentResources
    unsigned m_numberOfThreadsWaiting;
};