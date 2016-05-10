#include "player.h"
#include <map>
#include "utf8_db.h"
#include "Query.h"
#include "utility.h"
#include "db_delay_helper.h"
#include <unordered_map>

extern boost::shared_ptr<utf8_data_base>	db_;
extern db_delay_helper<std::string, int>& get_delaydb();
void koko_player::connection_lost()
{
	{
		BEGIN_UPDATE_TABLE("user_account");
		SET_FINAL_VALUE("is_online", 0);
		EXECUTE_NOREPLACE_DELAYED("", get_delaydb());
	}
	{
		BEGIN_INSERT_TABLE("log_user_loginout");
		SET_FIELD_VALUE("uid", uid_);
		SET_FINAL_VALUE("islogin", 0);
		EXECUTE_NOREPLACE_DELAYED("", get_delaydb());
	}
}

