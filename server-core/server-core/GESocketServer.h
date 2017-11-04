#pragma once

#include <stdint.h>

#include <mutex>
#include <unordered_map>

#include <event2/util.h> // for evutil_socket_t

#include "GESingleton.h"

struct event_base;
struct evconnlistener;
struct bufferevent;

struct connection;

class GESocketServer : public GESingleton<GESocketServer> {

	friend void readcb(struct bufferevent*, void*);
	friend void eventcb(struct bufferevent*, short, void*);
	friend void listenercb(struct evconnlistener*, evutil_socket_t, struct sockaddr *, int, void*);

public:
	GESocketServer();
	~GESocketServer();

	void listen(uint16_t port);
	void run();
	void close(uint16_t id);
	void send(uint16_t id, uint16_t type, void* data, uint16_t size);

private:
	uint16_t lst_port;
	struct event_base* ev_base;
	struct evconnlistener* ev_listener;

	std::mutex mtx;
	std::unordered_map<uint16_t, struct connection*> data_sock_map;
};