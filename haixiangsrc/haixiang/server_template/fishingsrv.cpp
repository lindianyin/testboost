#include "service.h"
#include "error_define.h"
#include "log.h"
bool glb_exit = false;
extern log_file<cout_output> glb_log;
int main()
{
	srand((unsigned int)time(NULL));
	int a = rand_r(0);

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
