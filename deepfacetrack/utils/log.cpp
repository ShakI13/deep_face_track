// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#include "log.h"
#include <iostream>

FILE* _log;

void open_log(bool show_log)
{
	if (show_log)
		_log = fopen("dft_camera.log", "w");
	else
		_log = NULL;
}

void close_log()
{
	if (_log != NULL)
		fclose(_log);
}

void write_log(std::string line)
{
	using namespace std;
	cout << line << endl;
	if (_log != NULL)
	{
		fputs(line.c_str(), _log);
		fputs("\n", _log);
	}
}