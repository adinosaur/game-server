#pragma once
#include "GEService.h"

class GELog : public GEService {
public:
	GELog();
	~GELog() = default;

	//virtual void create();
	//virtual void destory();
	virtual void dispatch(int type, int session, uint64_t source, const void* msg, uint16_t sz);
};