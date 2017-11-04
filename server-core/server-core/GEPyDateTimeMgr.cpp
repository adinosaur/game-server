#include "GEPyDateTimeMgr.h"
#include "GEPython.h"
#include "GELogger.h"

using namespace std;

GEPyDateTimeMgr::GEPyDateTimeMgr()
	: cumulation(0)
	, timespeed(1000)
{
	// 计算时区
	this->unixtime = 0;
	this->cache_time();
	if (this->month == 1) {
		this->timezone_second = this->hour * 3600 + this->minute * 60 + this->second - this->dst_second;
	} else {
		this->timezone_second = this->hour * 3600 + this->minute * 60 + this->second - 86400 - this->dst_second;
	}
	// 缓存当前时间
	this->unixtime = static_cast<int32_t>(time(0));
	this->cache_clock();
	this->cache_time();
}

void GEPyDateTimeMgr::update() {
	uint64_t tmp_clock = this->cpuclock;
	cache_clock();
	if (this->cpuclock < tmp_clock) {
		GELogger::instance()->log("Datetime update error, time cycle.\n");
		return;
	}
	// 累加CPU clock
	this->cumulation += (this->cpuclock - tmp_clock);
	if (this->cumulation <= this->timespeed) {
		return;
	}
	this->cumulation -= this->timespeed;
	this->unixtime += 1;
	cache_time();
	// 触发定时事件
	trigger_event();
}

void GEPyDateTimeMgr::cache_clock() {
#ifdef _WIN32
	this->cpuclock = static_cast<uint64_t>(clock());
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	this->cpuclock = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

void GEPyDateTimeMgr::trigger_event() {
	static uint32_t lastSecond = this->second;
	static uint32_t lastMinute = this->minute;
	static uint32_t lastHour = this->hour;
	static uint32_t lastDay = this->day;

	if (lastSecond == this->second) {
		return;
	}
	lastSecond = this->second;
	//计算触发
	bool is_new_min = lastMinute != this->minute;
	bool is_new_hour = false;
	bool is_new_day = false;
	if (is_new_min) {
		lastMinute = this->minute;
		is_new_hour = lastHour != this->hour;
		if (is_new_hour) {
			lastHour = this->hour;
			is_new_day = lastDay != this->day;
			if (is_new_day) {
				lastDay = this->day;
			}
		}
	}
	//触发事件函数调用
	if (is_new_min) {
		if (is_new_hour) {
			if (is_new_day) {
				this->trigger_before_day();
			}
			this->trigger_before_hour();
		}
		this->trigger_before_minute();
	}
	
	this->trigger_per_second();

	if (is_new_min) {
		this->trigger_after_minute();
		if (is_new_hour) {
			this->trigger_after_hour();
			if (is_new_day) {
				this->trigger_after_day();
			}
		}
	}
}

void GEPyDateTimeMgr::cache_time() {
	time_t _tt = static_cast<time_t>(this->unixtime);
	tm* _tm = localtime(&_tt);
	this->year = _tm->tm_year + 1900;
	this->month = _tm->tm_mon + 1;
	this->day = _tm->tm_mday;
	this->hour = _tm->tm_hour;
	this->minute = _tm->tm_min;
	this->second = _tm->tm_sec;
	this->weekday = _tm->tm_wday;
	this->yearday = _tm->tm_yday;
	if (_tm->tm_isdst > 0) {
		this->dst_second = 3600;
	} else {
		this->dst_second = 0;
	}
}

void GEPyDateTimeMgr::reg_function(std::vector<struct datetime_event>& cb_functions, PyObject* pyCallBack_BorrowRef, PyObject* pyParam_BorrowRef) {
	struct datetime_event dv;
	dv.py_callback = pyCallBack_BorrowRef;
	dv.py_param = pyParam_BorrowRef;
	Py_INCREF(dv.py_callback);
	Py_INCREF(dv.py_param);
	cb_functions.push_back(dv);
}

void GEPyDateTimeMgr::trigger_function(std::vector<struct datetime_event>& cb_functions) {
	for (vector<struct datetime_event>::iterator it = cb_functions.begin();
		it != cb_functions.end(); ++it) {
		PyObject* pyFunctions_BorrowRef = it->py_callback;
		PyObject* pyParam_BorrowRef = it->py_param;
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
	}
}