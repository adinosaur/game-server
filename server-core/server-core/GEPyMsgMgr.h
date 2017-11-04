#pragma once
#include <Python.h>

#define MESSAGE_NUM 200

class GEPyMsgMgr {
public:
	GEPyMsgMgr();
	~GEPyMsgMgr() = default;
	GEPyMsgMgr(const GEPyMsgMgr&) = delete;
	GEPyMsgMgr& operator=(const GEPyMsgMgr&) = delete;

	bool reg_msg(uint16_t msg, PyObject* pyCallBack_BorrowRef);
	bool trigger_msg(uint16_t msg, uint16_t sock_id, void* data, uint16_t size);

private:
	PyObject* msg_cb_functions[MESSAGE_NUM];
};