#include "process.h"

#include <conio.h>
#include <Windows.h>
#include <sstream>

#define bzero(a) memset(a,0,sizeof(a)) //easier -- shortcut

bool _isWinNT()  //check if we're running NT
{
	OSVERSIONINFO osv;
	osv.dwOSVersionInfoSize = sizeof(osv);
	GetVersionEx(&osv);
	return (osv.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

void _errorMessage(char *str, DWORD code = 0)  //display detailed error info
{
	if (code == 0)
		code = GetLastError();

	wchar_t err[256];
	memset(err, 0, 256);

	auto ret = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		err,
		255,
		NULL
	);
	code = GetLastError();
	ret = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		err,
		255,
		NULL
	);
	printf(str);
	wprintf(err);
	printf("\n");
	//int msgboxID = MessageBoxW(NULL,
	//	err,
	//	(LPCWSTR)L"X",
	//	MB_OK);
}

Process::Process()
{
	processId = threadId = status = 0;
	hProcess = hThread = newstdin = newstdout = read_stdout = write_stdin = nullptr;
	close();
}

Process::~Process()
{
	close();
}

bool Process::create(std::string app)
{
	std::vector<std::string> args;

	return create(app, args);
}

bool Process::create(std::string app, std::vector<std::string> args)
{
	close();

	std::stringstream ss;
	ss << app;
	for (int i = 0; i < args.size(); i++)
	{
		ss << " " << args[i];
	}
	std::string cmd_line = ss.str();

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	if (_isWinNT())        //initialize security descriptor (Windows NT)
	{
		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, true, NULL, false);
		sa.lpSecurityDescriptor = &sd;
	}

	else sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = true;         //allow inheritable handles

	if (!CreatePipe(&newstdin, &write_stdin, &sa, 0))   //create stdin pipe
	{
		_errorMessage("Failed to redirect stdin");
		close();
		return false;
	}
	if (!CreatePipe(&read_stdout, &newstdout, &sa, 0))  //create stdout pipe
	{
		_errorMessage("Failed to redirect stdout");
		close();
		return false;
	}

	ZeroMemory(&si, sizeof(si));
	GetStartupInfoA(&si);      //set startupinfo for the spawned process
	//						   /*
	//						   The dwFlags member tells CreateProcess how to make the process.
	//						   STARTF_USESTDHANDLES validates the hStd* members. STARTF_USESHOWWINDOW
	//						   validates the wShowWindow member.
	//						   */
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = newstdout;
	si.hStdError = newstdout;     //set the new handles for the child process
	si.hStdInput = newstdin;
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process. 
	if (!CreateProcessA(app.c_str(),   // No module name (use command line)
		(char*)cmd_line.c_str(),        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		TRUE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		_errorMessage("Failed to create process");
		close();
		return false;
	}

	hProcess = pi.hProcess;
	hThread = pi.hThread;
	processId = pi.dwProcessId;
	threadId = pi.dwThreadId;
	GetExitCodeProcess(hProcess, &status);      //while the process is running

	return true;
}

bool Process::iterate()
{
	if (hProcess == nullptr)
		return false;

	char buf[1024];           //i/o buffer
	status = 0;  //process exit code
	unsigned long bread;   //bytes read
	unsigned long avail;   //bytes available

	bzero(buf);
	GetExitCodeProcess(hProcess, &status);      //while the process is running
	if (status != STILL_ACTIVE)
	{
		if (status != 0)
			_errorMessage("Process crash:", status);
		return false;
	}

	PeekNamedPipe(read_stdout, buf, 1023, &bread, &avail, NULL);
	//check to see if there is any data to read from stdout
	if (bread != 0)
	{
		bzero(buf);
		if (avail > 1023)
		{
			while (bread >= 1023)
			{
				ReadFile(read_stdout, buf, 1023, &bread, NULL);  //read the stdout pipe
				printf("%s", buf);
				bzero(buf);
			}
		}
		else {
			ReadFile(read_stdout, buf, 1023, &bread, NULL);
			printf("%s", buf);
		}
	}
	if (kbhit())      //check for user input.
	{
		bzero(buf);
		*buf = (char)getche();
		//printf("%c",*buf);
		WriteFile(write_stdin, buf, 1, &bread, NULL); //send it to stdin
		if (*buf == '\r') {
			*buf = '\n';
			printf("%c", *buf);
			WriteFile(write_stdin, buf, 1, &bread, NULL); //send an extra newline char,
															//if necessary
		}
	}

	return true;
}

bool Process::is_running()
{
	return status == STILL_ACTIVE;
}

void Process::close()
{
	if (is_running())
	{
		auto ret = TerminateProcess(hProcess, 0);
		if (ret == 0)
			_errorMessage("Failed to terminate a process");
	}

	if (hProcess != nullptr)
		CloseHandle(hProcess);
	if (hThread != nullptr)
		CloseHandle(hThread);
	if (newstdin != nullptr)
		CloseHandle(newstdin);
	if (newstdout != nullptr)
		CloseHandle(newstdout);
	if (read_stdout != nullptr)
		CloseHandle(read_stdout);
	if (write_stdin != nullptr)
		CloseHandle(write_stdin);

	processId = threadId = status = 0;
	hProcess = hThread = newstdin = newstdout = read_stdout = write_stdin = nullptr;
}
