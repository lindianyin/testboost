#include "koko_socket.h"
#include "player.h"

void koko_socket::close()
{
	remote_socket_impl<koko_socket, koko_player>::close();
	player_ptr pp = the_client_.lock();
	if (pp.get()){
		pp->is_connection_lost_ = 1;
	}
}

koko_socket::koko_socket(net_server<koko_socket>& srv):
	remote_socket_impl<koko_socket, koko_player>(srv)
{
	set_authorize_check(900000);
	is_register_ = is_login_ = false;
}
