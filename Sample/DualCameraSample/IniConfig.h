#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <map>

using namespace std;

class ININode
{
public:
	ININode(string root, string key, string value)
	{
		this->root = root;
		this->key = key;
		this->value = value;
	}
	string root;
	string key;
	string value;
};

class SubNode
{
public:
	void InsertElement(string key, string value)
	{
		sub_node.insert(pair<string, string>(key, value));
	}
	map<string, string> sub_node;
};


class IniConfig
{
public:
	IniConfig();
	~IniConfig();

public:
	int ReadINI(string path);
	int GetInt(string root, string key, int defaultValue);
	float GetFloat(string root, string key, float defaultValue);
	double GetDouble(string root, string key, double defaultValue);
	bool GetBool(string root, string key, bool defaultValue);
	
	string GetValue(string root, string key);
	vector<ININode>::size_type GetSize() { return map_ini.size(); }
	vector<ININode>::size_type SetValue(string root, string key, string value);
	int WriteINI(string path);
	void Clear() { map_ini.clear(); }
	void Travel();
private:
	map<string, SubNode> map_ini;
};

