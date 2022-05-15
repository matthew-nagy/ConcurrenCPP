#pragma once

#include "base.hpp"

class Wait_Group {
public:
	//Adds one more unit to wait for
	void add();
	//Adds multiple units to wait for
	void add(uint32_t additionalUnits);
	//Tells the group some unit is finished
	void finish();
	//Block the thread until the job is done
	void wait();

	uint32_t activeUnits();

private:
	uint32_t units;
	std::mutex myLock;
	std::condition_variable groupWaiter;
};