#include "semaphore.hpp"

//Incriments the resources held by the semaphore
//Releases any threads that were waiting for resources
void semaphore::incriment(unsigned newResources){
    //Make sure this is serial only
    mutex_lock l(m_internalMutex);
    m_currentResources += newResources;

    if(m_numberOfThreadsWaiting > 0){
        unsigned numberToRelease = m_currentResources > m_numberOfThreadsWaiting ? m_numberOfThreadsWaiting : m_currentResources;
        m_currentResources -= numberToRelease;

        //We know how many waiters to release, so we can now unlock to let these waiters immediately leave
        l.unlock();

        for(unsigned i = 0; i < numberToRelease; i++)
            m_resourceWaiter.notify_one();

    }
}

//Decreases the number of resources held by the semaphore
//Blocks until the semaphore has a resource to be taken
void semaphore::decriment(){
    mutex_lock l(m_internalMutex);

    if(m_currentResources == 0){
        //There were no resources, 
        m_numberOfThreadsWaiting++;
        m_resourceWaiter.wait(l);
        m_numberOfThreadsWaiting--;
    }
    else{
        m_currentResources -= 1;
    }
}

//Used to prevent any access to the semaphore for a time
void semaphore::blockAccess(){
    m_internalMutex.lock();
}
//Lets other threads access the semaphore again
void semaphore::allowAccess(){
    m_internalMutex.unlock();
}

unsigned semaphore::getResourceNumber()const{
    return m_currentResources;
}
bool semaphore::hasResources()const{
    return m_currentResources > 0;
}

unsigned semaphore::getNumberOfWaitingThreads()const{
    return m_numberOfThreadsWaiting;
}
bool semaphore::hasThreadsWaiting()const{
    return m_numberOfThreadsWaiting > 0;
}


semaphore::semaphore(unsigned startResources):
    m_currentResources(startResources),
    m_numberOfThreadsWaiting(0)
{}

semaphore::~semaphore(){
#ifdef CONCURENT_LIB_THROW_ON_FALIURE
    mutex_lock l(m_internalMutex);
    
    //Don't let a semphore be destroyed if there are still threads waiting on it
    //Its bad form to throw in a destructor, but it is also bad form to delete and active semaphore so pick your poison
    if(m_numberOfThreadsWaiting > 0)
        throw(-1);
#endif
}


