#include "memory_map_data.h"

#include <Windows.h>

bool create_mapping(std::string memmap_name, std::string mutex_name, unsigned long size, void ** memmap, void ** mutex, void ** view, bool only_open)
{
	HANDLE _memmap = 0;
	HANDLE _mutex = 0;
	LPVOID _view = 0;

	_memmap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, memmap_name.c_str());
	if (_memmap == nullptr && ! only_open)
	{
		_memmap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, memmap_name.c_str());
	}
	if (_memmap != 0) 
	{
		_view = MapViewOfFile(_memmap, FILE_MAP_ALL_ACCESS, 0, 0, size);
		if (mutex_name.size() != 0)
		{
			_mutex = CreateMutexA(NULL, FALSE, mutex_name.c_str());
			if (_mutex == nullptr)
				_mutex = OpenMutexA(SYNCHRONIZE, FALSE, mutex_name.c_str());
		}
		
		if (_mutex != nullptr || mutex_name.size() == 0)
		{
			*memmap = _memmap;
			*mutex = _mutex;
			*view = _view;
			return true;
		}
	}
	return false;
}

void release_mapping(void * memmap, void * mutex)
{
	memmap = nullptr;
	mutex = nullptr;
}

bool lock_mutex(void * mutex)
{
	return WaitForSingleObject(mutex, 100) == WAIT_OBJECT_0;
}

void unlock_mutex(void * mutex)
{
	if (mutex != nullptr)
		ReleaseMutex(mutex);
}

MemoryMapBuffer::MemoryMapBuffer()
{
	close();
}

MemoryMapBuffer::~MemoryMapBuffer()
{
	close();
}

bool MemoryMapBuffer::create(std::string name, int size, bool only_open)
{
	void* dummy;
	_name = name;
	return create_mapping(name, "", size, &_memmap, &dummy, &_ptr, only_open);
}

void MemoryMapBuffer::close()
{
	release_mapping(_memmap, nullptr);
	_name = "";
	_memmap = _ptr = nullptr;
}

std::string MemoryMapBuffer::mapname()
{
	return _name;
}

void * MemoryMapBuffer::ptr()
{
	return _ptr;
}
