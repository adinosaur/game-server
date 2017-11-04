#pragma once

template<class T> class GESingleton {
public:
	static void	create() { ins = new T; }
	static void	destory() { if (ins) { delete ins; ins = nullptr; }; }
	static T* instance() { return ins; }

protected:
	GESingleton() { ins = nullptr; }
	~GESingleton() {}

protected:
	static T* ins;
};

template<class T> T* GESingleton<T>::ins;