#include "Wait_Group.hpp"

//Adds one more unit to wait for
void Wait_Group::add() {
	mutex_lock l(myLock);
	units += 1;
}
//Adds multiple units to wait for
void Wait_Group::add(uint32_t additionalUnits) {
	mutex_lock l(myLock);
	units += additionalUnits;
}
//Tells the group some unit is finished
void Wait_Group::finish() {
	mutex_lock l(myLock);
	units -= 1;
	if (units == 0)
		groupWaiter.notify_all();
}
//Block the thread until the job is done
void Wait_Group::wait() {
	mutex_lock l(myLock);
	if (units > 0)
		groupWaiter.wait(l);
}

uint32_t Wait_Group::activeUnits() {
	mutex_lock l(myLock);
	return units;
}