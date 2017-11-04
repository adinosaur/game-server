
#include "GEService.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

using namespace std;

#define LOG_MESSAGE_SIZE 256

GEService::GEService(uint64_t service_id)
	: service_id(service_id)
	, in_global(false)
	, mtx()
	, mq() {
	GEServiceMgr::instance()->register_service(service_id, this);
}

void 
GEService::mq_push(const struct ServiceMessage& message) {
	this->mtx.lock();
	this->mq.push_back(message);
	if (!this->in_global) {
		this->in_global = 1;
		GEServiceMgr::instance()->mq_push(this);
	}
	this->mtx.unlock();
}

int 
GEService::mq_pop(struct ServiceMessage& message) {
	int ret = 0;
	this->mtx.lock();
	if (!this->mq.empty()) {
		message = this->mq.front();
		this->mq.pop_front();
		ret = 1;
	}
	this->mtx.unlock();
	return ret;
}

bool 
GEService::mq_empty() {
	bool empty;
	this->mtx.lock();
	empty = this->mq.empty();
	this->mtx.unlock();
	return empty;
}

void 
GEService::log(char* msg, ...) {
	char tmp[LOG_MESSAGE_SIZE];

	va_list ap;
	va_start(ap, msg);
	int len = vsnprintf(tmp, LOG_MESSAGE_SIZE, msg, ap);
	va_end(ap);

	char* data = new char[len];
	assert(data);
	int str_len = strlen(tmp);
	memcpy(data, tmp, str_len);

	struct ServiceMessage m;
	m.type = 0;
	m.session = 0;
	m.source = this->service_id;
	m.sz = 0;
	m.data = reinterpret_cast<void*>(data);

	int ret = GEServiceMgr::instance()->push_message(SERVICE_LOG, m);
	assert(ret);
}

int 
GEServiceMgr::push_message(uint64_t service_id, const struct ServiceMessage& message) {
	int ret = 0;
	std::unordered_map<uint64_t, GEService*>::iterator it = this->services_map.find(service_id);
	if (it != this->services_map.end()) {
		GEService* service = it->second;
		service->mq_push(message);
		ret = 1;
	}
	return ret;
}

GEService* 
GEServiceMgr::mq_pop() {
	GEService* s = nullptr;
	this->mq_mtx.lock();
	if (!this->mq.empty()) {
		s = this->mq.front();
		this->mq.pop_front();
	}
	this->mq_mtx.unlock();
	return s;
}

void 
GEServiceMgr::mq_push(GEService* s) {
	this->mq_mtx.lock();
	this->mq.push_back(s);
	this->mq_cv.notify_all();
	this->mq_mtx.unlock();
}

void 
GEServiceMgr::wait() {
	unique_lock<mutex> lock(this->mq_mtx);
	this->mq_cv.wait(lock);
}

void 
GEServiceMgr::register_service(uint64_t service_id, GEService* service) {
	this->services_map.insert(make_pair(service_id, service));
}