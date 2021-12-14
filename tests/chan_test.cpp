#include "../src/channel.hpp"
#include <iostream>
#include "../src/semaphore.hpp"

int main(){

    channel<char>* data_channel;
    channel<char>* output_channel;
    printf("What type of channel are you testing?\n\ts: single\n\tb: buffered(4)\n\tq: queued\n");
    char a;
    std::cin>>a;

    switch(a){
        case 's':
            data_channel = new single_channel<char>;
            output_channel = new single_channel<char>;
            break;
        case 'b':
            data_channel = new buffered_channel<char>(4);
            output_channel = new buffered_channel<char>(4);
            break;
        case 'q':
            data_channel = new queue_channel<char>;
            output_channel = new queue_channel<char>;
            break;
        default:
            printf("'%c' is not a valid choice\n", a);
            return main();
    }

    //Create 4 workers that take a char, and "return" the next letter
    const size_t T_num = 4;
    std::thread threads[T_num];
    semaphore w_sem;
    for(size_t i = 0; i < T_num; i++){
        threads[i] = std::thread([](size_t id, channel<char>& chan, channel<char>& output, semaphore& controlSem){
            printf("Worker %zu has started\n", id);
            controlSem.decriment();
            printf("Worker %zu Past control semaphore\n", id);
            char value;
            value << chan;
            printf("Worker %zu recieved '%c' down channel, sending down response\n", id, value);
            output << (value + 1);
            printf("Worker %zu has finished\n", id);
        }, i, std::ref(*data_channel), std::ref(*output_channel), std::ref(w_sem));
        threads[i].detach();
    }

    //Create a consumer to print out these updated values, once told to
    semaphore c_sem;
    std::thread consumerThread([](channel<char>& outputChan, semaphore& controlSem, size_t numOfThreads){
        controlSem.decriment();
        printf("Consumer thread has been activated\n");

        for(size_t i = 0; i < numOfThreads; i++){
            char output;
            output << outputChan;
            printf("Consumer has recieved output '%c'\n", output);
        }
        printf("Consumer has finished\n");

    }, std::ref(*output_channel), std::ref(c_sem), T_num);
    consumerThread.detach();

    char response = ' ';
    while(response != 'q'){
        std::cin >> response;
        char furtherInput;
        switch(response){
            case 'w':
                w_sem.incriment();
                break;
            case 'c':
                c_sem.incriment();
                break;
            case 'd':
                std::cin>>furtherInput;
                printf("Sending data\n");
                (*data_channel) << furtherInput;
                break;
        }
    }

    delete data_channel;
    delete output_channel;
    return 0;
}