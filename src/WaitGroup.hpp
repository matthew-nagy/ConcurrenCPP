#pragma once

#include<mutex>
#include<condition_variable>

#ifndef CONCURRENT_TYPEDEF
#define CONCURRENT_TYPEDEF
typedef std::unique_lock<std::mutex> mutex_lock;
#endif

class WaitGroup{
public:

    void Add(unsigned amount = 1);

    void Done();

    void Wait()const;

private:

    unsigned m_waiting = 0;

    mutable std::mutex m_internalMutex;
    mutable std::condition_variable m_waitUntilDone;
};