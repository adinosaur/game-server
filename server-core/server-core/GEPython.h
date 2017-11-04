#pragma once

#include <Python.h>
#include "GESingleton.h"

class GEPython : public GESingleton<GEPython> {

public:
	GEPython();
	~GEPython();
	
	//向logger打印堆栈
	void show_stack();
	//插入path目录
	void insert_path(uint32_t uPos, const char* sPath);
	//直接执行一段python代码
	bool exec(const char* code);
	//从python模块执行一个函数
	bool call_module_function(const char* sModuleName, const char* sFunctionName);
};