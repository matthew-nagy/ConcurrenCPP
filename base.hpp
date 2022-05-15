#pragma once


#include <mutex>
#include <condition_variable>
#include <atomic>
#include <exception>

typedef std::unique_lock<std::mutex> mutex_lock;

#define CCPP_ERROR(name) class name : std::runtime_error{public: name():std::runtime_error("An exception was raised by ConcurenCPP"){}}
