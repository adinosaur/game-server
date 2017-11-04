#include "GEPyMsgMgr.h"
#include "GEPython.h"
#include "GELogger.h"

GEPyMsgMgr::GEPyMsgMgr() {
	for (int i = 0; i != MESSAGE_NUM; i++) {
		this->msg_cb_functions[i] = nullptr;
	}
}

bool GEPyMsgMgr::reg_msg(uint16_t msg, PyObject* pyCallBack_BorrowRef) {
	if (msg >= MESSAGE_NUM) {
		GELogger::instance()->log("Failed to reg msg(%u), msg not found.\n", msg);
		return false;
	}
	if (this->msg_cb_functions[msg] != nullptr) {
		GELogger::instance()->log("Failed to reg msg(%u), repeat msg.\n", msg);
		return false;
	}
	this->msg_cb_functions[msg] = pyCallBack_BorrowRef;
	Py_INCREF(pyCallBack_BorrowRef);
	return true;
}

bool GEPyMsgMgr::trigger_msg(uint16_t msg, uint16_t sock_id, void* data, uint16_t size) {
	if (msg >= MESSAGE_NUM) {
		GELogger::instance()->log("Failed to tigger msg(%u), msg not found.\n", msg);
		return false;
	}

	PyObject* pyFunction_BorrowRef = this->msg_cb_functions[msg];

	/*
	* Return value: New reference.
	* Call a callable Python object callable_object, with arguments given by the tuple args. 
	* If no arguments are needed, then args may be NULL. Returns the result of the call on success, 
	* or NULL on failure. This is the equivalent of the Python expression callable_object(*args).
	*/
	PyObject* pyResult_NewRef = PyObject_CallFunction(pyFunction_BorrowRef, "Hy#", sock_id, data, size);
	if (pyResult_NewRef == NULL) {
		GELogger::instance()->log("Failed to tigger msg(%u), msg cb error.\n", msg);
		GEPython::instance()->show_stack();
		return false;
	} else {
		Py_DECREF(pyResult_NewRef);
		return true;
	}
}