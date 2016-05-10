
void service_config_base::refresh()
{
	Query q(*the_service.gamedb_);
	std::string sql = "SELECT init_stock, force_set, lowcap, lowcap_max, upcap, upcap_max, decay_per_hour FROM server_parameters_stock_control";
	q.get_result(sql);
	int forece_set = 0;
	if (q.fetch_row()){
		longlong stock = q.getbigint();
		forece_set = q.getval();
		the_service.the_new_config_.stock_lowcap_ = q.getbigint();
		the_service.the_new_config_.stock_lowcap_max_ = q.getbigint();
		the_service.the_new_config_.stock_upcap_start_ = q.getbigint();
		the_service.the_new_config_.stock_upcap_max_ = q.getbigint();
		the_service.the_new_config_.stock_decay_ = q.getbigint();
		if (forece_set){
			the_service.add_stock(stock);
		}
	}
	q.free_result();

	sql = "select uid, rate from server_parameters_blacklist";
	q.get_result(sql);
	the_service.the_new_config_.personal_rate_control_.clear();
	the_service.the_config_.personal_rate_control_.clear();
	while (q.fetch_row())
	{
		std::string uid = q.getstr();
		float rate = q.getnum();
		the_service.the_new_config_.personal_rate_control_.insert(
			std::make_pair(uid, rate));
	}
	q.free_result();

	if (forece_set){
		sql = "update server_parameters_stock_control set force_set = 0";
		q.execute(sql);
	}
	q.free_result();
}