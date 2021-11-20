#include "IniConfig.h"
#include <stdlib.h>
#include <errno.h>

//#define INIDEBUG

IniConfig::IniConfig()
{
}


IniConfig::~IniConfig()
{
}

string &TrimString(string &str)
{
	string::size_type pos = 0;
	while (str.npos != (pos = str.find(" ")))
		str = str.replace(pos, pos + 1, "");
	return str;
}

int IniConfig::ReadINI(string path)
{
	ifstream in_conf_file(path.c_str());
	if (!in_conf_file) return -errno;
	string str_line = "";
	string str_root = "";
	vector<ININode> vec_ini;
	while (getline(in_conf_file, str_line))
	{
		string::size_type left_pos = 0;
		string::size_type right_pos = 0;
		string::size_type equal_div_pos = 0;
		string str_key = "";
		string str_value = "";
		if ((str_line.npos != (left_pos = str_line.find("["))) && (str_line.npos != (right_pos = str_line.find("]"))))
		{
			//cout << str_line.substr(left_pos+1, right_pos-1) << endl;
			str_root = str_line.substr(left_pos + 1, right_pos - 1);
		}

		if (str_line.npos != (equal_div_pos = str_line.find("=")))
		{
			str_key = str_line.substr(0, equal_div_pos);
			str_value = str_line.substr(equal_div_pos + 1, str_line.size() - 1);
			str_key = TrimString(str_key);
			str_value = TrimString(str_value);
			//cout << str_key << "=" << str_value << endl;
		}

		if ((!str_root.empty()) && (!str_key.empty()) && (!str_value.empty()))
		{
			ININode ini_node(str_root, str_key, str_value);
			vec_ini.push_back(ini_node);
			//cout << vec_ini.size() << endl;
		}
	}
	in_conf_file.close();
	in_conf_file.clear();

	//vector convert to map
	map<string, string> map_tmp;
	for (vector<ININode>::iterator itr = vec_ini.begin(); itr != vec_ini.end(); ++itr)
	{
		map_tmp.insert(pair<string, string>(itr->root, ""));
	}
	for (map<string, string>::iterator itr = map_tmp.begin(); itr != map_tmp.end(); ++itr)
	{
#ifdef INIDEBUG
		cout << "root： " << itr->first << endl;
#endif	//INIDEBUG
		SubNode sn;
		for (vector<ININode>::iterator sub_itr = vec_ini.begin(); sub_itr != vec_ini.end(); ++sub_itr)
		{
			if (sub_itr->root == itr->first)
			{
#ifdef INIDEBUG
				cout << "key-value： " << sub_itr->key << "=" << sub_itr->value << endl;
#endif	//INIDEBUG
				sn.InsertElement(sub_itr->key, sub_itr->value);
			}
		}
		map_ini.insert(pair<string, SubNode>(itr->first, sn));
	}
	return 1;
}

string IniConfig::GetValue(string root, string key)
{
	map<string, SubNode>::iterator itr = map_ini.find(root);
	if (map_ini.end() == itr)
	{
		return "";
	}

	map<string, string>::iterator sub_itr = itr->second.sub_node.find(key);
	if (itr->second.sub_node.end() != sub_itr && sub_itr!= itr->second.sub_node.end())
	{
		if (!(sub_itr->second).empty())
			return sub_itr->second;
	}
	return "";
}

int IniConfig::GetInt(string root, string key, int defaultValue)
{
	int ret = defaultValue;
	string valueStr = GetValue(root, key);
	if (false == valueStr.empty())
	{
		ret = atoi(valueStr.c_str());
	}

	return ret;
}

float IniConfig::GetFloat(string root, string key, float defaultValue)
{
	float ret = defaultValue;
	string valueStr = GetValue(root, key);
	if (false == valueStr.empty())
	{
		ret = strtof(valueStr.c_str(), NULL);
	}

	return ret;
}

double IniConfig::GetDouble(string root, string key, double defaultValue)
{
	double ret = defaultValue;
	string valueStr = GetValue(root, key);
	if (false == valueStr.empty())
	{
		ret = strtod(valueStr.c_str(), NULL);
	}

	return ret;
}


bool IniConfig::GetBool(string root, string key, bool defaultValue)
{
	bool ret = defaultValue;
	string valueStr = GetValue(root, key);
	if (false == valueStr.empty())
	{
		if (0 == valueStr.compare("1"))
		{
			ret = true;
		}
		else if(0 == valueStr.compare("TRUE"))
		{
			ret = true;
		}
		else if(0 == valueStr.compare("true"))
		{
			ret = true;
		}
		else
		{
			ret = false;
		}
		
	}

	return ret;
}

int IniConfig::WriteINI(string path)
{
	ofstream out_conf_file(path.c_str());
	if (!out_conf_file)
		return -1;
	//cout << map_ini.size() << endl;
	for (map<string, SubNode>::iterator itr = map_ini.begin(); itr != map_ini.end(); ++itr)
	{
		//cout << itr->first << endl;
		out_conf_file << "[" << itr->first << "]" << endl;
		for (map<string, string>::iterator sub_itr = itr->second.sub_node.begin(); sub_itr != itr->second.sub_node.end(); ++sub_itr)
		{
			//cout << sub_itr->first << "=" << sub_itr->second << endl;
			out_conf_file << sub_itr->first << "=" << sub_itr->second << endl;
		}
	}

	out_conf_file.close();
	out_conf_file.clear();

	return 1;
}

vector<ININode>::size_type IniConfig::SetValue(string root, string key, string value)
{
	map<string, SubNode>::iterator itr = map_ini.find(root);
	if (map_ini.end() != itr)
	{
		//itr->second.sub_node.insert(pair<string, string>(key, value));
		itr->second.sub_node[key] = value;
	}
	else
	{
		SubNode sn;
		sn.InsertElement(key, value);
		map_ini.insert(pair<string, SubNode>(root, sn));
	}

	return map_ini.size();
}

void IniConfig::Travel()
{
	for (map<string, SubNode>::iterator itr = this->map_ini.begin(); itr != this->map_ini.end(); ++itr)
	{
		//root
		cout << "[" << itr->first << "]" << endl;
		for (map<string, string>::iterator itr1 = itr->second.sub_node.begin(); itr1 != itr->second.sub_node.end();
			++itr1)
		{
			cout << "    " << itr1->first << " = " << itr1->second << endl;
		}
	}

}
