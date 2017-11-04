#include <Python.h>
#include <stdint.h>
#include "GEGameServer.h"
#include "GESocketServer.h"
#include "GELogger.h"
#include "GEDefine.h"

static PyObject* reg_msg(PyObject* self, PyObject* arg) {
	uint16_t msg_type = 0;
	PyObject* pyfun_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "HO", &msg_type, &pyfun_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->msg_mgr.reg_msg(msg_type, pyfun_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_tick(PyObject* self, PyObject* arg) {
	uint16_t timeout = 0;
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyargs_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "HOO", &timeout, &pyfun_BorrowRef, &pyargs_BorrowRef)) {
		return NULL;
	}
	uint64_t uTickID = GEGameServer::instance()->timer_mgr.reg_tick(timeout, pyfun_BorrowRef, pyargs_BorrowRef);
	return PyLong_FromUnsignedLongLong(uTickID);
}

static PyObject* unreg_tick(PyObject* self, PyObject* arg) {
	uint64_t tick_id = 0;
	if (!PyArg_ParseTuple(arg, "K", &tick_id)) {
		return NULL;
	}
	GEGameServer::instance()->timer_mgr.unreg_tick(tick_id);
	Py_RETURN_NONE;
}

static PyObject* reg_event(PyObject* self, PyObject* arg) {
	uint16_t evt = 0;
	PyObject* pyfun_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "HO", &evt, &pyfun_BorrowRef)) {
		return NULL;
	}
	uint64_t uTickID = GEGameServer::instance()->event_mgr.reg_event(evt, pyfun_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* trigger_event(PyObject* self, PyObject* arg) {
	uint16_t evt = 0;
	PyObject* pyParam_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "HO", &evt, &pyParam_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->event_mgr.trigger_event(evt, pyParam_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_per_second(PyObject* self, PyObject* arg) {
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyarg_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "OO", &pyfun_BorrowRef, &pyarg_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->datetime_mgr.reg_per_second(pyfun_BorrowRef, pyarg_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_before_minute(PyObject* self, PyObject* arg) {
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyarg_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "OO", &pyfun_BorrowRef, &pyarg_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->datetime_mgr.reg_before_minute(pyfun_BorrowRef, pyarg_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_after_minute(PyObject* self, PyObject* arg) {
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyarg_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "OO", &pyfun_BorrowRef, &pyarg_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->datetime_mgr.reg_after_minute(pyfun_BorrowRef, pyarg_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_before_hour(PyObject* self, PyObject* arg) {
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyarg_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "OO", &pyfun_BorrowRef, &pyarg_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->datetime_mgr.reg_before_hour(pyfun_BorrowRef, pyarg_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_after_hour(PyObject* self, PyObject* arg) {
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyarg_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "OO", &pyfun_BorrowRef, &pyarg_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->datetime_mgr.reg_after_hour(pyfun_BorrowRef, pyarg_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_before_day(PyObject* self, PyObject* arg) {
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyarg_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "OO", &pyfun_BorrowRef, &pyarg_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->datetime_mgr.reg_before_day(pyfun_BorrowRef, pyarg_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* reg_after_day(PyObject* self, PyObject* arg) {
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyarg_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(arg, "OO", &pyfun_BorrowRef, &pyarg_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->datetime_mgr.reg_after_day(pyfun_BorrowRef, pyarg_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* send_obj(PyObject* self, PyObject* args)
{
	uint16_t sock_id = UINT16_MAX;
	uint16_t msg_type = CMsgType::Ping;
	PyObject* pyobj_BorrowRef = Py_None;
	if (!PyArg_ParseTuple(args, "HHO", &sock_id, &msg_type, &pyobj_BorrowRef)) {
		return NULL;
	}
	GEGameServer::instance()->send_obj(sock_id, msg_type, pyobj_BorrowRef);
	Py_RETURN_NONE;
}

static PyObject* send_obj_and_back(PyObject* self, PyObject* args)
{
	uint16_t sock_id = UINT16_MAX;
	uint16_t msg_type = CMsgType::Ping;
	PyObject* pyobj_BorrowRef = Py_None;
	uint32_t timeout;
	PyObject* pyfun_BorrowRef = Py_None;
	PyObject* pyargs_BorrowRef = Py_None;

	if (!PyArg_ParseTuple(args, "HHOOOI", &sock_id, &msg_type, &pyobj_BorrowRef, &pyfun_BorrowRef, &pyargs_BorrowRef, &timeout)) {
		return NULL;
	}
	GEGameServer::instance()->send_obj_and_back(sock_id, msg_type, pyobj_BorrowRef, pyfun_BorrowRef, pyargs_BorrowRef, timeout);
	Py_RETURN_NONE;
}

static PyObject* kick(PyObject* self, PyObject* args)
{
	uint16_t sock_id = UINT16_MAX;

	if (!PyArg_ParseTuple(args, "H", &sock_id)) {
		return NULL;
	}
	GEGameServer::instance()->kick(sock_id);
	Py_RETURN_NONE;
}

static PyObject* log(PyObject* self, PyObject* args)
{
	char* log_message = NULL;
	if (!PyArg_ParseTuple(args, "s", &log_message)) {
		return NULL;
	}
	GELogger::instance()->log(log_message);
	Py_RETURN_NONE;
}

static PyMethodDef PyGameServer_Methods[] = {
	{ "reg_msg",			reg_msg,			METH_VARARGS, "注册消息处理函数" },
	{ "reg_tick",			reg_tick,			METH_VARARGS, "注册定时触发处理函数" },
	{ "unreg_tick",			unreg_tick,			METH_VARARGS, "取消注册定时触发处理函数" },
	{ "reg_event",			reg_event,			METH_VARARGS, "注册事件触发处理函数" },
	{ "trigger_event",		trigger_event,		METH_VARARGS, "触发事件" },
	{ "reg_per_second",		reg_per_second,		METH_VARARGS, "注册每秒触发处理函数" },
	{ "reg_before_minute",	reg_before_minute,	METH_VARARGS, "注册在每分钟之前触发处理函数" },
	{ "reg_after_minute",	reg_after_minute,	METH_VARARGS, "注册在每分钟之后触发处理函数" },
	{ "reg_before_hour",	reg_before_hour,	METH_VARARGS, "注册在每小时之前触发处理函数" },
	{ "reg_after_hour",		reg_after_hour,		METH_VARARGS, "注册在每小时之后触发处理函数" },
	{ "reg_before_day",		reg_before_day,		METH_VARARGS, "注册在每天之前触发处理函数" },
	{ "reg_after_day",		reg_after_day,		METH_VARARGS, "注册在每天之后触发处理函数" },
	{ "send_obj",			send_obj,			METH_VARARGS, "发送消息函数" },
	{ "send_obj_and_back",	send_obj_and_back,	METH_VARARGS, "发送消息并且回调函数" },
	{ "kick",				kick,				METH_VARARGS, "关闭连接" },
	{ "log",				log,				METH_VARARGS, "打印日志" },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef PyGameServer_ModuleDef = {
	PyModuleDef_HEAD_INIT,
	"PyGameServer",
	NULL,
	-1,
	PyGameServer_Methods,
};

PyMODINIT_FUNC
PyInit_PyGameServer(void) {
	return PyModule_Create(&PyGameServer_ModuleDef);
}