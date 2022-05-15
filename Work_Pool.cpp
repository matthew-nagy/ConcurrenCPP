#include "Work_Pool.hpp"

#include <stdio.h>
struct Job {

	operator bool()const {
		return hasFunction;
	}

	void operator()() {
		myFunction();
	}

	Job() :
		hasFunction(false)
	{}
	Job(void(*func)()) :
		myFunction(func),
		hasFunction(true)
	{}
	Job(std::function<void()> myFunction) :
		myFunction(myFunction),
		hasFunction(true)
	{}

private:
	std::function<void()> myFunction;
	bool hasFunction;
};

void workerPoolJob(Channel<Job*>* jobs, Wait_Group* waitGroup) {
	bool working = true;
	while (working) {
		Job* myJob = jobs->pop();

		if (*myJob)
			(*myJob)();
		else
			working = false;

		waitGroup->finish();

		delete myJob;
	}
}



void Worker_Pool::addTasks(const std::vector<void(*)()>& functions) {
	mutex_lock l(lock);
	waitGroup.add(functions.size());
	for (size_t i = 0; i < functions.size(); i++)
		jobs.emplace(new Job(functions[i]));
}

void Worker_Pool::waitUntilFinished() {
	waitGroup.wait();
}

void Worker_Pool::increaseWorkers(uint16_t newJobs) {
	mutex_lock l(lock);
	activeWorkers += newJobs;
	for (uint16_t i = 0; i < newJobs; i++) {
		std::thread(workerPoolJob, &jobs, &waitGroup).detach();
	}
}

void Worker_Pool::decreaseWorkers(uint16_t killJobs) {
	mutex_lock l(lock);
	activeWorkers -= killJobs;
	waitGroup.add(killJobs);
	for (uint16_t i = 0; i < killJobs; i++)
		jobs.emplace(new Job());
}

Worker_Pool::~Worker_Pool() {
	mutex_lock l(lock);
	waitUntilFinished();

	for (uint16_t i = 0; i < activeWorkers; i++) {
		waitGroup.add();
		jobs.emplace(new Job());
	}

	waitGroup.wait();
}
