#include "../src/WorkerPool.hpp"

//Computers an index in the hailstone sequence of a number, and sends the result down a channel
unsigned hailstone5000(unsigned start){
    for(unsigned i = 0; i < 5000; i++)
        if(start % 2 == 0)
            start /= 2;
        else
            start = (start * 3) + 1;
    
    return start;
}
//
unsigned int ackermann(unsigned int m, unsigned int n) {
	if (m == 0) {
		return n + 1;
	}
	if (n == 0) {
		return ackermann(m - 1, 1);
	}
	return ackermann(m - 1, ackermann(m, n - 1));
}
#include <iostream>

int main(){
    printf("Starting\n");
    
    
    auto ackermannPrint = [](unsigned m, unsigned n){printf("\tAkermann of %u,%u is %u\n", m, n, ackermann(m,n));};
    auto hailstonePrint = [](unsigned n){printf("\t5000th hailstone of %u is %u\n", n, hailstone5000(n));};
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    ackermannPrint(4, 1);
    ackermannPrint(4, 1);
    ackermannPrint(2, 5);
    hailstonePrint(27);
    hailstonePrint(164);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto singleThreadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    printf("Duration of singular tests: %ld ms\n", singleThreadDuration);

    WorkerPool pool(4);

    begin = std::chrono::steady_clock::now();

    pool(ackermannPrint, 4, 1);
    pool(ackermannPrint, 4, 1);
    pool(ackermannPrint, 2, 5);
    pool(hailstonePrint, 27);
    pool(hailstonePrint, 164);
    end = std::chrono::steady_clock::now();
    auto workerPushDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    printf("Duration of worker push: %ld ms\n", workerPushDuration);

    pool.waitUntilComplete();

    end = std::chrono::steady_clock::now();
    auto workerPoolDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    printf("Duration of post worker tests: %ld ms\n", workerPoolDuration);
    printf("\n\n>Total Results:\n");
    printf("\tSerial: %ldms\n", singleThreadDuration);
    printf("\tWorker setup: %ldms\n", workerPushDuration);
    printf("\tParallel: %lums\n", workerPoolDuration);

    return 0;
}