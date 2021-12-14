#pragma once

#include "channel.hpp"
#include "Ticker.hpp"
#include "semaphore.hpp"
#include <vector>
#include <initializer_list>

template<class T>
class Case{
public:
    void assignRelevent(bool* flag){
        *flag = true;
        m_stillInSelect = flag;
    }

    //To be called when there is a default statement
    bool tryExecute(){
        if(m_type == ct_Default)
            return true;
        mutex_lock l(m_channel->m_internalMutex);
        return doTryExecute(l);
    }

    //Called when there is no default statement
    void execute(std::shared_ptr<std::mutex> selectMutex, std::vector<bool*>& relevancies, semaphore& finishSem, size_t myID, size_t& successID){
        selectMutex->lock();
        if(!m_stillInSelect.isValid()){
            selectMutex->unlock();
            return;
        }
        mutex_lock l(m_channel->m_internalMutex);
        bool success;
        switch(m_type){
            case ct_Insert:
                //If the insertion didn't happen, don't tell the select statement its all over
                if(!insertExecute(l, selectMutex))
                    return;
                break;
            //Either pop need mostly the same thing, so generalise it into one case
            case ct_PopAssign:
            case ct_PopIgnore:
                //If we try to pop but its not needed anyore, return and do nothing
                if(!generalPopExecute(l, selectMutex))
                    return;

                if(m_type == ct_PopAssign)
                    *m_value = m_channel->pop(l);
                else
                    m_channel->pop(l);
                break;
            case ct_Default:
                //It should never have got here with a default, tryExecute should have long stopped it
                throw(-1);
        }
        
        l.unlock();
        //Successfully taken action, announce to all other parties they are no longer needed
        //And let the select statement move forwards

        for(bool* flag : relevancies)
            *flag = false;
        successID = myID;
        selectMutex->unlock();
        finishSem.incriment();
    }

    //@param channel will have the value of into inserted into it
    //@param into will be copied into the given channel
    //Golang equiv: channel <- into
    Case(channel<T>* channel, T& into):
        m_channel(channel),
        m_value(&into),
        m_type(ct_Insert)
    {}
    //@param assign will be assigned from the channel
    //@param channel will have a value removed to put into 'assign'
    //Golang equiv: into <- channel
    Case(T& assign, channel<T>* channel):
        m_type(ct_PopAssign),
        m_channel(channel),
        m_value(&assign)
    {}
    //@param channel shall have a value removed from it and forgotten
    //Golang equiv: <-channel
    Case(channel<T>* channel):
        m_channel(channel),
        m_type(ct_PopIgnore)
    {}

    //@param channel will have the value of into inserted into it
    //@param into will be copied into the given channel
    //Golang equiv: channel <- into
    Case(channel<T>& channel, T& into):
        Case(&channel, into)
    {}
    //@param assign will be assigned from the channel
    //@param channel will have a value removed to put into 'assign'
    //Golang equiv: into <- channel
    Case(T& assign, channel<T>& channel):
        Case(assign, &channel)
    {}
    //@param channel shall have a value removed from it and forgotten
    //Golang equiv: <-channel
    Case(channel<T>& channel):
        Case(&channel)
    {}
    
    //@param ticker shall be listened too, and this case will pass when the ticker signals that its elsapsed time has passed
    Case(Ticker<T>& ticker):
        Case(&ticker.chan())
    {}
    Case(Ticker<T>* ticker):
        Case(ticker->chan())
    {}

    Case():
        m_type(ct_Default)
    {}
private:
    //These objects are used to know if this case is still relevent; is the seletc statement this case belongs too still active?
    class relevancy_flag{
    public:
        bool isValid()const{
            return *m_isRelevent;
        }

        relevancy_flag& operator=(bool* isRelevent){
            m_isRelevent = isRelevent;
            return *this;
        }

        relevancy_flag() = default;

        ~relevancy_flag(){
            if(m_isRelevent != nullptr)
                delete m_isRelevent;
        }
    private:
        bool* m_isRelevent = nullptr;
    };

    enum CaseType{ct_Insert, ct_PopAssign, ct_PopIgnore, ct_Default} m_type;

    //Some memory held by this case that the select statement will change when it ends
    //This way, once a case wakes up from checking the channel, it will be able to check if the select statement is over
    relevancy_flag m_stillInSelect;

    //Pointer to channel it is going to use in the case
    channel<T>* m_channel;
    //Pointer to the value it could be assigning or inserting
    T* m_value;

    //Attempts to pop a value from the target channel, returning true if it succeeded
    bool doTryExecute(mutex_lock& l){
        switch(m_type){
            case ct_Insert:
                if(m_channel->canPush(l)){
                    m_channel->push(*m_value, l);
                    return true;
                }
                return false;
            case ct_PopAssign:
                if(m_channel->hasValue(l)){
                    *m_value = m_channel->pop(l);
                    return true;
                }
                return false;
            case ct_PopIgnore:
                if(m_channel->hasValue(l))
                    return true;
                return false;
            case ct_Default:
                throw(-1);
                //Never should have got here, should have been caught before getting to this function
        }
    }


    bool insertExecute(mutex_lock& l, std::shared_ptr<std::mutex>& selectMutex){
        bool success = m_channel->canPush(l);
        if(!success){
            //Let other cases move on
            selectMutex->unlock();
            //Wait until a value has been removed so we can continue
            m_channel->waitOnRemove(l);
            selectMutex->lock();
            
            //Out of the select statement, its over
            if(!m_stillInSelect.isValid()){
                //We aren't using this "removal" and so say we removed something again
                m_channel->notifyRemoval();
                l.unlock();

                selectMutex->unlock();
                return false;
            }
        }

        m_channel->push(*m_value, l);
        return true;
    }

    bool generalPopExecute(mutex_lock& l, std::shared_ptr<std::mutex>& selectMutex){
        bool success = m_channel->hasValue(l);
        if(!success){
            //Let other cases move on
            selectMutex->unlock();
            //Wait until a value has been added so we can continue
            m_channel->waitOnInsert(l);
            selectMutex->lock();
            
            //Out of the select statement, its over
            if(!m_stillInSelect.isValid()){
                //We aren't using this "addition" and so say we added something again
                m_channel->notifyInsertion();
                l.unlock();

                selectMutex->unlock();
                return false;
            }
        }
        return true;
    }

};

#define Default Case


//Hide the select class, we only want the function so we can switch statment off it
namespace hidden{
    //Analagous to the select statements in go, though with slightly different syntax
    //Also, due to the implimentation, will only work on channels of the same type
    template<class T, class ReturnType>
    class Select{
        typedef std::initializer_list<std::pair<const Case<T>&, const ReturnType&>> Parameters;
    public:

        ReturnType resolve(){
            for(auto& p : m_cases){
                if(p.first->tryExecute()){
                    return p.second;
                }
            }

            m_internalMutex->lock();
            size_t successfulID;
            for(size_t i = 0; i < m_cases.size(); i++){
                std::thread([](std::shared_ptr<std::mutex> selectMutex, std::vector<bool*>& relevancies, semaphore& finishSem, size_t myID, size_t& successID, Case<T>* runCase){
                    runCase->execute(selectMutex, relevancies, finishSem, myID, successID);
                    delete runCase;
                }, m_internalMutex, std::ref(m_inSelectFlags), std::ref(m_blockingSemaphore), i, std::ref(successfulID), m_cases[i].first).detach();
            }
            m_internalMutex->unlock();

            m_blockingSemaphore.decriment();
            return m_cases[successfulID].second;
        }


        Select(const Parameters& params):
            m_blockingSemaphore(0),
            m_internalMutex(std::make_shared<std::mutex>())    
        {
            m_cases.reserve(params.size());
            m_inSelectFlags.reserve(params.size());

            size_t i = 0;
            for(auto& p : params){
                m_cases.emplace_back(new Case<T>(p.first), p.second);
                bool* inSelectFlag = new bool(true);
                m_cases[i].first->assignRelevent(inSelectFlag);
                m_inSelectFlags.emplace_back(inSelectFlag);

                i++;
            }
        }


    private:
        std::vector<std::pair<Case<T>*, ReturnType>> m_cases;
        std::vector<bool*> m_inSelectFlags;

        semaphore m_blockingSemaphore;
        std::shared_ptr<std::mutex> m_internalMutex;
    };
}

template<class T, class ReturnType>
ReturnType select(const std::initializer_list<std::pair<const Case<T>&, const ReturnType&>>& params){
    return hidden::Select<T, ReturnType>(params).resolve();
}

