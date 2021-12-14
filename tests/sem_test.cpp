
#include <thread>
#include <iostream>
#include <stdio.h>
#include "../src/channel.hpp"
#include "../src/semaphore.hpp"

int main(){
    semaphore t(1);
    single_channel<char> chan;

    std::thread threads[2];
    for(size_t i = 0; i < 2; i++){
        threads[i] = std::thread( [&](size_t id, channel<char>& chan){
            printf("Starting thread %zu\n", id);
            t.decriment();

            char a;
            printf("Waiting for %zu to get a char\n", id);
            a << chan;

            printf("%zu recieved %c from channel, adding value to semaphore\n", id, a);
            t.incriment();

        } , i, std::ref(chan));
    }

    for(size_t i = 0; i < 2; i++){
        char z;
        std::cin >> z;
        printf("Sending char down channel\n");
        chan << z;
    }

    for(size_t i = 0; i < 2; i++)
        threads[i].join();
    return 0;
}