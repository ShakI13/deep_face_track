#pragma once

#include <string>
#include <vector>

class Process
{
public:
	Process();
	~Process();

	bool create(std::string app);
	bool create(std::string app, std::vector<std::string> args);
	bool iterate();
	bool is_running();
	void close();

protected:
	unsigned long processId;
	unsigned long threadId;
	unsigned long status;
	void* hProcess;
	void* hThread;
	void* newstdin;
	void* newstdout;
	void* read_stdout;
	void* write_stdin;
};

