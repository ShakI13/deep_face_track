// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#pragma once
#include <string>

char* get_arg(char ** begin, char ** end, const std::string & option);

bool arg_exists(char** begin, char** end, const std::string& option);