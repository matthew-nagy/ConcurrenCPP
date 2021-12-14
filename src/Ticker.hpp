#pragma once

#include "channel.hpp"
#include <chrono>
using namespace std::chrono_literals;

template<class ChanType = bool>
class Ticker{
public:
    single_channel<ChanType>& chan(){
        return m_alertChannel;
    }


    Ticker( const std::chrono::milliseconds& sleepDuration):
        m_period(sleepDuration),
        m_active(new bool(true))
    {
        std::thread([](Ticker* me, bool* active){
            //While this ticker is active
            while(*active){
                //Sleep,
                std::this_thread::sleep_for(me->m_period);

                //Make sure that the ticker isn't already destroyed
                if(*active){
                    //Then tick. First remove something if there is anything, then add something in
                    me->m_alertChannel.tryGet();
                    me->m_alertChannel << ChanType();
                }

            }
            delete active;
        }, this, m_active).detach();
    }

    ~Ticker(){
        //It will be deleted in the different thread
        *m_active = false;
    }

private:
    //This channel will get a signal sent down it whenever the period of time passes
    single_channel<ChanType> m_alertChannel;
    //How many milliseconds will this ticker wait between sending data down the channel (If there isn't any already)
    const std::chrono::milliseconds m_period;
    //A pointer used by the detached thread to control when it should join the main program agin
    bool* m_active;
};
