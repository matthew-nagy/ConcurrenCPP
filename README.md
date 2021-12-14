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