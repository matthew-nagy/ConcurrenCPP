#include "src/select.hpp"
#include "src/Ticker.hpp"
#include <iostream>

channel<char>* producer1 = new single_channel<char>;
channel<char>* producer2 = new single_channel<char>;
channel<char>* signaler = new single_channel<char>;
channel<char>* exitChan = new single_channel<char>;
Ticker<char>* ticker = new Ticker<char>(7000ms); 

enum ChanType{Producer, Signal, Tick, Exit};

int main(){

    *signaler << 'a';
    
    std::thread([&](){
        while(true){
            char a;
            std::cin >> a;
            switch(a){
                case 'p':
                    std::cin >> a;
                    if(a == '1')
                        *producer1 << a;
                    else
                        *producer2 << a;
                    break;
                case 's':
                        a << (*signaler);
                    break;
                default:
                    *exitChan << a;
            }
        }
    }).detach();

    ChanType t = Producer;
    char c;
    printf("Type p and a char to send down a producer (either 1 (send 1), or 2 (send anything else))\n");
    printf("Type s to recieve a signal from a channel\n");
    printf("Type q to quit\n");
    printf("This is all done by waiting in one thread using a select statement\n");
    while(t != Exit){
        t = select<char, ChanType>({
            {Case(c, producer1), Producer},
            {Case(c, producer2), Producer},
            {Case(signaler, c), Signal},
            {Case(ticker), Tick},
            {Case(exitChan), Exit}
        });
        switch(t){
            case Producer:
                printf("Produced from chan %c\n", c);
                break;
            case Signal:
                printf("Passed along signal\n");
                break;
            case Tick:
                printf("7 SECONDS HAVE PASSED\n");
                break;
            case Exit:
                printf("Exit recieved!\n");
        }
    }

    return 0;
}