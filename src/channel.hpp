#pragma once


#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>


//When this flag is on, should a concurrency class notice some deadlock or issue that would occour while
//being deleted, it will throw an error. This is bad form and will set off warnings in most compilers, but
//I view this as better than letting the program continue in an nstable state
//#define CONCURENT_LIB_THROW_ON_FALIURE

//Shorthand for a unique lock
#ifndef CONCURRENT_TYPEDEF
#define CONCURRENT_TYPEDEF
typedef std::unique_lock<std::mutex> mutex_lock;
#endif


//Case will be used for select statements in 'select.hpp'
template<class T>
class Case;

//A golang style channel, that lets data in and out between threads.
//The pure 'channel' class is virtual, as there are unbuffered, buffered, and (not in go) queue channel (unlimited size) implimentations
template<class T>
class channel{
    //A case of this type is a friend, letting it play about with the internals, whithout exposing the inner workings
    friend Case<T>;
public:

    //@param value- is inserted into the channel, blocking until there is room
    friend channel<T>& operator<<(channel<T>& me, T& value){
        mutex_lock l(me.m_internalMutex);
        me.push(value, l);
        return me;
    }
    //@param value- is inserted into the channel, blocking until there is room
    friend channel<T>& operator<<(channel<T>& me, T&& value){
        mutex_lock l(me.m_internalMutex);
        me.push(value, l);
        return me;
    }

    //@param value- assigned a value from the channel, blocking until there is a value to take
    friend channel<T>& operator<<(T& value, channel<T>& me){
        mutex_lock l(me.m_internalMutex);
        value = me.pop(l);
        return me;
    }
    //A value is passes from the channel on the right to the channel on the left
    friend channel<T>& operator<< (channel<T>& left, channel<T>& right){
        mutex_lock ll(left.m_internalMutex);
        mutex_lock rl(left.m_internalMutex);
        left.emplace(right.pop(rl), ll);
        return left;
    }

    //The function attempts to get a value from the channel without the risk of blocking the thread until that is possible
    //@return-
    //          Case 1: the first value being store by this channel, removing it from the channel
    //          Case 2: std::nullopt if there was no value.
    std::optional<T> tryGet(){
        mutex_lock l(m_internalMutex);
        if(hasValue(l))
            return std::optional<T>(pop(l));
        return std::nullopt;
    }

    channel<T>();

    //Copying the channels could cause undefined behaviour based on mutices, condition variables,
    //contents of the channel, so on, so its really best not to allow it
    channel<T>(const channel<T>&) = delete;
    channel<T>(channel<T>&&) = delete;

    virtual ~channel() = default;
protected:

    //@param l will probably not be used, its just there so you remember
    //that the channel is currently locked. The compiler will probably optimize it out anyway.
    virtual bool hasValue(mutex_lock& l) = 0;
    virtual bool canPush(mutex_lock& l) = 0;

    //Virtual functions that, when given a valid lock, will use it to impliment inserting and removing values from the channel
    virtual void push(T& value, mutex_lock& l) = 0;
    virtual T pop(mutex_lock& l) = 0;

    //Blocks the thread using 'l' until m_insertCondition is notified
    void waitOnInsert(mutex_lock& l);
    //Blocks the thread using 'l' until m_removeCondition is notified
    void waitOnRemove(mutex_lock& l);

    //Tells the channel a value was removed
    void notifyRemoval();
    //Tells the channel a value was inserted
    void notifyInsertion();

    //Used for syncing the channel
    std::mutex m_internalMutex;
    std::condition_variable m_insertCondition;
    std::condition_variable m_removeCondition;

    //These values keep track of threads paying attention to the channel
    unsigned m_waitingToRemove;
    unsigned m_waitingToInsert;
};

//Because of the templateness of it, you need to say in derrived classes that you are using it, or else you have to put
//'this->' in front everything. But because this is a lot of boilerplate, lets put it in a handy macro, and undef it at the end of the file!
#define USING_CHANNEL_VARIABLES using channel<T>::m_internalMutex;\
    using channel<T>::m_waitingToRemove;\
    using channel<T>::m_waitingToInsert;\
    using channel<T>::waitOnInsert;\
    using channel<T>::waitOnRemove;\
    using channel<T>::notifyRemoval;\
    using channel<T>::notifyInsertion;

//A channel that can store one value before blocking
template<class T>
class single_channel : public channel<T>{
    USING_CHANNEL_VARIABLES
public:
    single_channel<T>();

    ~single_channel<T>();
private:
    bool hasValue(mutex_lock& l)override;
    bool canPush(mutex_lock& l)override;
    void push(T& value, mutex_lock& l)override;
    T pop(mutex_lock& l)override;

    //Is the value being held currently *in* the channel?
    bool m_hasValue;
    //Value being held to be streamed out later
    T m_value;
};

//A channel that can store N values before blocking
template<class T>
class buffered_channel : public channel<T>{
    USING_CHANNEL_VARIABLES
public:
    buffered_channel<T>(unsigned capacity);

    ~buffered_channel<T>();
private:

    bool hasValue(mutex_lock& l)override;
    bool canPush(mutex_lock& l)override;
    void push(T& value, mutex_lock& l)override;
    T pop(mutex_lock& l)override;

    //The array in memory where values are stored. Treated as a circular list
    T*const m_buffer;
    //How many values maximum can this channel hold (and thus when should it block)
    const unsigned m_maxCapacity;
    //How many values are currently being stored (used to know when to block)
    unsigned m_currentSize;

    //These two variables are used to let m_buffer act as a circular list:

    //Where is the current start of the buffer, which is returned when poping
    unsigned m_startIndex;
    //Where is the current end of the buffer, which is where an inserted value is placed
    unsigned m_endIndex;

    //When a value is popped, the start index must go up towards the end index. But the value it was at must still
    //be known. That is what this function does
    unsigned getAndIncrimentStartIndex();
};

//A channel that can store any number of values, never blocking
template<class T>
class queue_channel : public channel<T>{
    USING_CHANNEL_VARIABLES
public:
    queue_channel<T>();

    ~queue_channel<T>();
private:

    bool hasValue(mutex_lock& l)override;
    bool canPush(mutex_lock& l)override;
    void push(T& value, mutex_lock& l)override;
    T pop(mutex_lock& l)override;

    std::queue<T> m_buffer;
};

#undef USING_CHANNEL_VARIABLES







template<class T>
channel<T>::channel():
    m_waitingToRemove(0),
    m_waitingToInsert(0)
{}

//This entire function is executed with the lock, so no need to worry about race conditions
template<class T>
void channel<T>::waitOnInsert(mutex_lock& l){
    //Let the channel know we are waiting, and then wait
    m_waitingToRemove++;
    m_insertCondition.wait(l);
    //We are no longer waiting
    m_waitingToRemove--;
}
template<class T>
//This entire function is executed with the lock, so no need to worry about race conditions
void channel<T>::waitOnRemove(mutex_lock& l){
    //Let the channel know we are waiting, and then wait
    m_waitingToInsert++;
    m_removeCondition.wait(l);
    //We are no longer waiting
    m_waitingToInsert--;
}

template<class T>
void channel<T>::notifyRemoval(){
    m_removeCondition.notify_one();
}
template<class T>
void channel<T>::notifyInsertion(){
    m_insertCondition.notify_one();
}






template<class T>
bool single_channel<T>::hasValue(mutex_lock& l){
    return m_hasValue;
}
template<class T>
bool single_channel<T>::canPush(mutex_lock& l){
    return m_hasValue == false;
}

template<class T>
void single_channel<T>::push(T& value, mutex_lock& l){
    //if it already has its value, wait for it to be taken out
    if(m_hasValue){
        m_waitingToInsert++;
        waitOnRemove(l);
        m_waitingToInsert--;
    }
    m_hasValue = true;
    m_value = value;

    if(m_waitingToRemove > 0)
        notifyInsertion();
}
template<class T>
T single_channel<T>::pop(mutex_lock& l){
    // if there is no value to remove, wait until there is
    if(!m_hasValue){
        m_waitingToRemove++;
        waitOnInsert(l);
        m_waitingToRemove--;
    }

    if(m_waitingToInsert > 0)
        notifyRemoval();
    
    m_hasValue = false;
    return m_value;
}

template<class T>
single_channel<T>::single_channel():
    channel<T>(),
    m_hasValue(false),
    m_value(T())
{}

template<class T>
single_channel<T>::~single_channel<T>(){
#ifdef CONCURENT_LIB_THROW_ON_FALIURE
    mutex_lock l(m_internalMutex);

    if(m_hasValue)
        throw(-1);
#endif
}



template<class T>
bool buffered_channel<T>::hasValue(mutex_lock& l){
    return m_currentSize > 0;
}
template<class T>
bool buffered_channel<T>::canPush(mutex_lock& l){
    return m_currentSize < m_maxCapacity;
}

template<class T>
void buffered_channel<T>::push(T& value, mutex_lock& l){
    if(m_currentSize == m_maxCapacity){
        waitOnRemove(l);
    }
    m_buffer[m_endIndex] = value;
    m_endIndex++;
    if(m_endIndex == m_maxCapacity)
        m_endIndex = 0;
    m_currentSize++;

    if(m_waitingToRemove > 0)
        notifyInsertion();
}
template<class T>
T buffered_channel<T>::pop(mutex_lock& l){
    if(m_currentSize == 0){
        waitOnInsert(l);
    }

    if(m_waitingToInsert > 0)
        //Notified channel will block again on the internal mutex, so no need
        //to worry about race conditions
        notifyRemoval();
    
    //Size is 1 over indexes, so this works out
    m_currentSize--;
    return m_buffer[getAndIncrimentStartIndex()];
}

template<class T>
buffered_channel<T>::buffered_channel(unsigned capacity):
    channel<T>(),
    m_buffer((T*)malloc(sizeof(T) * capacity)),
    m_currentSize(0),
    m_maxCapacity(capacity),
    m_startIndex(0),
    m_endIndex(0)
{}

template<class T>
buffered_channel<T>::~buffered_channel<T>(){
#ifdef CONCURENT_LIB_THROW_ON_FALIURE
    mutex_lock l(m_internalMutex);

    if(m_currentSize > 0)
        throw(-1);
#endif
    free(m_buffer);
}

template<class T>
unsigned buffered_channel<T>::getAndIncrimentStartIndex(){
    unsigned index = m_startIndex;
    m_startIndex++;
    if(m_startIndex == m_maxCapacity)
        m_startIndex = 0;
    return index;
}





template<class T>
bool queue_channel<T>::hasValue(mutex_lock& l){
    return m_buffer.size() > 0;
}
template<class T>
bool queue_channel<T>::canPush(mutex_lock& l){
    return true;
}

template<class T>
void queue_channel<T>::push(T& value, mutex_lock& l){
    m_buffer.emplace(value);

    if(m_waitingToRemove > 0)
        notifyInsertion();
}
template<class T>
T queue_channel<T>::pop(mutex_lock& l){
    if(m_buffer.size() == 0){
        waitOnInsert(l);
    }
    
    auto temp = std::move(m_buffer.front());
    m_buffer.pop();
    return temp;
}

template<class T>
queue_channel<T>::queue_channel():
    channel<T>()
{}

template<class T>
queue_channel<T>::~queue_channel<T>(){
#ifdef CONCURENT_LIB_THROW_ON_FALIURE
    mutex_lock l(m_internalMutex);

    if(m_buffer.size() > 0)
        throw(-1);
#endif
}

