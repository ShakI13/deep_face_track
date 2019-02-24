// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#include "command_line.h"

char* get_arg(char ** begin, char ** end, const std::string & option)
{
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
	{
		return *itr;
	}
	return 0;
}

bool arg_exists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}