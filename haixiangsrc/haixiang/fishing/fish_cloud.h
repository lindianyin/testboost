#pragma once
#include <list>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/date_time.hpp>
class Point
{
public:
	Point()
	{

	}
	Point(float x1, float y1)
	{
		x = x1;
		y = y1;
	}

	inline Point operator+(const Point& pt)
	{
		return Point(x + pt.x, y + pt.y);
	}

	inline Point operator-(const Point& pt)
	{
		return Point(x - pt.x, y - pt.y);
	}

	Point& operator +=(const Point& pt)
	{
		*this = *this + pt;
		return *this;
	}

	inline Point operator*(float n)
	{
		return Point(x * n, y * n);
	}

	inline Point operator/(float n)
	{
		return Point(x / n, y / n);
	}
	inline Point normalize()
	{
		float length = get_length();
		if(length == 0.) return Point(1.f, 0);
		return *this / length;
	}

	inline float get_length() const {
		return sqrtf(x*x + y*y);
	};

	float		get_lengthsq()
	{
		return x*x +  y*y;
	}

	float		dot(const Point& pt)
	{
		return x * pt.x + y * pt.y;
	}

	float x, y;
};

class path_config
{
public:
	std::string		type_;
	int		rate_;
	std::list<Point> path_; 
};

class fish_config
{
public:
	std::string type_;
	std::string	id_;
	float		rate_;
	int		reward_;
	int		mv_step_;
	int		spawn_rate_;
	int		spawn_min_,	spwan_max_;
	int		szx_,		szy_;
	int		exp_;
	int		data_;		//0正常,1连锁，2全屏，3同类消除
	int		width_, height_;
	std::vector<path_config>	preset_path_;
	std::vector<std::string>	exclude_path_;
	fish_config();
};

typedef boost::shared_ptr<fish_config> fish_config_ptr;
class move_by_vector;
class move_speed_change;

class fish : public boost::enable_shared_from_this<fish>
{
public:
	int		iid_;
	std::string		strid_;
	std::string		type_;
	float		rate_;
	int		mv_step_;
	int		reward_;
	int		exp_;
	int		data_;		//0正常，1连锁，2，全屏，3同类消除  5，boss 10,消除同一波鱼
	std::list<Point> path_;
	std::list<Point> path_backup_;
	Point	pos_;
	boost::shared_ptr<move_by_vector> pmov_;
	boost::shared_ptr<move_speed_change> pchg_;

	long long	lottery_;
	int				rate_little_;
	int				reward_little_;
	int				die_enable_;
	float			acc_;
	int				width_, height_;
	long long	consume_gold_;		//已吸收多少货币
	int				lock_by_player_;
	bool	move_to_next();
	int		update(float dt);
	void	init(std::map<std::string, fish_config_ptr>& the_fish_config);
	fish();

	int m_groupId;
};


typedef boost::shared_ptr<fish> fish_ptr;
typedef boost::weak_ptr<fish> fish_wptr;

class cloud_config
{
public:
	std::string type_;
	int		rate_;
};

typedef boost::shared_ptr<cloud_config> cloud_config_ptr;

class move_by_vector : public boost::enable_shared_from_this<move_by_vector>
{
public:
	move_by_vector(Point& vm,	fish_ptr target):
		mov_vector_(vm.x, vm.y),moved_vector_(0,0) 
	{
		wtarget_ = target;
		acc_ = 1.0;
		pos_dest_ = target->pos_ + mov_vector_;
	}

	int		update(float time);
	void	speed_up(float acc);
	
	float get_mov_percent();
	float	get_acc(){
		return acc_;
	}
private:
	//根据向量进行移动
	Point mov_vector_, moved_vector_, pos_dest_;
	fish_wptr wtarget_;
	float acc_;
};

class move_speed_change
{
public:
	move_speed_change(float sp_dest, float duration, boost::shared_ptr<move_by_vector> mov);
	boost::shared_ptr<move_by_vector> the_move_;
	int		update(float time);

	int		get_timeleft();

protected:
	boost::posix_time::ptime start_tick_;
	float speed_now;
	float sp_dest_, sp_start_;
	float duration_;
};


class fishing_logic;
typedef boost::weak_ptr<fishing_logic> logic_wptr;
class fish_cloud
{
public:
	enum 
	{
		cloud_free,
		cloud_alone,
		cloud_combine,
	};
	virtual int update(float dt) = 0;
	bool					is_finished_;
	logic_wptr		the_logic_;
	int						cloud_type_;
	fish_cloud()
	{
		is_finished_ = false;
	}
};

class fish_cloud;
typedef boost::shared_ptr<fish_cloud> fcloud_ptr;


//随机鱼
class fish_freely_generator : public fish_cloud
{
public:
	boost::posix_time::ptime pt;
	int			update(float dt);

	static std::vector<fish_ptr>		spawn(
		std::map<std::string, fish_config_ptr>& the_fish_config_,
		std::map<std::string, path_config>& the_path_,
		bool random_startpos = true);
	fish_freely_generator()
	{
		cloud_type_ = cloud_free;

	}
};

//鱼群
class fish_cloud_generator : public fish_cloud
{
public:
	enum 
	{
		use_template_one_by_one,			//一次刷新间隔刷一条鱼，按模板顺序
		use_template_once_a_random,	//一次刷新间隔刷一条鱼，模板类随机
		use_template_once_for_all,		//一次刷新所有鱼
	};

	enum 
	{
		use_spawn_one_by_one,
		use_spawn_random,
		use_spawn_rect,
	};

	fish_cloud_generator(std::vector<std::string>& fish_templates,	//鱼模板,鱼的条数由它决定
		std::list<Point>& mov_path,	//运动轨迹
		std::vector<Point>& spawn_location,	//鱼群刷新点
		int		howto_use_fish_templates,	//怎么样使用鱼模板
		int		howto_use_spawn_location,			//怎么样使用鱼群刷新点
		float interval = 2.0,						//鱼的生成时间间隔
		float speed = 1.0,
		int		repeat = 1,
		Point offset = Point(0, 0),
		int		fish_repeat = 1);							//整体游动速度


	int			update(float dt);
	std::map<std::string, fish_config_ptr> the_fish_config_;
private:
	std::vector<std::string> fish_templates_;
	std::vector<Point> spawn_location_;
	std::list<Point> moving_path_;

	float		gernate_interval_;
	float		gernate_counter_;
	float		speed_;

	int			howto_use_spawn_location_;
	int			howto_use_fish_templates_;
	int			cloud_repeat_;
	int			fish_repeat_;
	int			generate_index_;
	Point		offset_;

	std::vector<fish_ptr>	generate();
};

//鱼群组合
class fish_cloud_generator_combine : public fish_cloud
{
public:
	int			update(float dt);
	std::vector<fcloud_ptr> clouds_;
	Point		offset_;
	fish_cloud_generator_combine()
	{
		cloud_type_ = cloud_combine;
	}
};
