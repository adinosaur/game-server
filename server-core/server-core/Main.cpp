
#ifdef _WIN32
#include <WinSock2.h>
#endif // _WIN32

#include <thread>
#include "GEDefine.h"
#include "GELogger.h"
#include "GESocketServer.h"
#include "GEGameServer.h"
#include "GEPython.h"

using namespace std;

void thread_logger() {
	GELogger::instance()->run();
}

void thread_socket() {
	GESocketServer::instance()->listen(PORT);
	GESocketServer::instance()->run();
}

void thread_gamelogic() {
	//必须在game server线程中初始化python虚拟机
	GEPython::create();
	GEGameServer::instance()->run();
	GEPython::destory();
}

int main(int argc, char **argv) {

#ifdef _WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	GELogger::create();
	GESocketServer::create();
	GEGameServer::create();

	thread* threads[3];
	threads[0] = new thread(thread_logger);
	threads[1] = new thread(thread_socket);
	threads[2] = new thread(thread_gamelogic);

	for (int i = 0; i < 3; i += 1) {
		threads[i]->join();
		delete threads[i];
	}

	GEGameServer::destory();
	GESocketServer::destory();
	GELogger::destory();
	return 0;
}