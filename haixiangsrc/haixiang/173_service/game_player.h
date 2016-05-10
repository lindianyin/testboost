#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
#include "msg.h"
#include "game_service_base.h"


using namespace std;
class game_logic;
class game_player;




typedef boost::shared_ptr<game_player> player_ptr;


class game_player : public player_base< game_player,remote_socket<game_player> > 
{
public:
	//��Ϸ��ʱ����
	boost::weak_ptr<game_logic> the_game_;
	int cur_egg_level;
	int old_egg_level; //������֮ǰ�ĵȼ�

	longlong old_jinbi;
	longlong cur_jinbi;


	string rel_uid;

	boost::posix_time::ptime		last_op_time;

	game_player::game_player();

	//ÿ�ν����ҵ���Ϸ�����Ǵ�0����ʼ��
	void    init_egg_game(); 

	void    send_cur_level_to_client();

	void    send_cur_server_state_to_client(int state);

	void	on_connection_lost();

	void    save_db_egg_level();

	void    save_db_egg_jinbi(int op_type);

	void    save_db_egg_data(int op_type,int gold ,int is_succs );

	void    save_db_egg_on_line();

	void    delete_db_egg_on_line();

	//
	void    set_last_operator_time();

};
