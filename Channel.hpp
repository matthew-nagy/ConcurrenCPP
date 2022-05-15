#pragma once

#include "Semaphore.hpp"
#include <optional>
#include <queue>

CCPP_ERROR(CHANNEL_DESTROYED_WITH_VALUE);
CCPP_ERROR(CHANNEL_DESTROYED_WITH_WAITING_THREADS);

template<class T>
class Channel {
public:

	template<class ...Args>
	void emplace(Args&&... arguments) {
		mutex_lock l(internalMutex);
		if (!canPush(l)) {
			onPop.wait(l);
		}
		internalEmplace(l, std::forward<Args>(arguments)...);
	}
	template<class ...Args>
	bool tryEmplace(Args&&... arguments) {
		mutex_lock l(internalMutex);
		if (!canPush(l)) 
			return false;

		internalEmplace(l, arguments);
		return true;
	}
	
	T pop() {
		mutex_lock l(internalMutex);
		if (!canPop(l))
			onPush.wait(l);
		
		return internalPop(l);
	}
	std::optional<T> tryPop() {
		mutex_lock l(internalMutex);
		if (!canPop(l)) 
			return {};
		
		return internalPop(l);
	}

	T peek() {
		mutex_lock l(internalMutex);
		if (!canPop(l))
			onPush.wait(l);

		return internalPeek(l);
	}
	T tryPeek() {
		mutex_lock l(internalMutex);
		if (!canPop(l))
			return {};

		return internalPeek(l);
	}

	//Makes a one sized queue
	Channel():
		maxSize(1)
	{}

	//Makes a @param queueSize sized internal queue
	Channel(size_t queueSize):
		maxSize(queueSize)
	{}

private:
	std::queue<T> internalQueue;
	const size_t maxSize;

	std::mutex internalMutex;
	std::condition_variable onPush, onPop;

	bool canPush(mutex_lock& lock)const {
		return internalQueue.size() < maxSize;
	}
	bool canPop(mutex_lock& lock)const {
		return internalQueue.size() > 0;
	}

	template<class ...Args>
	void internalEmplace(mutex_lock& lock, Args&&... arguments) {
		internalQueue.emplace(std::forward<Args>(arguments)...);
		onPush.notify_one();
	}
	T internalPop(mutex_lock& lock) {
		T value = internalQueue.front();
		internalQueue.pop();
		onPop.notify_one();
		return value;
	}
	T internalPeek(mutex_lock& lock) {
		T value = internalQueue.front();
		//"Put it back" so any other threads waiting on this can run
		onPush.notify_one();
		return value;
	}
};