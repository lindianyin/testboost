#pragma once
#include "boost/smart_ptr.hpp"
#include <vector>

const int max_msg_size = 1024;

#define GET_CLSID(m) m##_id

#define DECLARE_MSG_CREATE()\
	msg_ptr ret_ptr;\
	switch (cmd)\
{

#define END_MSG_CREATE()\
}\

#define REGISTER_CLS_CREATE(cls_)\
	case GET_CLSID(cls_):\
	ret_ptr.reset(new cls_);\
	break

#define MSG_CONSTRUCT(msg)\
	msg()\
{\
	head_.cmd_ = msg##_id;\
	init();\
}

#define 	max_name  64
#define   max_guid  64

struct  msg_head
{
  unsigned short    len_;
  unsigned short    cmd_;
};

struct stream_buffer
{
	stream_buffer()
	{
		used_ = 0;
		data_len_ = 0;
		buffer_len_ = 0;
	}
	stream_buffer(boost::shared_array<char> dat, unsigned int dlen, unsigned int blen)
	{
		used_ = 0;
		data_len_ = dlen;
		data_ = dat;
		buffer_len_ = blen;
	}

	char*			data()
	{
		if (!data_.get()){
			return NULL;
		}
		return data_.get() + used_;
	}

	char*			buffer()
	{
		if (!data_.get()){
			return NULL;
		}
		return data_.get() + data_len_;
	}

	unsigned int	buffer_left()
	{
		return buffer_len_ - data_len_;
	}

	unsigned int	data_left()
	{
		return data_len_ - used_;
	}

	bool			is_complete()
	{
		return	data_left() == 0;
	}

	void			use_data(unsigned int len)
	{
		used_ += len;
	}

	void			use_buffer(unsigned int len)
	{
		data_len_ += len;
	}

	void			remove_used()
	{
		if (used_ > 0){
			memmove(data_.get(), data_.get() + used_, data_left());
			data_len_ = data_left();
			used_ = 0;
		}
	}
	
	unsigned int buffer_len()
	{
		return buffer_len_;
	}

private:
	unsigned int		used_;
	boost::shared_array<char>	data_;
	unsigned int		data_len_;
	unsigned int		buffer_len_;
};

template<class T> T swap_byte_order(T d)
{
#ifndef _NO_SWAP_ORDER
  char* p = (char*)&d;
  int s = sizeof(T);
  if (s <= 1) return d;
  char t;
  for (int i = 0; i < (s >> 1); i++)
  {
    t = p[i];
    p[i] = p[s - i - 1];
    p[s - i - 1] = t;
  }
#endif
  return d;
}

struct stream_base : public boost::enable_shared_from_this<stream_base>
{
public:
  stream_base()
  {
    rd_begin_ = NULL;
    rd_buff_len_ = 0;
    rd_fail_ = false;
		wt_begin_ = NULL;
		wt_fail_ = false;
		wt_buff_len_ = 0;
  }

	virtual int				write	(char*& data_s, unsigned int  l)
	{
		wt_begin_ = (char*)data_s;
		wt_buff_len_ = l;
		wt_fail_ = false;
		return 0;
	}
  virtual int				read	(const char*& data_s, unsigned int l)
  {
    rd_begin_ = (char*)data_s;
    rd_buff_len_ = l;
		rd_fail_ = false;
    return 0;
  }
  virtual ~stream_base(){}
  template<class T>
  T				read_data(const char*&data_s, bool swap_order = true)
  {
		if (rd_fail_) return;

    if((rd_buff_len_ - ((char*)data_s - rd_begin_)) < sizeof(T)){
			rd_fail_ = true;
			return T(0);
    }

		T t = *(T*)data_s;
		if (swap_order)	{
			t = swap_byte_order<T>(t);
		}
		data_s += sizeof(T);

		return t;
  }
  template<class T>
  void			read_data(T& t, const char*&data_s, bool swap_order = true)
  {
		if (rd_fail_) return;

    if((rd_buff_len_ - ((char*)data_s - rd_begin_)) < sizeof(T)){
			 rd_fail_ = true;
			 return;
    }

		t = *(T*)data_s;
		if (swap_order)	{
			t = swap_byte_order<T>(t);
		}
		data_s += sizeof(T);
  }

  template<class T>
  void			read_data(T* v, int element_count, const char*&data_s, bool swap_order = false)
  {
		if (rd_fail_) return;

		if(rd_buff_len_ - (data_s - rd_begin_) < int(sizeof(T)) * element_count){
			rd_fail_ = true;
			return;
		}

    T* vv = (T*)v;
    for (int i = 0 ; i < element_count; i++)
    {	
        T t = *(T*)data_s;
        if (swap_order)	{
          t = swap_byte_order<T>(t);
        }
        data_s += sizeof(T);
        *vv = t;
				//防止越界
				if((i + 1) < element_count)
				  vv++;
    }
  }

	template<> 
	void	read_data<char>(char* v, int element_count, const char*&data_s, bool swap_order)
	{
		if (rd_fail_) return;
		memset(v, 0, element_count);
		
		int buff_left = rd_buff_len_ - (data_s - rd_begin_);
		if (buff_left <= 0)	{
			rd_fail_ = true;
			return;
		}

		int len = 0;
		unsigned char len1 = *(unsigned char*) data_s;
		data_s++;
		if (len1 == 0xFF){
			len = int((unsigned char)(*data_s) << 8); data_s++;
			len |= (unsigned char)(*data_s);
			data_s++;
		}
		else{
			len = len1;
		}

		buff_left = rd_buff_len_ - (data_s - rd_begin_);

		if (buff_left < len){
			rd_fail_ = true;
			return;
		}

		if(len == 0) return;

		memcpy(v, data_s, len);

		//添加终结符.以防攻击
		if(len < element_count){
			v[len] = 0;
			data_s += len + 1;
		}
		else {
			v[element_count - 1] = 0;
			data_s += element_count;
		}
	}

	template<class T>
	void			read_data(std::vector<T>& v, const char*& data_s, int element_count, bool swap_order = false)
	{
		if (rd_fail_) return;

		if(rd_buff_len_ - (data_s - rd_begin_) < int(sizeof(T) * v.size())){
			rd_fail_ = true;
			return;
		}

		for (unsigned int i = 0 ; i < element_count; i++)
		{	
			T t = *(T*)data_s;
			if (swap_order)	{
				t = swap_byte_order<T>(t);
			}
			data_s += sizeof(T);
			v.push_back(t);
		}
	}

  template<class T>
  void			write_data(T v, char*& data_s, bool swap_order = true)
  {
		if (wt_fail_) return;
		if(wt_buff_len_ - (data_s - wt_begin_) < int(sizeof(T))){
			wt_fail_ = true;
			return;
		}
		if (swap_order)	{
				v = swap_byte_order<T>(v);
		}
     
    memcpy(data_s, &v, sizeof(T));
    data_s += sizeof(T);
  }

  template<class T>
  void			write_data(T* v, int element_count, char*& data_s, bool swap_order = false)
  {
		if (wt_fail_) return;
		if(wt_buff_len_ - (data_s - wt_begin_) < int(sizeof(T)) * element_count){
			wt_fail_ = true;
			return;
		}
    T* vv = (T*)v;
    for (int i = 0 ; i < element_count; i++)
    {
      T t = *vv;
      if (swap_order)	{
        t = swap_byte_order<T>(t);
      }
      memcpy(data_s, &t, sizeof(T));
      data_s += sizeof(T);
			//防止越界
			if((i + 1) < element_count)
				vv++;
    }
  }

	template<> 
	void	write_data<char>(char* v, int element_count, char*& data_s, bool swap_order)
	{
		if (wt_fail_) return;
		char* vv = v;
		//找出字符串长度
		int len = 0;
		for (;len < element_count; len++, vv++)
		{
			if(*vv == 0){	break; }
		}

		int buff_left = wt_buff_len_ - (data_s - wt_begin_);
		if (buff_left <= 0){
			wt_fail_ = true;
			return;
		}

		if (len > 0xFF){
			*(unsigned char*)data_s = 0xFF; data_s++;
			*data_s = ((len >> 8) & 0xFF); data_s++;
		}
		*(unsigned char*)data_s = (len & 0xFF); data_s++;

		if(len == 0) return;		//没内容

		buff_left = wt_buff_len_ - (data_s - wt_begin_);
		if (buff_left < len){
			wt_fail_ = true;
			return;
		}

		memcpy(data_s, v, len);
		//整个长度内未包含终结符，则强制最后一个字节为终结符
		if(len < element_count){ 
			data_s[len] = 0;
			data_s += len + 1;
		}
		//添加一个终结符
		else{
			data_s[element_count - 1] = 0;
			data_s += element_count;
		}
	}

	template<class T>
	void			write_data(std::vector<T>& v, char*& data_s, bool swap_order = false)
	{
		if (wt_fail_) return;
		if(wt_buff_len_ - (data_s - wt_begin_) < int(sizeof(T) * v.size())){
			wt_fail_ = true;
			return;
		}
		for (unsigned int i = 0 ; i < v.size(); i++)
		{
			T t = v[i];
			if (swap_order)	{
				t = swap_byte_order<T>(t);
			}
			memcpy(data_s, &t, sizeof(T));
			data_s += sizeof(T);
		}
	}
  bool				read_successful()	{return !rd_fail_;}
	bool				write_successful(){return !wt_fail_;}
private:
  char*					rd_begin_;	//开始读取位置,用来检测越界读
  int						rd_buff_len_;
  bool					rd_fail_;
	char*					wt_begin_;	//开始读取位置,用来检测越界读
	int						wt_buff_len_;
	bool					wt_fail_;
};

template<class remote_t>
class msg_base : public stream_base
{
public:
	typedef boost::shared_ptr<msg_base<remote_t>> msg_ptr;
	msg_head		head_;
	remote_t		from_sock_;

	int					read(const char*& data_s, unsigned int l)
	{
		stream_base::read(data_s, l);
		read_data(head_.len_, data_s);
		read_data(head_.cmd_, data_s);
		return 0;
	}

	int					write(char*& data_s, unsigned int l)
	{
		stream_base::write(data_s, l);
		write_data(head_.len_, data_s);
		write_data(head_.cmd_, data_s);
		return 0;
	}
	virtual	int handle_this() {return 0;};
	void			init()
	{

	}
};


struct none_socket{};

template<class socket_t>
class msg_common_reply: public msg_base<socket_t>
{
public:
	int				rp_cmd_;
	int				err_;
	char			des_[128];
	msg_common_reply()
	{
		head_.cmd_ = 1001;
		memset(des_, 0, 128);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data(rp_cmd_, data_s);
		read_data(err_, data_s);
		read_data<char>(des_, 128, data_s);
		return 0;
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(rp_cmd_, data_s);
		write_data(err_, data_s);
		write_data<char>(des_, 128, data_s);
		return 0;
	}
};

template<class socket_t>
class msg_common_reply_internal: public msg_common_reply<socket_t>
{
public:
	int		SN;
	msg_common_reply_internal()
	{
		head_.cmd_ = 6000;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_common_reply::read(data_s, l);
		read_data(SN, data_s);
		return 0;
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_common_reply::write(data_s, l);
		write_data(SN, data_s);
		return 0;
	}
};

template<class remote_t>
stream_buffer build_send_stream(msg_base<remote_t>* msg)
{
	char* pstrm = new char[max_msg_size];
	char* pc = pstrm;
	boost::shared_array<char> ret(pstrm);
	msg->write(pstrm, max_msg_size);
	unsigned short len = pstrm - pc;
	*(unsigned short*) pc = swap_byte_order(len);
	return stream_buffer(ret, len, len);
}

template<class remote_t>
void build_send_stream(msg_base<remote_t>* msg, stream_buffer& strm)
{
	char* pstrm = strm.buffer();
	char* pc = pstrm;
	msg->write(pstrm, strm.buffer_left());
	unsigned short len = pstrm - pc;
	strm.use_buffer(len);
	*(unsigned short*) pc = swap_byte_order(len);
}

template<class remote_t, class msg_t>
int			send_msg(remote_t psock, msg_t& msg,  bool close_this = false, bool sync = false)
{
	stream_buffer strm = build_send_stream(&msg);
	if (!sync){
		psock->add_to_send_queue(strm, close_this);
		return 0;
	}
	else{
		boost::system::error_code ec;
		psock->s.send(boost::asio::buffer(strm.data(), strm.data_left()), 0, ec);
		return ec.value();
	}
}

template<class remote_t>
int			send_msg(remote_t psock, stream_buffer strm, bool close_this = false, bool sync = false)
{
	if (!sync){
		psock->add_to_send_queue(strm, close_this);
		return 0;
	}
	else{
		boost::system::error_code ec;
		psock->s.send(boost::asio::buffer(strm.data(), strm.data_left()), 0, ec);
		return ec.value();
	}
}

template<class remote_t, class msg_creator_t>
boost::shared_ptr<msg_base<remote_t>>		pickup_msg_from_socket(remote_t psock, msg_creator_t creator)
{
	unsigned short n = 0;
	if (psock->pickup_data(&n, 2, false))
	{
		n = ntohs(n);
		char * c = new char[n];
		boost::shared_array<char> dat(c);
		if (!psock->pickup_data(c, n, true)) return boost::shared_ptr<msg_base<remote_t>>();

		msg_head head;
		head = *(msg_head*) c;
		head.cmd_ = ntohs(head.cmd_);
		boost::shared_ptr<msg_base<remote_t>> msg = creator(head.cmd_);
		if (msg.get()){
			msg->read((const char*&)c, n);
			msg->from_sock_ = psock;
			if (msg->read_successful()){
				return msg;
			}
			else{
				std::cout<<"msg read failed, cmd = "<< head.cmd_<<endl;
			}
		}
		else{
			std::cout<<"can't create msg, cmd = "<< head.cmd_<<endl;
		}
	}

	return boost::shared_ptr<msg_base<remote_t>>();
}

class nosocket{};
