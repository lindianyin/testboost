//////////////////////////////////////////////////////////////////////////
//implement simple xml parsor
//not for big xml document
//author: hjt@2011-4-11
//////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <vector>
#include <stdio.h>
#include "boost/shared_ptr.hpp"
#include "utility.h"

using namespace std;

#define tag_doc_s		"<?"
#define tag_doc_e		"?>"
#define tag_comment_s	"<!--"
#define tag_comment_e	"-->"
#define tag_node_s		"<"
#define tag_node_e		"</"
#define tag_special_s	"<!"
#define tag_special_e	">"
#define tag_cdata_s		"<![CDATA["
#define tag_cdata_e		"]]>"
namespace nsp_simple_xml
{
	struct xml_node
	{
		friend struct xml_doc;
		enum
		{
			node_t_node,	//�������һ�����ڵ�
			node_t_value,	//�������һ��ֵ
			node_t_cdata,	//�������һ��ֵ��������CDATA����
		};
		enum
		{
			parse_s_want_name,	//��Ҫ�������
			parse_s_want_value,	//��Ҫ�����ֵ
			parse_s_want_close,	//��Ҫ���պ�
			parse_s_complete,	//�������
		};

		string	name_;
		string	value_;
		string	attr_;
		vector<xml_node> child_lst;

		int		node_type;
		int		parse_state;//��㴦�ڵķ���״̬
		xml_node()
		{
			node_type = node_t_node;
			parse_state = parse_s_want_name;
		}
		void		reset()
		{
			node_type = node_t_node;
			parse_state = parse_s_want_name;
			name_ = value_ = attr_ = "";
			child_lst.clear();
		}
		template<class val_t>
		val_t		get_attr(const char* name)
		{
			vector<std::string> v;
			split_str(attr_.c_str(), attr_.length(), " ", v, false);
			if (!v.empty())	{
				for (int i = 0; i < (int)v.size();i++)
				{
					vector<std::string> v2;
					split_str(v[i].c_str(), v[i].length(), "=", v2, false);
					if (v2.size() == 2){
						if (v2[0] == name && v2[1].size() > 2){
							v2[1].pop_back(); v2[1].erase(v2[1].begin());
							return boost::lexical_cast<val_t>(v2[1]);
						}
					}
				}
			}
			return val_t();
		}

		template<class val_t>
		void		set_value(val_t v)
		{
			value_ = boost::lexical_cast<string>(v);
		}

		template<class val_t>
		val_t		get_value()
		{
			return boost::lexical_cast<val_t>(value_);
		}

		template<class val_t>
		val_t		get_value(const char* name, val_t def)
		{
			auto it = child_lst.begin(); 
			while (it != child_lst.end())
			{
				xml_node& nd = *it; it++;
				if (nd.name_ == name){
					return nd.get_value<val_t>();
				}
			}
			return def;
		}

		template<class val_t>
		bool		add_node(const char* name, val_t val)
		{
			xml_node n;
			n.name_ = name;
			n.set_value<val_t>(val);
			child_lst.push_back(n);
			return true;
		}
		bool		add_node(xml_node& add)
		{
			child_lst.push_back(add);
			return true;
		}
		//�õ��ӽ��
		xml_node*	get_node(const char* name);
	private:
		string		to_string(bool indent, string indent_str);
		xml_node*	get_node(vector<string>& path_vec );
	};
	//û���߳�ͬ����ֻ�������ֲ�����
	struct xml_doc
	{
		bool	parse(const char* xml_str, int len);
		bool	parse_from_file(const char* path = "e:\\test.xml");

		string&	to_string(bool indent = false, bool refresh = false);
		operator string ()
		{
			return to_string();
		}
		template<class val_t>
		bool		set_value(const char* path, val_t val)
		{
			xml_node* pn = get_node(path);
			if (!pn) return false;
			pn->set_value<val_t>(val);
			return true;
		}
		template<class val_t>
		bool		set_value(xml_node* pn, val_t val)
		{
			pn->set_value<val_t>(val);
			return true;
		}

		//���Ҳ�����������£���������Ҫ�ṩһ��Ĭ��ֵ,�Ա�֤����ȷ�ķ���ֵ
		template<class val_t>
		val_t		get_value(const char* path, val_t def)
		{
			xml_node* pn = get_node(path);
			if (!pn){
				cout<<"get xml node "<<path<<" failed" << endl;
				return def;
			}
			cout<<"get xml value path = "<<path<<" value = "<<pn->value_<< endl;
			return pn->get_value<val_t>();
		}

		//���Ҳ�����������£���������Ҫ�ṩһ��Ĭ��ֵ,�Ա�֤����ȷ�ķ���ֵ
		template<class val_t>
		val_t		get_value(xml_node* parent, const char* path, val_t def)
		{
			vector<string> vec_p;
			split_str(path, strlen(path), "/", vec_p, false);
			xml_node* pn = parent->get_node(vec_p);
			if (!pn){
				cout<<"get xml node "<<path<<" failed" << endl;
				return def;
			}
			return pn->get_value<val_t>();
		}

		//��ָ����·��������һ��xml�ӽ��
		template<class val_t>
		bool		add_node(const char* path, const char* name, val_t val)
		{
			xml_node* parent = get_node(path);
			xml_node n;
			n.name_ = name;
			n.set_value<val_t>(val);
			if(parent)	{
				parent->child_lst.push_back(n);
				return true;
			}
			return false;
		}

		//��ָ����xml���������һ���ӽ��
		bool		add_node(xml_node& parent, xml_node& add)
		{
			return parent.add_node(add);
		}
		//��root���������һ���ӽ��
		bool		add_node(xml_node& add)
		{
			return xml_root_.add_node(add);
		}
		//��ָ����xml���������һ���ӽ��
		template<class val_t>
		bool		add_node(xml_node& parent, const char* name, val_t val)
		{
			return parent.add_node(name, val);
		}

		xml_node*	get_node(const char* path)
		{
			vector<string> vec_p;
			split_str(path, strlen(path), "/", vec_p, false);
			return xml_root_.get_node(vec_p);
		}
		void		set_root_name(const char* root_name)
		{
			xml_root_.name_ = root_name;
		}
		xml_doc(const char* root_name)
		{
			set_root_name(root_name);
		}
		xml_doc()
		{
			purged_ = false;
			is_parsed_ = false;
		}
		bool		save_to_file(const char* path);
		bool		is_parsed()
		{
			return is_parsed_;
		}

	private:
		xml_node xml_root_;
		string	to_string_;
		bool		purged_;
		bool		is_parsed_;
		void	purge(vector<char>& data_read);
	};
};
typedef boost::shared_ptr<nsp_simple_xml::xml_doc> xml_doc_ptr;








