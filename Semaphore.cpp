#include "Semaphore.hpp"

//Incriments the resources held by the semaphore
//Releases any threads that were waiting for resources
void Semaphore::incriment(unsigned newResources) {
    //Make sure this is serial only
    mutex_lock l(m_internalMutex);
    m_currentResources += newResources;

    if (m_numberOfThreadsWaiting > 0) {
        unsigned numberToRelease = m_currentResources > m_numberOfThreadsWaiting ? m_numberOfThreadsWaiting : m_currentResources;
        m_currentResources -= numberToRelease;

        //We know how many waiters to release, so we can now unlock to let these waiters immediately leave
        l.unlock();

        for (unsigned i = 0; i < numberToRelease; i++)
            m_resourceWaiter.notify_one();

    }
}

//Decreases the number of resources held by the semaphore
//Blocks until the semaphore has a resource to be taken
void Semaphore::decriment() {
    mutex_lock l(m_internalMutex);

    if (m_currentResources == 0) {
        //There were no resources, 
        m_numberOfThreadsWaiting++;
        m_resourceWaiter.wait(l);
        m_numberOfThreadsWaiting--;
    }
    else {
        m_currentResources -= 1;
    }
}

//Used to prevent any access to the semaphore for a time
void Semaphore::blockAccess() {
    m_internalMutex.lock();
}
//Lets other threads access the semaphore again
void Semaphore::allowAccess() {
    m_internalMutex.unlock();
}

unsigned Semaphore::getResourceNumber()const {
    return m_currentResources;
}
bool Semaphore::hasResources()const {
    return m_currentResources > 0;
}

unsigned Semaphore::getNumberOfWaitingThreads()const {
    return m_numberOfThreadsWaiting;
}
bool Semaphore::hasThreadsWaiting()const {
    return m_numberOfThreadsWaiting > 0;
}


Semaphore::Semaphore(unsigned startResources) :
    m_currentResources(startResources),
    m_numberOfThreadsWaiting(0)
{}

CCPP_ERROR(SEMAPHORE_DESTROYED_WHILE_WAITING);

Semaphore::~Semaphore() {
#ifdef _DEBUG
    mutex_lock l(m_internalMutex);

    //Don't let a semphore be destroyed if there are still threads waiting on it
    //Its bad form to throw in a destructor, but it is also bad form to delete and active semaphore so pick your poison
    if (m_numberOfThreadsWaiting > 0)
        throw(new SEMAPHORE_DESTROYED_WHILE_WAITING);
#endif
}
