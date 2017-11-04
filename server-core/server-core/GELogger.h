#pragma once

#include <list>
#include <mutex>
#include <condition_variable>

#include "GESingleton.h"

struct log_message {
	const char* data;
};

class GELogger : public GESingleton<GELogger> {
public:
	void run();
	void log(char* msg, ...);

private:
	std::mutex mtx;
	std::condition_variable cv;
	std::list<struct log_message> mq;
};