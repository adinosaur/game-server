#pragma once
#include <Python.h>

#include <stdint.h>
#include <time.h>
#include <mutex>
#include <forward_list>

#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)

struct timer_event {
	int64_t	tick_id;
	uint32_t timeout;
	bool cancel;
	PyObject* py_callback;
	PyObject* py_param;
};

class GEPyTimerMgr {
public:
	GEPyTimerMgr();
	~GEPyTimerMgr() = default;
	GEPyTimerMgr(const GEPyTimerMgr&) = delete;
	GEPyTimerMgr& operator=(const GEPyTimerMgr&) = delete;

	uint64_t reg_tick(uint32_t timeout, PyObject* pyCallBack_BorrowRef, PyObject* pyParam_BorrowRef);
	void unreg_tick(uint64_t tick_id);
	void update();

private:
	void timer_add(timer_event* ev);
	void timer_update();
	void timer_shift();
	void timer_execute();
	void move_list(int level, int idx);
	std::forward_list<timer_event>*	find_slot(int time);

private:
	std::mutex mtx;
	uint32_t step;			//累加计时器
	uint32_t current_point;	//绝对时间
	std::forward_list<timer_event>	_near[TIME_NEAR];
	std::forward_list<timer_event>	_tv[4][TIME_LEVEL];
};