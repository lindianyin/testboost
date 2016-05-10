#include "service.h"
#include "error_define.h"
#include "log.h"


bool glb_exit = false;
BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	switch(CEvent)
	{
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		glb_exit = true;
	}
	boost::posix_time::milliseconds ms(1000);
	boost::this_thread::sleep(ms);
	return TRUE;
}

int main()
{
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
	the_service.main_thread_update();
	the_service.stop();
	glb_log.stop_log();
	return 0;
}
