#pragma once

#include "base.hpp"
#include "Channel.hpp"
#include "Wait_Group.hpp"
#include <functional>
#include <thread>

struct Job;

class Worker_Pool {
public:

	//Adds a new task to the queue to be executed
	template< class Function, class... Args >
	void addTask(Function&& func, Args... arguments) {
		mutex_lock l(lock);
		waitGroup.add();
		jobs.emplace(new Job([func, arguments...]{ func(arguments...); }));
	}

	void addTasks(const std::vector<void(*)()>& functions);

	void waitUntilFinished();

	void increaseWorkers(uint16_t newJobs);

	void decreaseWorkers(uint16_t killJobs);

	uint16_t getWorkerNumber() {
		mutex_lock l(lock);
		return activeWorkers;
	}

	~Worker_Pool();

private:

	uint16_t activeWorkers = 0;
	Wait_Group waitGroup;
	Channel<Job*> jobs;

	std::mutex lock;
};