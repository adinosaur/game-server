#include "GEServiceLog.h"
#include <stdio.h>

GELog::GELog()
	: GEService(SERVICE_LOG) {
}

void 
GELog::dispatch(int type, int session, uint64_t source, const void* msg, uint16_t sz) {
	//TODO
	const char* data = static_cast<const char*>(msg);
	fprintf(stdout, "[LOG] session(%I64u) : %s\n", source, data);
	delete data;
}