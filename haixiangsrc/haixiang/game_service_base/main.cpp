#include "log.h"
#include "service.h"
#include <DbgHelp.h>
#include "boost/atomic.hpp"
#include "game_service_base.hpp"
#pragma auto_inline (off)
#pragma comment(lib, "dbghelp.lib")

boost::atomic<bool> glb_exit;
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
	while (!glb_exit.load())
	{
		boost::this_thread::sleep(boost::posix_time::millisec(1000));
	}
	return TRUE;
}

int		dump = 0;
LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
	std::string fname = "crash" + boost::lexical_cast<std::string>(dump++) + ".dmp";
	HANDLE lhDumpFile = CreateFileA(fname.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL ,NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = ExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);

	CloseHandle(lhDumpFile);
	return EXCEPTION_EXECUTE_HANDLER;
}

int main()
{
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
	{
		return -1;
	}
	srand(::time(nullptr));
	if (the_service.run() != ERROR_SUCCESS_0)
	{
		glb_log.write_log("service run failed!");
		goto _Exit;
	}
	glb_log.write_log("service run successful!");

	__try{
		the_service.main_thread_update();
	}
	__except(MyUnhandledExceptionFilter(GetExceptionInformation()),
		EXCEPTION_EXECUTE_HANDLER){

	}
_Exit:
	the_service.stop();
	glb_log.stop_log();
	glb_exit = true;
	return 0;
}
