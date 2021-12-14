# ConcurenCPP

**Intro:** The point of this library was to bring many of the parallel-focused language features I found so useful in golang into C++. I then added a few extentions I felt nicely complemented them. Of course, the name is a pun off c++ and concurrency
The go features are:
 - channels (buffered and unbuffered)
 - select statement with default
 - Tickers
 - WaitGroups

Other features I added:
 - Queue channels
 - semaphore (c++17 implimentation)
 - A worker pool

 ## Golang features

**Channels** have been implimented, with buffered, unbuffered versions available. You have to give them the type and size at decleration.
You can interact with them via the << operation, like how the arrows work in go. Also, a **queued** version is included which can take in any number of inputs, storing them internally in a queue.

**select** statements with cases and default statement are also available, creating a powerful tool for blocking and synchronising workers. The downside to this implimentation as opposed to go, is that all channels must work with the same underlying type (int, float, etc), and that this variable must be defined beforehand. You can differentiate between which channel was used through the ReturnType template.

An example of how to use these features are below, and other examples are available in the tests folder 
```c++
//Create a channel of ints that can store 4 values before blocking
channel<int>* bufferedIntChan = new buffered_channel<int>(4);

//Create a channel that can only hold one string or int value
channel<std::string>* stringChan = new single_channel<std::string>;
channel<int>* exitChan = new single_channel<int>;

enum ChannelType{ct_PrintInt, ct_End};

//Create a thread that waits for something to print out, then sends 6 ints down a channel
std::thread([](channel<std::string>& inChan, channel<int>& outChan, channel<int>& exitChan){
    std::string printOut;
    printOut << inChan;
    std::cout<< printOut << std::endl;
    for(int i = 0; i < 6; i++)
        outChan << i;
    exitChan << 0;
}, std::ref(*stringChan), std::ref(*buferedIntChan), std::ref(*exitChan)).detatch();

(*stringChan) << "Worker has started!";
ChannelType lastChannel = ct_PrintInt;
while(lastChannel != ct_End){
    //A select statement using int channels, differentiated by ChannelType 
    int chanValue;
    lastChannel = select<int, ChannelType>({
        {Case(chanValue, bufferedIntChan), ct_PrintInt},
        {Case(exitChan), ct_End}
        //{Default<int>(), ct_PrintInt} 
        //, this would trigger whenever neither other case is ready
    });
    switch(lastChannel){
        case ct_PrintInt:   //If the int channel send a value, print it
            std::cout<< "Recieved: " << chanValue << std::endl; 
            break;
        case ct_End:        //If anything went down exitChan, finish
            std::cout<< "Recieved signal to exit" << std::endl;
    }
}
//Free channels or whatever else you would like to do

```
In the future I plan to add a more generic channel, which can wrap any other channel as a channel of std::any. This will give more fleability in select statements

**WorkGroups** and **tickers** have also been added with near-if-not identical syntax to go. You can use WaitGroups to atomically add tasks that should be waited for and signal for them to be done, while other threads wait for completion. You can use Tickers to get a regular pulse down a channel, to help synchronise workers. Examples of both of these 
can be found in the tests folder

## Note on code warnings

In the makefile and through the code you will see a macro called CONCURENT_LIB_THROW_ON_FALIURE. If a channel or semaphore is deleted while it is in use, it could have wild consequences for the correctness of your program.
Therefore I added in a throw statement in the destructors if this is the case.
However, as this is bad form (arguably not as bad as deleting in-use condition variables and mutexes...) and will make some compilers throw warnings, I have hidden this behind said macro.
Having this macro defined will let the destructors throw, removing them will mean the code shall not warn you should you make a horrific error. I leave this choice to the users

## To Do
- [X] Impliment select statements
- [X] Impliment default in select
- [X] make and test worker pool
- [ ] make generic std::any wrapper for channels in select
- [ ] Look into promises and async to see if there are any utilities that could be added