#include "game_player.h"
#include "msg_server.h"
#include "service.h"


class game_service;
extern game_service the_service;


game_player::game_player()
{
	init_egg_game();
}
void    game_player::init_egg_game()
{
	cur_egg_level = 0;
	old_egg_level = 0;

	cur_jinbi     = 0;
	old_jinbi     = 0;
}

//
void game_player::on_connection_lost()
{

	save_db_egg_level();
	delete_db_egg_on_line();
}


void    game_player::send_cur_level_to_client()
{
		msg_173_eggs_ret_level msg;
		msg.egg_level_ = cur_egg_level;
		send_msg_to_client(msg);
}

void    game_player::send_cur_server_state_to_client(int state)
{
	    msg_173_eggs_service_state msg;
		msg.state_ = state;
		send_msg_to_client(msg);
}

void    game_player::save_db_egg_jinbi(int op_type)
{
	Database& db = *the_service.gamedb_;

	BEGIN_INSERT_TABLE("egg_jinbin_log");
	SET_STRFIELD_VALUE("uid", uid_);
	SET_FIELD_VALUE("old_level",old_egg_level );
	SET_FIELD_VALUE("new_level",cur_egg_level );
	SET_FIELD_VALUE("old_jinbi",old_jinbi );
	SET_FIELD_VALUE("new_jinbi",cur_jinbi );
	SET_FIELD_VALUE("op_type",op_type );
	SET_FINAL_VALUE("change_jinbi",cur_jinbi-old_jinbi );
	EXECUTE_IMMEDIATE();
}


void    game_player::save_db_egg_level()
{
	std::string sql;

	sql = "call egg_update_level('" + uid_ + "'," + lex_cast_to_str(cur_egg_level) + ");" ;

	Query q(*the_service.gamedb_);

	if (q.execute(sql))
	{

	}
	else
	{

	}

}

void    game_player::save_db_egg_data(int op_type,int gold ,int is_succs)
{
	Database& db = *the_service.gamedb_;

	if(op_type == 0 )
		gold = -abs(gold);


	BEGIN_INSERT_TABLE("gold_egg");
	SET_STRFIELD_VALUE("uid", uid_);
	SET_STRFIELD_VALUE("uname", name_);
	SET_STRFIELD_VALUE("nick_name","" );
	SET_FIELD_VALUE("op_type",op_type );
	SET_FIELD_VALUE("op_result",is_succs );
	SET_FIELD_VALUE("egg_level",old_egg_level );
	SET_FIELD_VALUE("new_egg_level",cur_egg_level );
	SET_FINAL_VALUE("gold",gold );
	EXECUTE_IMMEDIATE();

}

void    game_player::save_db_egg_on_line()
{
	Database& db(*the_service.gamedb_);
	BEGIN_REPLACE_TABLE("log_online_players");
	SET_STRFIELD_VALUE("uid", uid_);
	SET_STRFINAL_VALUE("name",name_);
	EXECUTE_IMMEDIATE();
}

void  game_player::delete_db_egg_on_line()
{
	Query q(*the_service.gamedb_);

	string sql = "delete from log_online_players  where uid = '" + uid_ + "'" ;
	q.execute(sql);
	q.free_result();
}

void game_player::set_last_operator_time()
{
	last_op_time = boost::posix_time::microsec_clock::local_time();
}