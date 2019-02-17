#pragma once

#include <string>

bool create_mapping(std::string memmap_name, std::string mutex_name, unsigned long size, void** memmap, void** mutex, void** view, bool only_open);
void release_mapping(void* memmap, void* mutex);
bool lock_mutex(void* mutex);
void unlock_mutex(void* mutex);

class MemoryMapBuffer
{
public:
	MemoryMapBuffer();
	~MemoryMapBuffer();

	bool create(std::string name, int size, bool only_open = true);
	void close();

	std::string mapname();
	void* ptr();

protected:
	std::string _name;
	void* _memmap;
	void* _ptr;
};

template<class T>
class MemoryMapData
{
public:
	MemoryMapData();
	~MemoryMapData();

	bool create(std::string memmap_name, std::string mutex_name, bool only_open = true);
	T& operator() ();
	bool lock();
	void unlock();
	void close();

protected:
	std::string _memmap_name;
	std::string _mutex_name;

	void* _memmap;
	void* _mutex;

	T* _view;
};

template<class T>
inline MemoryMapData<T>::MemoryMapData()
{
	close();
}

template<class T>
inline MemoryMapData<T>::~MemoryMapData()
{
	unlock();
	close();
}

template<class T>
inline bool MemoryMapData<T>::create(std::string memmap_name, std::string mutex_name, bool only_open)
{
	_memmap_name = memmap_name;
	_mutex_name = mutex_name;
	void* ptr = nullptr;
	bool ret = create_mapping(memmap_name, mutex_name, sizeof(T), &_memmap, &_mutex, &ptr, only_open);
	_view = (T*)ptr;
	return ret;
}

template<class T>
inline T & MemoryMapData<T>::operator()()
{
	return *_view;
}

template<class T>
inline bool MemoryMapData<T>::lock()
{
	return lock_mutex(_mutex);
}

template<class T>
inline void MemoryMapData<T>::unlock()
{
	unlock_mutex(_mutex);
}

template<class T>
inline void MemoryMapData<T>::close()
{
	release_mapping(_memmap, _mutex);
	_memmap_name = _mutex_name = "";
	_memmap = _mutex = _view = nullptr;
}
