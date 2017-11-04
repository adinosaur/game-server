#pragma once

#include "GEService.h"

class GEGate : public GEService {
public:
	GEGate();
	~GEGate() = default;

	//virtual void create();
	//virtual void destory();
	virtual void dispatch(int type, int session, uint64_t source, const void* msg, uint16_t sz);
};