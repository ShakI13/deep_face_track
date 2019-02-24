// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

/*
Class allows to instantiate external application with command-line parameters
*/
class Process
{
public:
	Process();
	~Process();

	/**
	 * \brief Starts new process
	 * \param app path to exe file
	 * \return true if success
	 */
	bool create(std::string app);

	/**
	 * \brief Starts new process
	 * \param app path to exe file
	 * \param args command-line arguments in format ['-arg0','val0',...]
	 * \return true if success
	 */
	bool create(std::string app, std::vector<std::string> args);

	/**
	 * \brief Checks the process status and stdin, stdout
	 * \return true if success and the process is still active
	 */
	bool iterate();

	/**
	 * \brief Checks the process is still active
	 * \return true if active
	 */
	bool is_running();

	/**
	 * \brief Terminates the process
	 */
	void close();

protected:
	unsigned long _process_id;
	unsigned long _thread_id;
	unsigned long _status;

	void* hProcess;
	void* hThread;
	void* newstdin;
	void* newstdout;
	void* read_stdout;
	void* write_stdin;
};

