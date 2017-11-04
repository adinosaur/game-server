
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "GELogger.h"

using namespace std;

#define LOG_MESSAGE_SIZE 256

void GELogger::run() {
	for (;;) {
		unique_lock<mutex> lock(this->mtx);
		if (this->mq.empty()) {
			this->cv.wait(lock);
		} else {
			struct log_message m = this->mq.front();
			this->mq.pop_front();
			fprintf(stdout, "[LOG] %s", m.data);
			delete m.data;
		}
	}
}

void GELogger::log(char* msg, ...) {
	assert(msg != nullptr);

	char tmp[LOG_MESSAGE_SIZE];

	va_list ap;
	va_start(ap, msg);
	int len = vsnprintf(tmp, LOG_MESSAGE_SIZE, msg, ap);
	va_end(ap);

	char* data = new char[len + 1];
	assert(data);
	memcpy(data, tmp, len +1);

	struct log_message m;
	m.data = data;

	unique_lock<mutex> lock(this->mtx);
	this->mq.push_back(m);
	this->cv.notify_all();
}