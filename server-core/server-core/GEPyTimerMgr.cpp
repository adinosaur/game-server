
#include <time.h>
#include "GEPyTimerMgr.h"
#include "GEPython.h"

using namespace std;

GEPyTimerMgr::GEPyTimerMgr()
	: step(0) 
{
	this->current_point = static_cast<uint32_t>(time(NULL));
}


uint64_t GEPyTimerMgr::reg_tick(uint32_t timeout, PyObject* pyCallBack_BorrowRef, PyObject* pyParam_BorrowRef) {
	static uint32_t cnt = 0;
	uint32_t t = ++cnt;
	uint64_t tick_id = static_cast<uint64_t>(timeout) << 32 | t;
	
	struct timer_event ev;
	ev.tick_id = tick_id;
	ev.timeout = this->step + timeout;
	ev.cancel = false;
	ev.py_callback = pyCallBack_BorrowRef;
	ev.py_param = pyParam_BorrowRef;
	// 增加Python的引用计数
	Py_INCREF(ev.py_callback);
	Py_INCREF(ev.py_param);

	timer_add(&ev);
	return tick_id;
}

void GEPyTimerMgr::unreg_tick(uint64_t tick_id) {
	uint32_t timeout = tick_id >> 32;
	std::forward_list<timer_event>* list = find_slot(timeout);
	for (std::forward_list<timer_event>::iterator it = list->begin(); 
		it != list->end(); ++it) {
		if (it->tick_id != tick_id)
			continue;
		it->cancel = true;
		// 减少引用计数
		Py_DECREF(it->py_callback);
		Py_DECREF(it->py_param);
		return;
	}
}

void GEPyTimerMgr::update() {
	uint32_t cp = static_cast<uint32_t>(time(NULL));
	if (cp < this->current_point) {
		this->current_point = cp;
	} else if (cp != this->current_point) {
		uint32_t diff = (uint32_t)(cp - this->current_point);
		this->current_point = cp;
		for (uint32_t i = 0; i < diff; i++) {
			timer_update();
		}
	}
}

std::forward_list<timer_event>* GEPyTimerMgr::find_slot(int time) {
	uint32_t current_time = this->step;
	if ((time | TIME_NEAR_MASK) == (current_time | TIME_NEAR_MASK)) {
		return &this->_near[time&TIME_NEAR_MASK];
	} else {
		int i;
		uint32_t mask = TIME_NEAR << TIME_LEVEL_SHIFT;
		for (i = 0; i < 3; i++) {
			if ((time | (mask - 1)) == (current_time | (mask - 1))) {
				break;
			}
			mask <<= TIME_LEVEL_SHIFT;
		}
		return &this->_tv[i][((time >> (TIME_NEAR_SHIFT + i*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)];
	}
}

void GEPyTimerMgr::timer_add(timer_event* ev) {
	uint32_t time = ev->timeout;
	this->mtx.lock();
	std::forward_list<timer_event>* list = find_slot(time);
	list->push_front(*ev);
	this->mtx.unlock();
}

void GEPyTimerMgr::timer_update() {
	this->mtx.lock();
	timer_shift();
	timer_execute();
	this->mtx.unlock();
}

void GEPyTimerMgr::timer_shift() {
	int mask = TIME_NEAR;
	uint32_t ct = ++this->step;
	if (ct == 0) {
		move_list(3, 0);
	} else {
		uint32_t time = ct >> TIME_NEAR_SHIFT;
		int i = 0;
		while ((ct & (mask - 1)) == 0) {
			int idx = time & TIME_LEVEL_MASK;
			if (idx != 0) {
				move_list(i, idx);
				break;
			}
			mask <<= TIME_LEVEL_SHIFT;
			time >>= TIME_LEVEL_SHIFT;
			++i;
		}
	}
}

static void dispatch(struct timer_event* ev) {
	if (ev->cancel) {
		return;
	}
	/*
	* Return value: New reference.
	* Call a callable Python object callable, with a variable number of PyObject* arguments. 
	* The arguments are provided as a variable number of parameters followed by NULL. 
	* Returns the result of the call on success, or NULL on failure.
	*/
	PyObject* pyResult_NewRef = PyObject_CallFunctionObjArgs(ev->py_callback, ev->py_param, NULL);
	if (pyResult_NewRef == NULL) {
		GEPython::instance()->show_stack();
	} else {
		Py_DECREF(pyResult_NewRef);
	}
	// 减少引用计数
	Py_DECREF(ev->py_callback);
	Py_DECREF(ev->py_param);
}

void GEPyTimerMgr::timer_execute() {
	int idx = this->step & TIME_NEAR_MASK;
	std::forward_list<timer_event> list;
	list.swap(this->_near[idx]);
	
	for (std::forward_list<timer_event>::iterator it = list.begin(); 
		it != list.end(); ++it) {
		this->mtx.unlock();
		// dispatch_list don't need lock
		dispatch(&(*it));
		this->mtx.lock();
	}
}

void GEPyTimerMgr::move_list(int level, int idx) {
	std::forward_list<timer_event>* list = &this->_tv[level][idx];
	timer_event ev;
	while (!list->empty()) {
		ev = list->front();
		timer_add(&ev);
		list->pop_front();
	}
}