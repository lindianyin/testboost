#include "service.h"
#include "log.h"
#include <DbgHelp.h>

#pragma comment(lib, "dbghelp.lib")

bool glb_exit = false;
extern log_file<cout_output> glb_log;
BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	switch(CEvent)
	{
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		the_service.the_config_.shut_down_ = 1;
	}
	while (!glb_exit)
	{
	}
	return TRUE;
}

inline int CreateMiniDump(EXCEPTION_POINTERS* pep)
{
	HANDLE hFile = CreateFile("crash.dmp", GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)){
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId           = GetCurrentThreadId();
		mdei.ExceptionPointers  = pep;
		mdei.ClientPointers     = FALSE;
		MINIDUMP_CALLBACK_INFORMATION mci;
		mci.CallbackRoutine     = NULL;
		mci.CallbackParam       = 0;
		MINIDUMP_TYPE mdt       = MiniDumpNormal;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hFile, mdt, (pep != 0) ? &mdei : 0, 0,  0);
		CloseHandle(hFile); 
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

int main()
{
	srand((unsigned int)time(NULL));
	int a = rand_r(0);

	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
	{
		return -1;
	}

	if (the_service.run() != ERROR_SUCCESS_0)
	{
		glb_log.write_log("service run failed!");
		return 0;
	}
	glb_log.write_log("service run successful!");
	__try
	{
		the_service.main_thread_update();
	}
	__except(CreateMiniDump(GetExceptionInformation())){
		glb_log.write_log("exception catched in main_thread_update, exiting program!");
	}

	the_service.stop();
	glb_log.stop_log();
	glb_exit = true;
	return 0;
}
