#include "simple_xml_parse.h"

using namespace nsp_simple_xml;
static bool		next_tag_is(char*& p, const char* tag)
{
	char *tp = p, *tt = (char*)tag;
	while (*tp != 0 && *tt != 0)
	{
		if (*tp != *tt)	{
			return false;
		}
		tp++; tt++;
	}
	p = tp;
	return true;
}
//形如 <node ">" 或 <node "/>"
static int			seek_to_name_e(char*& p, char* end_p, string* name = NULL, string* attr = NULL)
{
	char buff[1024];
	char* tp = p;
	char* sep = NULL;
	int seek_ret = 0;
	while(*tp > 0 && tp < end_p)
	{
		if (*tp == '/')	{
			seek_ret |= 1;	//有'/'
		}
		else if (*tp =='>'){
			tp++;
			seek_ret |= 2;	//有'>'
			break;
		}
		else if (*tp == ' '){
			if (!sep) sep = tp;
		}
		else{
			seek_ret = 0;
		}
		tp++;
	}
	if (seek_ret & 2)
	{
		//有属性
		if (sep && (sep - p < 1023)){
			memcpy(buff, p, sep - p);
			buff[sep - p] = 0;
			*name = buff;
			sep++;	//跳过空格
			//如果是</>
			if (attr &&(seek_ret & 1)) {
				memcpy(buff, sep, tp - sep - 2);
				buff[tp - sep - 2] = 0;
			}
			else if(attr) {
				memcpy(buff, sep, tp - sep - 1);
				buff[tp - sep - 1] = 0;
			}
			if(attr) *attr = buff;
		}
		//没有属性
		else if ((tp - p < 1023)){
			//如果是</>
			if (name &&(seek_ret & 1)){
				memcpy(buff, p, tp - p - 2);
				buff[tp - p - 2] = 0;
			}
			else if (name){
				memcpy(buff, p, tp - p - 1);
				buff[tp - p - 1 ] = 0;
			}
			if(name) *name = buff;
		}
		p = tp;
	}
	return seek_ret;
}
static bool		seek_to_tag_e(char*& p, char* end_p, const char* tag, string* ret = NULL)
{
	char buff[1024];
	char *tp = p, *tt = (char*)tag;
	bool finded = false;
	while (tp < end_p && !finded)
	{
		if (*tp == *tt){
			tt++;
		}
		else if(*tt != 0)
			tt = (char*) tag;
		else if (*tt == 0){
			finded = true;
			break;
		}
		tp++;
	}
	if(finded && (tp - p < 1023)) {
		if(ret) {
			int l = tp - p - strlen(tag);
			memcpy(buff, p, l);
			buff[l] = 0;
			*ret = buff;
		}
		p = tp;
	}
	return finded;
}
//侦测元素状态机
struct xml_parse_state
{
	char*	xml_steam_s_, * xml_steam_e_;
	xml_node& pn_;
	virtual bool		update() = 0;
	virtual void		enter(){};
	virtual void		leave(){};
	xml_parse_state(xml_node& p):pn_(p){}
};

struct xml_parse_detect : public xml_parse_state
{
	xml_parse_detect(xml_node& p):xml_parse_state(p){error_near_ = nullptr;}

	bool				change_to_state(xml_parse_state& st)
	{
		st.xml_steam_s_ = xml_steam_s_;
		st.xml_steam_e_ = xml_steam_e_;
		if(!st.update()){
			error_near_ = st.xml_steam_s_;
			return false;
		}
		xml_steam_s_ = st.xml_steam_s_;
		return true;
	}
	virtual bool		update();
	char* error_near_;
};
struct xml_parse_skip_head : public xml_parse_state
{
	xml_parse_skip_head(xml_node& p):xml_parse_state(p){}
	virtual bool		update()
	{
		if (!seek_to_tag_e(xml_steam_s_, xml_steam_e_, tag_doc_e))
			return false;
		return true;	
	}
};
struct xml_parse_skip_comment : public xml_parse_state
{
	xml_parse_skip_comment(xml_node& p):xml_parse_state(p){}
	virtual bool		update()
	{
		if (!seek_to_tag_e(xml_steam_s_, xml_steam_e_, tag_comment_e))
			return false;
		return true;
	}
};
struct xml_parse_skip_spec : public xml_parse_state
{
	xml_parse_skip_spec(xml_node& p):xml_parse_state(p){}
	virtual bool		update()
	{
		if (!seek_to_tag_e(xml_steam_s_, xml_steam_e_, tag_special_e))
			return false;
		return true;
	}

};
struct xml_parse_read_name : public xml_parse_state
{
	xml_parse_read_name(xml_node& p):xml_parse_state(p){}
	virtual bool		update()
	{
		int seek_ret = seek_to_name_e(xml_steam_s_,xml_steam_e_, &pn_.name_, &pn_.attr_);
		if (seek_ret == 0){
			return false;
		}
		//如果是空结点
		if (seek_ret & 1){
			pn_.parse_state = pn_.parse_s_complete;
		}
		else 
			pn_.parse_state = pn_.parse_s_want_value;
		return true;
	}
};
struct xml_parse_read_value : public xml_parse_state
{
	xml_parse_read_value(xml_node& p):xml_parse_state(p){}
	virtual bool		update()
	{
		string end_n_str = "</" + pn_.name_ + ">";
		if (!seek_to_tag_e(xml_steam_s_, xml_steam_e_, end_n_str.c_str(), &pn_.value_))
			return false;
		pn_.parse_state = pn_.parse_s_complete;
		return true;
	}
};

bool	xml_parse_detect::update()
{
	bool continue_parse = false;
	//make sure every loop forwards xml_steam_s_,
	//else there may be a dead loop.
	do 
	{
		string end_n_str = "</" + pn_.name_ + ">";
		continue_parse = false;
		//skip <? ?>
		if (next_tag_is(xml_steam_s_, tag_doc_s)){
			xml_parse_skip_head st(pn_);
			if(!change_to_state(st))
				return false;
			continue_parse = true;
		}
		//skip comment <!--  --!>
		else if (next_tag_is(xml_steam_s_, tag_comment_s)){
			xml_parse_skip_comment st(pn_);
			if(!change_to_state(st))
				return false;
			continue_parse = true;
		}
		//skip ' '
		else if(next_tag_is(xml_steam_s_, " ")) {
			continue_parse = true;
		}
		//skip '\r'
		else if(next_tag_is(xml_steam_s_, "\r")) {
			continue_parse = true;
		}
		//skip '\n'
		else if(next_tag_is(xml_steam_s_, "\n")) {
			continue_parse = true;
		}
		//skip '\t'
		else if(next_tag_is(xml_steam_s_, "\t")) {
			continue_parse = true;
		}
		//read <!CDATA[[ d ]]>
		else if (next_tag_is(xml_steam_s_, tag_cdata_s))
		{
			if (!seek_to_tag_e(xml_steam_s_, xml_steam_e_, tag_cdata_e, &pn_.value_))
				return false;
			pn_.node_type = pn_.node_t_cdata;
			if (!seek_to_tag_e(xml_steam_s_, xml_steam_e_, end_n_str.c_str()))
				return false;
			pn_.parse_state = pn_.parse_s_complete;
		}
		//skip <! !>
		else if (next_tag_is(xml_steam_s_, tag_special_s)){
			xml_parse_skip_spec st(pn_);
			if(!change_to_state(st))
				return false;
			continue_parse = true;
		}
		//node parse complete
		else if (next_tag_is(xml_steam_s_, end_n_str.c_str())){
			pn_.parse_state = pn_.parse_s_complete;
		}
		//new node start
		else if (next_tag_is(xml_steam_s_, tag_node_s)){
			//read node name
			if (pn_.parse_state == pn_.parse_s_want_name) {
				xml_parse_read_name st(pn_);
				if(!change_to_state(st))
					return false;
				//it's a empty node, read complete
				if (pn_.parse_state == pn_.parse_s_complete)
					return true;
			}
			//new node start, it's a child of pn_;
			else if (pn_.parse_state == pn_.parse_s_want_value)	{
				xml_node tmp_node;
				xml_parse_detect st_dec(tmp_node);
				xml_steam_s_--;
				//read the child node
				if(!change_to_state(st_dec))
					return false;
				pn_.child_lst.push_back(tmp_node);
			}
			continue_parse = true;
		}
		//read node value
		else {
			xml_parse_read_value st(pn_);
			if(!change_to_state(st))
				return false;
		}
	} while (continue_parse && xml_steam_s_ < xml_steam_e_);

	return pn_.parse_state == pn_.parse_s_complete;
}

bool nsp_simple_xml::xml_doc::parse( const char* xml_str ,int len)
{
	if(!purged_){
		vector<char> v;
		v.insert(v.begin(), xml_str, xml_str + len);
		v.push_back(0);
		purge(v);
		return parse(v.data(), v.size());
	}
	else{
		xml_root_.reset();
		xml_parse_detect st(xml_root_);
		st.xml_steam_s_ = (char*) xml_str;
		st.xml_steam_e_ = (char*) xml_str + len;
		if (!st.update()){
			return false;
		}
	}
	is_parsed_ = true;
	return true;
}

bool nsp_simple_xml::xml_doc::parse_from_file( const char* path /*= "e:\\test.xml"*/ )
{
	vector<char> data_read;
	FILE* fp = fopen(path, "rb");
	if(!fp) return false;
	char c[128];
	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	while (data_read.size() < len)
	{
		int s = fread(c, sizeof(char), 128, fp);
		if (s <= 0)		break;
		data_read.insert(data_read.end(), c, c + s);
	}

	fclose(fp);
	if (!data_read.empty())
	{
		data_read.push_back(0);
		purge(data_read);
		return parse(data_read.data(), data_read.size());
	}
	return false;
}

std::string& nsp_simple_xml::xml_doc::to_string( bool indent, bool refresh)
{
	if (to_string_ == "" || refresh){
		to_string_ = "<?xml version=\"1.0\" encoding =\"UTF-8\"?>";
		to_string_ += (indent ? "\r\n" : "");
		to_string_ += xml_root_.to_string(indent, "");
	}
	return to_string_;
}

bool nsp_simple_xml::xml_doc::save_to_file( const char* path )
{
	FILE* fp = fopen(path, "w");
	if (fp)	{
		string& xml_s = to_string(true);
		char *cp = (char*)xml_s.c_str();
		while (strlen(cp) > 0)
		{
			size_t s = fwrite(cp, sizeof(const char), strlen(cp), fp);
			cp += s;
		}
		fclose(fp);
		return true;
	}
	return false;
}

void nsp_simple_xml::xml_doc::purge(vector<char>& data_read)
{
	if(data_read.size() > 3){
		//utf-8 标记，跳过
		if((unsigned char)data_read[0] == 0xEF && 
			(unsigned char) data_read[1] == 0xBB && 
			(unsigned char) data_read[2] == 0xBF)
			data_read.erase(data_read.begin(), data_read.begin() + 3);
		//unicode标记，重整
		else if ((unsigned char)data_read[0] == 0xFF && 
			(unsigned char) data_read[1] == 0xFE)
		{
			vector<char> v;
			data_read.erase(data_read.begin(), data_read.begin() + 2);
			for (int i = 0; i < (int)data_read.size(); i += 2)
			{
				unsigned short c = *(unsigned short*)(data_read.data() + i);
				if (c < 255){
					v.push_back((char)c);
				}
				else{
					v.push_back(*(data_read.data() + i));
					v.push_back(*(data_read.data() + i + 1));
				}
			}
			data_read.clear();
			data_read = v;
		}
	}
	purged_ = true;
}

std::string nsp_simple_xml::xml_node::to_string( bool indent, string indent_str )
{
	string ret;
	if(!indent) indent_str = "";
	if(!child_lst.empty()){
		ret += indent_str + "<" + name_+ (attr_ == "" ? "" : " " + attr_ )+">" + (indent ? "\r\n" : "");
		for (int i = 0 ; i < (int)child_lst.size(); i++)
		{
			ret += child_lst[i].to_string(indent, indent_str + "    " );
		}
		ret += indent_str + "</" + name_+ ">" + (indent ? "\r\n" : "");
	}
	else{
		if (value_ == ""){
			ret += indent_str + "<" + name_+ (attr_ == "" ? "" : " " + attr_ ) +"/>" + (indent ? "\r\n" : "");
		}
		else {
			if (node_type == node_t_cdata) {
				ret += indent_str + "<" + name_+ (attr_ == "" ? "" : " " + attr_ ) +">";
				ret += "<![CDATA[" + value_ + "]]>";
				ret += "</" + name_+">" + (indent ? "\r\n" : "");
			}
			else{
				ret += indent_str + "<" + name_+ (attr_ == "" ? "" : " " + attr_ ) +">";
				ret += value_;
				ret += "</" + name_+">" + (indent ? "\r\n" : "");
			}
		}
	}
	return ret;
}

xml_node* nsp_simple_xml::xml_node::get_node( vector<string>& vec_path )
{
	if (vec_path.empty()) return NULL;
	string n = vec_path.front();
	if (n != name_) return NULL;

	vec_path.erase(vec_path.begin());
	if (vec_path.empty()) return this;

	vector<xml_node>::iterator it = child_lst.begin();
	while (it != child_lst.end()) {
		xml_node* pn = (*it).get_node(vec_path);
		if (pn) 
			return pn;
		it++;
	}
	return NULL;
}

xml_node* nsp_simple_xml::xml_node::get_node( const char* name )
{
	for (int i = 0; i < (int)child_lst.size(); i++)
	{
		if(child_lst[i].name_ == name)
			return &child_lst[i];
	}
	return NULL;
}
