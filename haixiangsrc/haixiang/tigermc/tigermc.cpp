#include "service.h"
#include "error_define.h"
#include "log.h"
#include "game_logic.h"

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

	srand((unsigned int)time(NULL));
	int a = rand_r(0);
	if (the_service.run() != ERROR_SUCCESS_0)
	{
		glb_log.write_log("service run failed!");
		return 0;
	}
	
	glb_log.write_log("service run successful!");
	the_service.main_thread_update();
	log_file<file_output> file_log;
	file_log.output_.fname_ = "exit.log";
	file_log.write_log("program is exiting...");
	glb_log.stop_log();
	file_log.stop_log();
	return 0;
}
