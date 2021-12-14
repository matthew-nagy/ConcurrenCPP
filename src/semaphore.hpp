#pragma once

#include <mutex>
#include <condition_variable>

//When this flag is on, should a concurrency class notice some deadlock or issue that would occour while
//being deleted, it will throw an error. This is bad form and will set off warnings in most compilers, but
//I view this as better than letting the program continue in an nstable state
//#define CONCURENT_LIB_THROW_ON_FALIURE

#ifndef CONCURRENT_TYPEDEF
#define CONCURRENT_TYPEDEF
typedef std::unique_lock<std::mutex> mutex_lock;
#endif

class semaphore{
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


    semaphore(unsigned startResources = 0);

    semaphore(const semaphore&) = delete;

    ~semaphore();

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