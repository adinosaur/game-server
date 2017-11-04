
#include <assert.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/thread.h>

#include "GEDefine.h"
#include "GESocketServer.h"
#include "GEGameServer.h"
#include "GELogger.h"

using namespace std;

#define HEADER_SZ sizeof(uint32_t)

struct net_message {
	uint16_t size;
	uint16_t type;
	void* data;
};

struct connection {
	uint16_t sock_id;
	evutil_socket_t sock_fd;
	struct bufferevent* bufev;
	bool is_waiting_header;
	net_message current_message;
};

static void readcb(struct bufferevent *bev, void *user_data) {
	struct connection* conn = static_cast<struct connection*>(user_data);
	struct evbuffer* inbuf = bufferevent_get_input(bev);

	for (;;) {
		size_t data_sz = evbuffer_get_length(inbuf);
		
		if (conn->is_waiting_header) {
			size_t expect_sz = HEADER_SZ;
			if (data_sz < expect_sz) {
				return;
			} else {
				//完整接收完一个header
				size_t read_sz = bufferevent_read(bev, &conn->current_message, HEADER_SZ);
				assert(read_sz == HEADER_SZ);
				if (conn->current_message.size > 0) {
					conn->is_waiting_header = false;
					conn->current_message.data = new uint8_t[conn->current_message.size];
					assert(conn->current_message.data);
				} else {
					//部分消息仅有消息头
					GEGameServer::instance()->socket_data(conn->sock_id, conn->current_message.type, nullptr, 0);
					conn->is_waiting_header = true;
					conn->current_message.data = nullptr;
				}
			}
			
		} else {
			size_t expect_sz = conn->current_message.size;
			if (data_sz < expect_sz) {
				return;
			} else {
				//完整接收完一个message
				size_t read_sz = bufferevent_read(bev, conn->current_message.data, conn->current_message.size);
				assert(read_sz == conn->current_message.size);
				uint16_t sz = static_cast<uint16_t>(read_sz);

				GEGameServer::instance()->socket_data(conn->sock_id, conn->current_message.type, conn->current_message.data, conn->current_message.size);
				conn->is_waiting_header = true;
				conn->current_message.data = nullptr;
			}
			
		}
	}
}

static void eventcb(struct bufferevent *bev, short events, void *user_data) {
	//遇到EOF或者ERROR均关闭Connection
	struct connection* conn = static_cast<struct connection*>(user_data);
	GESocketServer::instance()->close(conn->sock_id);

	if (events & BEV_EVENT_EOF) {
		GEGameServer::instance()->socket_close(conn->sock_id);
	} else if (events & BEV_EVENT_ERROR) {
		GEGameServer::instance()->socket_error(conn->sock_id);
	}
}

static void listenercb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data) {
	GESocketServer* ss = static_cast<GESocketServer*>(user_data);
	assert(ss != nullptr);

	static uint16_t alloc_id = 0;
	struct connection* conn = new struct connection;
	assert(conn);
	conn->sock_id = ++alloc_id;
	conn->sock_fd = fd;
	conn->is_waiting_header = true;
	conn->current_message.data = nullptr;

	evutil_make_socket_nonblocking(fd);
	struct bufferevent* bufev = bufferevent_socket_new(ss->ev_base, fd, BEV_OPT_CLOSE_ON_FREE);
	assert(bufev);
	bufferevent_setcb(bufev, readcb, NULL, eventcb, conn);
	bufferevent_enable(bufev, EV_WRITE | EV_READ);
	conn->bufev = bufev;

	//插入字典需要加锁
	ss->mtx.lock();
	ss->data_sock_map.insert(make_pair(conn->sock_id, conn));
	ss->mtx.unlock();

	GEGameServer::instance()->socket_accept(conn->sock_id);
}

GESocketServer::GESocketServer()
	: lst_port(0)
	, ev_base(nullptr)
	, ev_listener(nullptr)
	, data_sock_map()
{
	//Functions for multi-threaded applications using Libevent.
	//When using a multi - threaded application in which multiple threads add and 
	//delete events from a single event base, Libevent needs to lock its data structures.
	//Like the memory - management function hooks, all of the threading functions must be 
	//set up before an event_base is created if you want the base to use them.
	//Most programs will either be using Windows threads or Posix threads.
	//You can configure Libevent to use one of these event_use_windows_threads() or event_use_pthreads() respectively.
	//If you're using another threading library, you'll need to configure threading functions manually 
	//using evthread_set_lock_callbacks() and evthread_set_condition_callbacks().
#ifdef _WIN32
	evthread_use_windows_threads();
#else
	event_use_pthreads();
#endif

	this->ev_base = event_base_new();
	assert(this->ev_base);
}

GESocketServer::~GESocketServer() {
	event_base_free(this->ev_base);
	evconnlistener_free(this->ev_listener);
}

void GESocketServer::listen(uint16_t port) {
	this->lst_port = port;
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0);
	sin.sin_port = htons(port);

	this->ev_listener = evconnlistener_new_bind(this->ev_base, listenercb, this, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&sin, sizeof(sin));
	assert(this->ev_listener != NULL);
}

void GESocketServer::run() {
	event_base_dispatch(this->ev_base);
}

void GESocketServer::close(uint16_t id) {
	//从字典中查找需要加锁
	this->mtx.lock();
	unordered_map<uint16_t, struct connection*>::iterator it;
	unordered_map<uint16_t, struct connection*>::iterator end = this->data_sock_map.end();
	it = this->data_sock_map.find(id);
	this->mtx.unlock();

	if (it == end) {
		return;
	}

	//从字典中删除需要加锁
	struct connection* conn = it->second;
	this->mtx.lock();
	this->data_sock_map.erase(it);
	this->mtx.unlock();

	//释放socket对应的bufferevent对象
	bufferevent_free(conn->bufev);
	//如果socket有消息接收缓冲区，也释放这部分的内存
	if (conn->current_message.data != nullptr) {
		delete conn->current_message.data;
	}
}

void GESocketServer::send(uint16_t id, uint16_t type, void* data, uint16_t size) {
	//从字典中查找需要加锁
	this->mtx.lock();
	unordered_map<uint16_t, struct connection*>::iterator it;
	unordered_map<uint16_t, struct connection*>::iterator end = this->data_sock_map.end();
	it = this->data_sock_map.find(id);
	this->mtx.unlock();
	
	if (it == end) {
		return;
	}
	
	struct connection* conn = it->second;
	assert(conn != nullptr);

	bufferevent_write(conn->bufev, &size, sizeof(size));
	bufferevent_write(conn->bufev, &type, sizeof(type));
	if (size > 0) {
		assert(data != nullptr);
		bufferevent_write(conn->bufev, data, size);
	}

	//Make sure it's safe to tell an event base to wake up from another thread or a signal handler.
	evthread_make_base_notifiable(this->ev_base);
	delete data;
}