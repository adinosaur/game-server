#include "GEPyEventMgr.h"
#include "GEPython.h"
#include "GELogger.h"

using namespace std;

#define CALLING_EVENT_STACK_MAX 50

GEPyEventMgr::GEPyEventMgr() {
	for (int i = 0; i != EVENT_NUM; i++) {
		evt_cb_functions[i] = new std::vector<PyObject*>();
	}
}

GEPyEventMgr::~GEPyEventMgr() {
	for (int i = 0; i != EVENT_NUM; i++) {
		delete evt_cb_functions[i];
	}
}

bool GEPyEventMgr::reg_event(uint16_t evt, PyObject* pyCallBack_BorrowRef) {
	if (evt >= EVENT_NUM) {
		GELogger::instance()->log("Failed to reg event(%u), event not found.\n", evt);
		return false;
	}
	vector<PyObject*>* evt_functions = this->evt_cb_functions[evt];
	evt_functions->push_back(pyCallBack_BorrowRef);
	Py_INCREF(pyCallBack_BorrowRef);
	return true;
}

bool GEPyEventMgr::trigger_event(uint16_t evt, PyObject* pyParam_BorrowRef) {
	if (evt >= EVENT_NUM) {
		GELogger::instance()->log("Failed to tigger event(%u), event not found.\n", evt);
		return false;
	}
	if (this->calling_evt_set.size() > CALLING_EVENT_STACK_MAX) {
		GELogger::instance()->log("Failed to tigger event(%u), event too much.\n", evt);
		return false;
	}
	if (this->calling_evt_set.find(evt) != this->calling_evt_set.end()) {
		GELogger::instance()->log("Failed to tigger event(%u), event repeat.\n", evt);
		return false;
	}

	this->calling_evt_set.insert(evt);
	
	vector<PyObject*>* cb_functions = this->evt_cb_functions[evt];
	for (vector<PyObject*>::iterator it = cb_functions->begin(); 
		it != cb_functions->end(); ++it) {
		PyObject* pyFunctions_BorrowRef = *it;
		/*
		* Return value: New reference.
		* Call a callable Python object callable, with a variable number of PyObject* arguments.
		* The arguments are provided as a variable number of parameters followed by NULL.
		* Returns the result of the call on success, or NULL on failure.
		*/
		PyObject* pyResult_NewRef = PyObject_CallFunctionObjArgs(pyFunctions_BorrowRef, pyParam_BorrowRef, NULL);
		if (pyResult_NewRef == NULL) {
			GEPython::instance()->show_stack();
		} else {
			Py_DECREF(pyResult_NewRef);
		}
		Py_DECREF(pyParam_BorrowRef);
	}

	this->calling_evt_set.erase(evt);
	return true;
}
