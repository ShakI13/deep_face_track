// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#pragma once

#include <string>

// Shared memory implementation

/**
 * \brief Allocates shared memory and creates access mutex
 * \param memory_map_name shared memory name
 * \param mutex_name access mutex name
 * \param size size of buffer to allocate in bytes
 * \param memory_map pointer that will hold created shared memory handle
 * \param mutex pointer that will hold created mutex handle
 * \param view pointer that will hold allocated area
 * \param only_open tries to only open shared memory if enabled, otherwise create it
 * \return true on success, false on fail
 */
bool create_mapping(std::string memory_map_name, std::string mutex_name, unsigned long size, void** memory_map, void** mutex, void** view, bool only_open);

/**
 * \brief Releases allocated shared memory and mutex
 * \param memory_map pointer to shared memory handle
 * \param mutex pointer to mutex handle
 */
void release_mapping(void* memory_map, void* mutex);

/**
 * \brief Tries to lock mutex for 100 msec and returns
 * \param mutex pointer to mutex handle
 * \return true on success, false on fail
 */
bool lock_mutex(void* mutex);

/**
 * \brief Unlocks previously locked mutex
 * \param mutex pointer to mutex handle
 */
void unlock_mutex(void* mutex);

/*
Helper class to access data buffer via shared memory
*/
class MemoryMapBuffer
{
public:
	MemoryMapBuffer();
	~MemoryMapBuffer();

	/**
	 * \brief Creates shared memory buffer 
	 * \param name shared memory name
	 * \param size buffer size in bytes
	 * \param only_open tries to only open shared memory if enabled, otherwise create it 
	 * \return true on success, false on fail
	 */
	bool create(std::string name, int size, bool only_open = true);
	void close();

	std::string name();
	void* ptr();

protected:
	std::string _name;
	void* _memory_map;
	void* _ptr;
};

/*
Helper class to access data structures via shared memory
*/
template<typename T>
class MemoryMapData
{
public:
	MemoryMapData();
	~MemoryMapData();

	/**
	 * \brief Created shared memory object for type T
	 * \param memory_map_name shared memory name
	 * \param mutex_name mutex name
	 * \param only_open tries to only open shared memory if enabled, otherwise create it  
	 * \return true on success, false on fail 
	 */
	bool create(std::string memory_map_name, std::string mutex_name, bool only_open = true);
	T& operator() ();
	bool lock();
	void unlock();
	void close();

protected:
	std::string _memory_map_name;
	std::string _mutex_name;

	void* _memory_map;
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
inline bool MemoryMapData<T>::create(std::string memory_map_name, std::string mutex_name, bool only_open)
{
	_memory_map_name = memory_map_name;
	_mutex_name = mutex_name;
	void* ptr = nullptr;
	bool ret = create_mapping(memory_map_name, mutex_name, sizeof(T), &_memory_map, &_mutex, &ptr, only_open);
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
	release_mapping(_memory_map, _mutex);
	_memory_map_name = _mutex_name = "";
	_memory_map = _mutex = _view = nullptr;
}
