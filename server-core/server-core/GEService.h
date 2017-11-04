#pragma once

#include "GESingleton.h"

#include <stdint.h>
#include <list>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

#define SER_MSG_TYPE_NORMAL 0

#define SERVICE_TIMER			1
#define SERVICE_SOCKET_SERVER	2
#define SERVICE_LOG				3
#define SERVICE_GATE			4

struct ServiceMessage {
	int type;
	int session;
	uint64_t source;
	uint16_t sz;
	void* data;
};

class GEService {
public:
	GEService(uint64_t service_id);
	~GEService() = default;

	virtual void create() {}
	virtual void destory() {}
	virtual void dispatch(int type, int session, uint64_t source, const void* msg, uint16_t sz) {}

	void mq_push(const struct ServiceMessage& message);
	// 1 for success
	int mq_pop(struct ServiceMessage& message);
	bool mq_empty();

	void log(char* msg, ...);

public:
	uint64_t service_id;
	bool in_global;

private:
	std::mutex mtx;
	std::list<struct ServiceMessage> mq;
};

class GEServiceMgr : public GESingleton<GEServiceMgr> {
public:
	// 1 for success
	int push_message(uint64_t service_id, const struct ServiceMessage& message);
	void register_service(uint64_t service_id, GEService* service);

	GEService* mq_pop();
	void mq_push(GEService* s);

	void wait();

private:
	std::mutex mq_mtx;
	std::condition_variable mq_cv;
	std::list<GEService*> mq;
	std::unordered_map<uint64_t, GEService*> services_map;
};