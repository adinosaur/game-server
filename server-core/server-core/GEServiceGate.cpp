#include "GEServiceGate.h"
#include "GESocketServer.h"
#include "GETimer.h"

#include <stdio.h>
#include <assert.h>

GEGate::GEGate()
	: GEService(SERVICE_GATE) {
}

void GEGate::dispatch(int type, int session, uint64_t source, const void* msg, uint16_t sz) {
	assert(source == SERVICE_SOCKET_SERVER);
	const struct SocketMessage* sm = reinterpret_cast<const struct SocketMessage*>(msg);

	switch (sm->type) {
	case SocketMsgType::Accept:
		this->log("Accept socket(%u)", sm->id);
		break;

	case SocketMsgType::Data:
		printf("%s", sm->data);
		GESocketServer::instance()->send_w(sm->id, sm->data, sz);
		GESocketServer::instance()->close_w(sm->id);
		//delete socket_message->data;
		break;

	case SocketMsgType::Connect:
		break;

	case SocketMsgType::Close:
		this->log("Close socket(%u)", sm->id);
		break;

	case SocketMsgType::Error:
		this->log("Error socket(%u)", sm->id);
		break;

	default:
		this->log("Recv a wrong socket message(%u)", sm->type);
		break;
	}
}