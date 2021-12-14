#include "WaitGroup.hpp"


void WaitGroup::Add(unsigned amount){
    mutex_lock l(m_internalMutex);
    m_waiting += amount;
}

void WaitGroup::Done(){
    mutex_lock l(m_internalMutex);
    m_waiting -= 1;
    if(m_waiting == 0)
        m_waitUntilDone.notify_all();
}

void WaitGroup::Wait()const{
    mutex_lock l(m_internalMutex);
    if(m_waiting != 0)
        m_waitUntilDone.wait(l);
}