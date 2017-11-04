#include <Python.h>
#include <chrono>
#include "GEGameServer.h"
#include "GESocketServer.h"
#include "GELogger.h"
#include "GEDefine.h"

using namespace std;

enum SocketStatus {
	Accept = 1,
	Data,
	Connect,
	Close,
	Error,
};

struct socket_message {
	uint16_t sock_id;
	SocketStatus status;
	void* data;
};

static void message_distribute(struct socket_message* sm) {
	struct game_message* gm = reinterpret_cast<struct game_message*>(sm->data);
	switch (gm->type) {
	case CMsgType::Ping:
		GESocketServer::instance()->send(sm->sock_id, CMsgType::PingReply, gm->data, gm->size);
		break;

	default:
		GEGameServer::instance()->msg_mgr.trigger_msg(gm->type, sm->sock_id, gm->data, gm->size);
		break;
	}
	delete gm;
}

void GEGameServer::run() {
	for (;;) {
		//10ms的超时返回
		unique_lock<mutex> lock(this->mtx);
		for (; this->socket_message_queue.empty();) {
			if (this->cv.wait_until(lock, chrono::system_clock::now() + chrono::milliseconds(10)) == cv_status::timeout) {
				break;
			}
		}

		this->gstate = PyGILState_Ensure();
		if (!this->socket_message_queue.empty()) {
			//有消息返回
			struct socket_message* sm = this->socket_message_queue.front();
			this->socket_message_queue.pop_front();
			switch (sm->status) {
			case SocketStatus::Accept:
				GEGameServer::instance()->event_mgr.trigger_event(SocketEvent_Accept, PyLong_FromLong(sm->sock_id));
				break;

			case SocketStatus::Connect:
				GEGameServer::instance()->event_mgr.trigger_event(SocketEvent_Connect, PyLong_FromLong(sm->sock_id));
				break;

			case SocketStatus::Close:
				GEGameServer::instance()->event_mgr.trigger_event(SocketEvent_Close, PyLong_FromLong(sm->sock_id));
				break;

			case SocketStatus::Error:
				GEGameServer::instance()->event_mgr.trigger_event(SocketEvent_Error, PyLong_FromLong(sm->sock_id));
				break;

			case SocketStatus::Data:
				GEGameServer::instance()->event_mgr.trigger_event(SocketEvent_Data, PyLong_FromLong(sm->sock_id));
				message_distribute(sm);
				break;
			}
			delete sm;
		} else {
			//超时返回
		}
		this->timer_mgr.update();
		this->datetime_mgr.update();
		PyGILState_Release(this->gstate);
	}
}

void GEGameServer::socket_accept(uint16_t sock_id) {
	struct socket_message* sm = new struct socket_message;
	sm->sock_id = sock_id;
	sm->status = SocketStatus::Accept;
	sm->data = nullptr;
	socket_message_push(sm);
}

void GEGameServer::socket_data(uint16_t sock_id, uint16_t type, void* data, uint16_t size) {
	struct game_message* gm = new game_message;
	gm->sock_id = sock_id;
	gm->size = size;
	gm->type = type;
	gm->data = data;

	struct socket_message* sm = new struct socket_message;
	sm->sock_id = sock_id;
	sm->status = SocketStatus::Data;
	sm->data = gm;
	socket_message_push(sm);
}

void GEGameServer::socket_connect(uint16_t sock_id) {
	struct socket_message* sm = new struct socket_message;
	sm->sock_id = sock_id;
	sm->status = SocketStatus::Connect;
	sm->data = nullptr;
	socket_message_push(sm);
}

void GEGameServer::socket_close(uint16_t sock_id) {
	struct socket_message* sm = new struct socket_message;
	sm->sock_id = sock_id;
	sm->status = SocketStatus::Close;
	sm->data = nullptr;
	socket_message_push(sm);
}

void GEGameServer::socket_error(uint16_t sock_id) {
	struct socket_message* sm = new struct socket_message;
	sm->sock_id = sock_id;
	sm->status = SocketStatus::Error;
	sm->data = nullptr;
	socket_message_push(sm);
}

void GEGameServer::socket_message_push(struct socket_message* m) {
	unique_lock<mutex> lock(this->mtx);
	this->socket_message_queue.push_back(m);
	this->cv.notify_all();
}

void GEGameServer::send_obj(uint16_t sock_id, uint16_t msg_type, PyObject* pyobj_BorrowRef) {
	assert(pyobj_BorrowRef != nullptr);
	if (pyobj_BorrowRef == Py_None) {
		GESocketServer::instance()->send(sock_id, msg_type, nullptr, 0);
		return;
	} else {
		Py_ssize_t size = PyBytes_Size(pyobj_BorrowRef);
		assert(size > 0);

		/*
		* Return a pointer to the contents of o. The pointer refers to the internal buffer of o,
		* which consists of len(o) + 1 bytes. The last byte in the buffer is always null,
		* regardless of whether there are any other null bytes. The data must not be modified in any way,
		* unless the object was just created using PyBytes_FromStringAndSize(NULL, size).
		* It must not be deallocated. If o is not a bytes object at all, PyBytes_AsString() returns NULL and raises TypeError.
		*/
		char* data = nullptr;
		const char* tmp = PyBytes_AsString(pyobj_BorrowRef);
		if (tmp == NULL) {
			GELogger::instance()->log("Failed to send msg(%u).\n", msg_type);
			return;
		} else {
			data = new char[size];
			memcpy(data, tmp, size);
		}
		GESocketServer::instance()->send(sock_id, msg_type, data, size);
	}
}

void GEGameServer::send_obj_and_back(uint16_t sock_id, uint16_t msg_type, PyObject* pyobj_BorrowRef, PyObject* pycb_BorrowRef, PyObject* pyparam_BorrowRef, uint32_t timeout) {
	send_obj(sock_id, msg_type, pyobj_BorrowRef);
	this->timer_mgr.reg_tick(timeout, pycb_BorrowRef, pyparam_BorrowRef);
}

void GEGameServer::kick(uint16_t sock_id) {
	GESocketServer::instance()->close(sock_id);
}