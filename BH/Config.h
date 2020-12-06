#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <map>
#include <list>
#include "Common.h"

using namespace std;

struct Toggle {
	unsigned int toggle;
	bool state;
};

enum ConfigType {
	CTBoolean,
	CTString,
	CTInt,
	CTKey,
	CTToggle,
	CTArray,
	CTAssoc,
	CTAssocInt,
	CTAssocBool
};

class ConfigEntry {
public:
	ConfigType type;
	std::wstring key;
	std::wstring value;
	std::wstring comment;
	int line;
	void* pointer;
	Toggle* toggle;


};

inline bool operator< (const ConfigEntry& lhs, const ConfigEntry& rhs) {
	// std::tuple's lexicographic ordering does all the actual work for you
	// and using std::tie means no actual copies are made
	return std::tie(lhs.value, lhs.key) < std::tie(rhs.value, rhs.key);
}


class Config {
private:
	std::string configName;
	std::map<std::wstring, ConfigEntry> contents;
	vector<pair<wstring, wstring>> orderedKeyVals;

	static bool HasChanged(ConfigEntry entry, wstring& value);
	static bool StringToBool(std::wstring input);
public:
	Config(std::string name) : configName(name) {};

	//Parse the config file and store results
	bool Parse();
	bool Write();
	std::list<std::wstring> GetDefinedKeys();

	std::string GetConfigName();
	void SetConfigName(std::string name);

	//Functions to read values from the configuration
	bool				ReadBoolean(std::wstring key, bool& value);
	std::wstring			ReadString(std::wstring key, std::wstring& value);
	int					ReadInt(std::wstring key, int& value);
	unsigned int    	ReadInt(std::wstring key, unsigned int& value);
	unsigned int		ReadKey(std::wstring key, std::wstring toggle, unsigned int &value);
	Toggle				ReadToggle(std::wstring key, std::wstring toggle, bool defaultState, Toggle& value);
	std::vector<wstring> ReadArray(std::wstring key, std::vector<wstring>& value);
	map<wstring, wstring> ReadAssoc(std::wstring key, std::map<wstring, wstring>& value);
	map<wstring, unsigned int> ReadAssoc(std::wstring key, std::map<wstring, unsigned int>& value);
	map<wstring, bool> ReadAssoc(std::wstring key, std::map<wstring, bool>& value);
	vector<pair<wstring, wstring>> ReadMapList(std::wstring key, vector<pair<wstring,wstring>>& value);
};