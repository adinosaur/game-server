#pragma once
#include <Python.h>
#include <stdint.h>
#include <unordered_set>

#define EVENT_NUM 1000

class GEPyEventMgr {
public:
	GEPyEventMgr();
	~GEPyEventMgr();
	GEPyEventMgr(const GEPyEventMgr&) = delete;
	GEPyEventMgr& operator=(const GEPyEventMgr&) = delete;

	bool reg_event(uint16_t evt, PyObject* pyCallBack_BorrowRef);
	bool trigger_event(uint16_t evt, PyObject* pyParam_BorrowRef);

private:
	std::unordered_set<uint16_t> calling_evt_set;
	std::vector<PyObject*>* evt_cb_functions[EVENT_NUM];
};