#include "fish_cloud.h"
#include "simple_xml_parse.h"
#include <math.h>
#include "game_logic.h"
#include "service.h"
#include "tinyxml2.h"
using namespace nsp_simple_xml;

std::map<std::string, xml_doc_ptr> the_xml_cache_;

int		generate_iid()
{
	static int iid = 0;
	return ++iid;
}

int		generateGroupId()
{
	static int iid = 0;
	return ++iid;
}

xml_doc_ptr get_xml_cache(std::string fname)
{
	auto itf = the_xml_cache_.find(fname);
	xml_doc_ptr doc;
	if (the_xml_cache_.find(fname) == the_xml_cache_.end()){
		doc.reset(new xml_doc);
		if(!doc->parse_from_file(fname.c_str())){
			doc.reset();
		}
		else{
			the_xml_cache_.insert(std::make_pair(fname, doc));
		}
	}
	else{
		doc = itf->second;
	}
	return doc;
}

std::list<Point> generate_random_path()
{
	std::list<Point> vt;
	Point pts, ptm1, ptm2, pte;
	//水平走
	if(rand_r(1, 2) == 1){
		pts.x = -int(rand_r(200, 600));
		pts.y = rand_r(100, 700);

		ptm1.x = rand_r(0, 1600);
		ptm1.y = rand_r(0, 900);

		ptm2.x = rand_r(0, 1600);
		ptm2.y = rand_r(0, 900);

		pte.x = 1600 + rand_r(200, 600);
		pte.y = rand_r(100, 700);
	}
	//竖直走
	else{
		pts.x = rand_r(100, 1500);
		pts.y = -int(rand_r(200, 600));

		ptm1.x = rand_r(0, 1600);
		ptm1.y = rand_r(0, 900);

		ptm2.x = rand_r(0, 1600);
		ptm2.y = rand_r(0, 900);

		pte.x = rand_r(100, 1500);
		pte.y = 900 + rand_r(200, 600);
	}
	vt.push_back(pts);
	vt.push_back(ptm1);
	vt.push_back(ptm2);
	vt.push_back(pte);
	return vt;
}

bool		get_fish_config(std::string path, std::map<std::string, fish_config_ptr>& the_fish_config)
{
	tinyxml2::XMLDocument *myDocument = new tinyxml2::XMLDocument();
	myDocument->LoadFile(path.c_str());
	tinyxml2::XMLElement* rootElement = myDocument->RootElement();  //root
	std::ostringstream os;
	os << rootElement->Value();;
	std::string text = os.str();

	if (text == "root")
	{
		tinyxml2::XMLElement* itemElement = rootElement->FirstChildElement();  //itemElement
		while (itemElement) {
			fish_config_ptr cnf(new fish_config());
			const tinyxml2::XMLAttribute* attributeOfItem = itemElement->FindAttribute("id");  //获得item的id属性
			if (attributeOfItem)
			{
				cnf->id_ = attributeOfItem->Value();
				cnf->type_ = itemElement->FindAttribute("type")->Value();
				cnf->rate_ = boost::lexical_cast<float>(itemElement->FindAttribute("rate")->Value());
				cnf->mv_step_ = boost::lexical_cast<int>(itemElement->FindAttribute("step")->Value());
				cnf->spawn_rate_ = boost::lexical_cast<int>(itemElement->FindAttribute("spawn")->Value());
				cnf->spawn_min_ = boost::lexical_cast<int>(itemElement->FindAttribute("minspawn")->Value());
				cnf->spwan_max_ = boost::lexical_cast<int>(itemElement->FindAttribute("maxspawn")->Value());
				cnf->szx_ = boost::lexical_cast<int>(itemElement->FindAttribute("spawnw")->Value());
				cnf->szy_ = boost::lexical_cast<int>(itemElement->FindAttribute("spawnh")->Value());
				cnf->reward_ = boost::lexical_cast<int>(itemElement->FindAttribute("reward")->Value());
				cnf->exp_ = boost::lexical_cast<int>(itemElement->FindAttribute("experience")->Value());
				cnf->data_ = boost::lexical_cast<int>(itemElement->FindAttribute("data")->Value());
				the_fish_config.insert(std::make_pair(cnf->id_, cnf));
			}
			itemElement = itemElement->NextSiblingElement();
		}
	}
	else
	{
		xml_doc doc;
		if (!doc.parse_from_file(path.c_str()))return false;
		xml_node* pn = doc.get_node("config");
		for (int i = 0 ; i < (int)pn->child_lst.size(); i++)
		{
			xml_node& fish_node = pn->child_lst[i];
			fish_config_ptr cnf(new fish_config());
			cnf->type_ = fish_node.get_value<std::string>("type", "");
			cnf->rate_ = fish_node.get_value("rate", 0);
			cnf->mv_step_ = fish_node.get_value("step", 0);
			cnf->spawn_rate_ = fish_node.get_value("spawn", 0);
			cnf->spawn_min_ = fish_node.get_value("minspawn", 0);
			cnf->spwan_max_ = fish_node.get_value("maxspawn", 0);
			cnf->szx_ = fish_node.get_value("spawnw", 0);
			cnf->szy_ = fish_node.get_value("spawnh", 0);
			cnf->reward_ = fish_node.get_value("reward", 0);
			cnf->exp_= fish_node.get_value("experience", 0);
			cnf->data_ = fish_node.get_value("data", 0);
			cnf->width_ = fish_node.get_value("width", 100);
			cnf->height_ = fish_node.get_value("height", 50);
			cnf->id_ = fish_node.get_value<std::string>("id", cnf->type_);
			the_fish_config.insert(std::make_pair(cnf->id_, cnf));
		}
	}
	return true;
}

//必须加载了fish_config,再调用这个
bool		get_path_config(std::string path, std::map<std::string, path_config>& the_path,  std::map<std::string, fish_config_ptr>& the_fish_config)
{
	return true;
}

std::vector<fish_ptr> fish_cloud_generator::generate()
{
	std::vector<fish_ptr> vt;
	if(fish_templates_.empty()) return vt;

	if (howto_use_fish_templates_ == use_template_one_by_one){
		fish_ptr fh(new fish());
		if (fh.get()){
			fh->strid_ = fish_templates_[generate_index_];
			fh->init(the_fish_config_);
			vt.push_back(fh);
		}
	}
	else if (howto_use_fish_templates_ == use_template_once_a_random){
		int r = rand_r(fish_templates_.size() - 1);
		fish_ptr fh(new fish);
		if (fh.get()){
			fh->strid_ = fish_templates_[r];
			fh->init(the_fish_config_);
			vt.push_back(fh);
		}
	}
	else if (howto_use_fish_templates_ == use_template_once_for_all){
		for (int i = 0; i < fish_repeat_; i++)
		{
			for (auto s : fish_templates_){
				fish_ptr fh(new fish());
				if (fh.get()){
					fh->strid_ = s;
					fh->init(the_fish_config_);
					vt.push_back(fh);
				}
			}
		}
	}

	if (!spawn_location_.empty()){
		if (howto_use_spawn_location_ == use_spawn_one_by_one){
			for (int i = 0 ; i < (int)vt.size(); i++)
			{
				int r =  i % spawn_location_.size();
				vt[i]->pos_ = spawn_location_[r];
			}
		}
		else if (howto_use_spawn_location_ == use_spawn_random){
			for (auto& fh : vt)
			{
				int r = rand_r(spawn_location_.size() - 1);
				fh->pos_ = spawn_location_[r];
			}
		}
		else if (howto_use_spawn_location_ == use_spawn_rect)
		{
			//矩形数据为左下角起始，逆时针
			for (auto& fh : vt)
			{
				int rx = rand_r(spawn_location_[0].x);	
				int ry = rand_r(spawn_location_[0].y);
				fh->pos_ = Point(rx, ry);
			}
		}
	}
	if (!moving_path_.empty()){
		Point pt = moving_path_.front();
		for (int i = 0 ; i < (int)vt.size(); i++)
		{
			vt[i]->pos_ += (pt + offset_);
		}
	}
	generate_index_++;
	return vt;
}

int fish_cloud_generator::update(float dt)
{
	gernate_counter_ += dt;

	if (cloud_repeat_ <= 0){
		is_finished_ = true;
		return 1;
	}

	if (gernate_counter_ > gernate_interval_){
		gernate_counter_ = 0;
		std::vector<fish_ptr> vfh = generate();
		for (auto& itf : vfh)
		{
			if (itf.get()){
				itf->path_ = moving_path_;
				itf->path_backup_ = moving_path_;
				logic_ptr plogic =  the_logic_.lock();
				if(plogic.get()){
					plogic->join_fish(itf);
				}
			}
		}
		//1遍生成完毕
		if (generate_index_ == fish_templates_.size() - 1){
			cloud_repeat_--;
			generate_index_ = 0;
		}
	}
	return 0;
}

fish_cloud_generator::fish_cloud_generator(std::vector<std::string>& fish_templates,
																					 std::list<Point>& mov_path,
																					 std::vector<Point>& spawn_location,	//鱼群刷新点
																					 int		howto_use_fish_templates,	//怎么样使用鱼模板
																					 int		howto_use_spawn_location,			//怎么样使用鱼群刷新点
																					 float	interval,
																					 float	speed,
																					 int		repeat,
																					 Point	offset,
																					 int		fish_repeat)
{
	fish_templates_ = fish_templates;
	moving_path_ = mov_path;
	gernate_counter_ = 0;
	gernate_interval_ = interval;
	speed_ = speed;
	spawn_location_ = spawn_location;
	howto_use_spawn_location_ = howto_use_spawn_location;
	howto_use_fish_templates_ = howto_use_fish_templates;
	cloud_repeat_ = repeat;
	generate_index_ = 0;
	offset_ = offset;
	fish_repeat_ = fish_repeat;
	cloud_type_ = cloud_alone;
}


inline Point	get_bezier_point(Point&p0, Point& p1, float t)
{
	return p0 * (1 - t) + p1 * t;
}

Point get_bezier_point(vector<Point>& v, float t)
{
	if (v.size() == 2){
		return get_bezier_point(v[0], v[1], t);
	}
	else{
		vector<Point> v1, v2;
		v1.insert(v1.begin(), v.begin(), v.end() - 1);
		v2.insert(v2.begin(), v.begin() + 1, v.end());
		return get_bezier_point(v1, t) * (1 - t) + get_bezier_point(v2, t) * t;
	}
}


//2次贝塞尔曲线平滑
//px = (1-t)*(1-t) * p0 + 2*t(1-t) * p1 + t * t * p2;

void  do_smooth(std::vector<int>& v, std::vector<Point>& vp, int& i)
{
	v.insert(v.begin(), v[0] - 1);
	v.insert(v.end(), v[v.size() - 1] + 1);

	std::vector<Point> tvp;
	for (int j = 0; j < (int)v.size(); j++)
	{
		tvp.push_back(vp[v[j]]);
	}

	//移除要代替的点
	for (int j = 1; j < (int)v.size() - 1; j++)
	{
		vp.erase(vp.begin() + v[1]);
	}

	std::vector<Point> vresult;

	for (int ii = 1; ii < 10; ii++)
	{
		vresult.push_back(get_bezier_point(tvp, ii / 10.0));
	}
	//在移除位置插入填充点
	vp.insert(vp.begin() + v[1], vresult.begin(), vresult.end());
	i = v[1] + vresult.size();

	v.clear();
}

void smooth_path(std::vector<Point>& vp)
{
	std::vector<int> v;
	for (int i = 1 ; i < (int) vp.size() - 1;)
	{
		Point p0 = vp[i - 1];
		Point p1 = vp[i];
		Point p2 = vp[i + 1];

		Point v1 = (p1 - p0).normalize();
		Point v2 = (p2 - p1).normalize();

		float cosx = acos(v1.dot(v2));
		//夹角>30度，需要调整曲线
		if (abs(cosx) > M_PI / 12){
			//如果上一角度也是需要调整的，则加入到上个调整组
			v.push_back(i);
			i++;
		}
		else{
			if (!v.empty()){
				do_smooth(v, vp, i);
			}
			else{
				i++;
			}
		}
	}

	if (!v.empty()){
		int i = 0;
		do_smooth(v, vp, i);
	}
}

fcloud_ptr create_cloud_from_xml(std::string xml, Point offset, logic_ptr plogic)
{
	xml_doc_ptr pdoc = get_xml_cache(xml);
	if (!pdoc.get()) return fcloud_ptr();
	xml_doc& doc = *pdoc;

	if(!doc.is_parsed()) return fcloud_ptr();

	fcloud_ptr ret;
	xml_node* root = doc.get_node("cloud");
	std::string cloud_class = root->get_attr<std::string>("class");
	if (cloud_class == "fish_cloud_generator"){
		int howto_use_fish_templates = root->get_value<int>("howto_use_fish_templates", 0);
		int howto_use_spawn_location = root->get_value<int>("howto_use_spawn_location", 0);
		
		xml_node* path_node = root->get_node("fish_path");
		std::string fish_path = path_node->get_value<std::string>();
		int reverse_path = path_node->get_attr<int>("reverse");

		float speed = root->get_value<float>("speed", 0);
		int repeat = root->get_value<int>("repeat", 0);
		float interval = root->get_value<float>("interval", 0);

		std::vector<std::string> fish_l;
		xml_node* pfish_tmpl = root->get_node("fish_templates");
		int fish_repeat =  pfish_tmpl->get_attr<int>("repeat");
		for (int i = 0; i < (int) pfish_tmpl->child_lst.size(); i++)
		{
			xml_node& pn = pfish_tmpl->child_lst[i];
			fish_l.push_back(pn.get_attr<std::string>("src"));
		}
		
		std::vector<Point> spawn_l;
		xml_node* pspawn = root->get_node("spawn_location");
		for (int i = 0; i < (int) pspawn->child_lst.size(); i++)
		{
			xml_node& pn = pspawn->child_lst[i];
			Point pt;
			pt.x = pn.get_attr<int>("x");
			pt.y = pn.get_attr<int>("y");
			spawn_l.push_back(pt);
		}

		std::list<Point> path_l = generate_random_path();
		if (reverse_path){
			std::reverse(path_l.begin(), path_l.end());
		}
		std::vector<Point> vp;
		vp.insert(vp.begin(), path_l.begin(), path_l.end());
		smooth_path(vp);
		path_l.clear();
		path_l.insert(path_l.begin(), vp.begin(), vp.end());
		fish_cloud_generator* pret = new fish_cloud_generator(fish_l,
			path_l,
			spawn_l,
			howto_use_fish_templates,
			howto_use_spawn_location,
			interval, speed, repeat, offset, fish_repeat);
		ret.reset(pret);

		pret->the_logic_ = plogic;
		if (plogic->scene_set_){
			pret->the_fish_config_ = plogic->scene_set_->the_fish_config_;
		}
	}
	else if (cloud_class == "fish_cloud_generator_combine"){
		fish_cloud_generator_combine* fcgc = new fish_cloud_generator_combine();
		for (int i = 0 ; i < (int)root->child_lst.size(); i++)
		{
			xml_node& pn = root->child_lst[i];
			std::string path = pn.get_attr<std::string>("path");
			if (strstr(xml.c_str(), path.c_str()) != nullptr){
				continue;
			}
			float offx = pn.get_attr<float>("offsetx");
			float offy = pn.get_attr<float>("offsety");
			fcloud_ptr pcld = create_cloud_from_xml(path, Point(offx, offy), plogic);
			if (pcld.get())	{
				fcgc->clouds_.push_back(pcld);
			}
		}
		ret.reset(fcgc);
		ret->the_logic_ = plogic;
	}
	return ret;
}

int fish_cloud_generator_combine::update(float dt)
{
	bool all_finished = 1;
	for (auto& cp : clouds_)
	{
		if(cp->update(dt) != 1)	{
			all_finished = 0;
		}
	}
	is_finished_ = all_finished;
	return all_finished;
}

int fish_freely_generator::update(float dt)
{
	logic_ptr plogic =  the_logic_.lock();
	if(!plogic.get()) return 1;
	if (pt.is_not_a_date_time()){
		pt = boost::posix_time::microsec_clock::local_time();
	}
	
	if ((boost::posix_time::microsec_clock::local_time() - pt).total_milliseconds() > 500){

		if (plogic->fishes_.size() < the_service.the_config_.fish_max_amount){
			std::vector<fish_ptr> v = spawn(plogic->scene_set_->the_fish_config_, 
				plogic->scene_set_->the_path_);

			for(auto fh:v)
			{
				plogic->join_fish(fh);
			}
		}
		pt = boost::posix_time::microsec_clock::local_time();
	}
	return 0;
}

std::vector<fish_ptr> fish_freely_generator::spawn(
	std::map<std::string, fish_config_ptr>& the_fish_config_,
	std::map<std::string, path_config>& the_path_,
	bool random_startpos)
{
	std::vector<fish_ptr> v;
	do 
	{
		fish_config_ptr cnf;
		{
			int r = rand_r(10000);
			int n = 0;
			auto it = the_fish_config_.begin();
			while (it != the_fish_config_.end())
			{
				cnf = it->second; it++;
				n += cnf->spawn_rate_;
				if (r > n){
					cnf.reset();
					continue;
				}
				break;
			}
		}

		if (!cnf.get()) break;

		int cnt = rand_r(cnf->spawn_min_, cnf->spwan_max_);

		std::list<Point> path;
		{
			int  r = rand_r(10000);
			int n = 0;
			if (!cnf->preset_path_.empty()){
				auto it = cnf->preset_path_.begin();
				while (it != cnf->preset_path_.end())
				{
					path_config& pconf = *it; it++;
					n += pconf.rate_;
					if (r > n){
						continue;
					}
					path = pconf.path_;
					break;
				}
			}
			else{
				if (the_path_.empty()){
					std::list<Point> v = generate_random_path();
					path.insert(path.end(), v.begin(), v.end());
				}
				else{
					auto it = the_path_.begin();
					while (it != the_path_.end())
					{
						path_config& pconf = it->second; it++;
						n += pconf.rate_;

						auto itsf = std::find(cnf->exclude_path_.begin(), cnf->exclude_path_.end(), pconf.type_);
						if (itsf != cnf->exclude_path_.end()){
							continue;
						}

						if (r > n){
							continue;
						}
						path = pconf.path_;
						break;
					}
				}
			}
		}

		if (path.empty())  break;
		if (rand_r(100) < 50){
			path.reverse();
		}

		std::vector<Point> vp;
		vp.insert(vp.begin(), path.begin(), path.end());
		smooth_path(vp);
		if (vp.size() == 5){
			int a = 0;
		}

		path.clear();	
		path.insert(path.begin(), vp.begin(), vp.end());
		vp.clear();

		std::string strid = cnf->id_;
		auto itf = the_fish_config_.find(cnf->id_);

		//delete by liufapu
// 		if (itf != the_fish_config_.end()){
// 			if (itf->second->data_ == 1){
// 				strid = get_guid();
// 			}
// 		}
		int tempId = 0;
		for (int i = 0 ; i < cnt; i++)
		{
			int x = 0;			int y = 0;

			if (random_startpos){
				if (i == 0){
					y = cnf->szy_ / 2;
					x = cnf->szx_ / 2;
				}
				else if (i % 2 == 0){
					y = rand_r(1, cnf->szy_ / 2) - cnf->height_ / 2;
					x = rand_r(1, cnf->szx_);
				}
				else{
					y = rand_r(cnf->szy_ / 2, cnf->szy_) + cnf->height_ / 2;
					x = rand_r(1, cnf->szx_);
				}
			}
		
			Point pt = Point(x, y);
			fish_ptr fh(new fish());
			fh->strid_ = cnf->id_;
			fh->type_ = cnf->type_;
			fh->init(the_fish_config_);
			fh->strid_ = strid;
			fh->mv_step_ *= rand_r(8000, 12000) / 10000.0;
			fh->path_ = path;
			fh->path_backup_ = path;
			fh->pos_ = pt;
			fh->pos_ += path.front();//设置到路径第一个结点位置
			if (i == 0)
			{
				fh->m_groupId = generateGroupId();
				tempId = fh->m_groupId;
			}
			else
			{
				fh->m_groupId = tempId;
			}
			v.push_back(fh);
		}
	} while (0);
	return v;
}

bool fish::move_to_next()
{
	if (path_.size() <= 1) return false;
	
	Point dir = *(++path_.begin()) - (*path_.begin());
	pmov_.reset(new move_by_vector(dir, shared_from_this()));
	pmov_->speed_up(acc_);
	return true;
}

int fish::update(float dt)
{
	int r = 1;
	if (pchg_.get()){
		r = pchg_->update(dt);
		if (r == 1){
			pchg_.reset();
		}
	}
	
	if (pmov_.get()){
		r = pmov_->update(dt);
		if (r == 1 && !path_.empty()){
			path_.pop_front();
			if(!move_to_next()){
				r = 1;
			}
			else{
				r = 0;
			}
		}
	}
	return r;
}

fish::fish()
{
	iid_ = generate_iid();
	pos_ = Point(0, 0);
	lottery_ = 0;
	rate_little_ = 0;
	die_enable_ = 1;
	data_ = 0;
	acc_ = 1.0;
	lock_by_player_ = 0;
	m_groupId = 0;
}

void fish::init(std::map<std::string, fish_config_ptr>& the_fish_config)
{
	consume_gold_ = 0;
	auto itf = the_fish_config.find(strid_);
	if (itf != the_fish_config.end()){
		type_ = itf->second->type_;
		rate_ = itf->second->rate_;
		mv_step_ = itf->second->mv_step_;
		reward_ = itf->second->reward_;
		exp_ = itf->second->exp_;
		data_ = itf->second->data_;
		width_ = itf->second->width_;
		height_ = itf->second->height_;
	}
}

int move_by_vector::update(float time)
{
	Point pv = mov_vector_.normalize();
	fish_ptr target_ = wtarget_.lock();
	if (!target_.get()) return 1;
	float rotation_slowdown = 0.8;
	Point pmov = pv * target_->mv_step_ * acc_ * time * rotation_slowdown;
	moved_vector_ += pmov;
	if (moved_vector_.get_lengthsq() >= mov_vector_.get_lengthsq()){
		target_->pos_ = pos_dest_;
		return 1;
	}
	else{
		target_->pos_ += pmov;
	}
	return 0;
}

float move_by_vector::get_mov_percent()
{
	if (mov_vector_.get_length() < 0.000001) return 1.0;
	float r = moved_vector_.get_length() / mov_vector_.get_length();
	if (r > 1.0) r = 1.0;
	return r;
}

void move_by_vector::speed_up(float acc)
{
	acc_ = acc;
}

fish_config::fish_config()
{
	rate_ = 0;
	mv_step_ = 0;
	spawn_rate_ = 0;
	spawn_min_ = spwan_max_ = 0;
	szx_ = szy_ = 0;
	reward_ = 0;
	exp_ = 0;
	data_ = 0;
	width_ = 100;
	height_ = 50;
}

int move_speed_change::update(float time)
{
	speed_now += time / duration_ * (sp_dest_ - sp_start_);
	if (speed_now < sp_dest_){
		speed_now = sp_dest_;
	}

	the_move_->speed_up(speed_now);

	if (get_timeleft() > (duration_ * 1000)){
		return 1;
	}

	return 0;
}

move_speed_change::move_speed_change(float sp_dest, float duration, boost::shared_ptr<move_by_vector> mov)
{
	start_tick_ = boost::posix_time::microsec_clock::local_time();
	duration_ = duration;
	sp_dest_ = sp_dest;
	sp_start_ = mov->get_acc();
	the_move_ = mov;
}

int move_speed_change::get_timeleft()
{
	return (boost::posix_time::microsec_clock::local_time() - start_tick_).total_milliseconds();
}
