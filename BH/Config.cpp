#include "Config.h"
#include "BH.h"
#include <algorithm>
#include <sstream>
#include <codecvt>

/* Parse()
Parse the configuration file and stores the results in a key->value pair.
Can be called multiple times so you can reload the configuration.
*/
bool Config::Parse() {

	//If this is a config with a pre-set of values, can't parse!
	if (configName.length() == 0)
		return false;

	//Open the configuration file
	ifstream file(BH::path + configName);// , wifstream::binary);
	//file.imbue(locale(file.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
	if (!file.is_open())
		return false;

	//Lock Critical Section then wipe contents incase we are reloading.
	contents.erase(contents.begin(), contents.end());
	orderedKeyVals.clear();

	//Begin to loop the configuration file one line at a time.
	std::string line;
	int lineNo = 0;
	while (std::getline(file, line)) {
		lineNo++;
		std::wstring comment;
		//Remove any comments from the config
		if (line.find("//") != string::npos) {
			comment = AnsiToUnicode(line.substr(line.find("//")).c_str());
			line = line.erase(line.find("//"));
		}

		//Insure we have something in the line now.
		if (line.length() == 0)
			continue;

		//Grab the Key and Value

		ConfigEntry entry;
		entry.line = lineNo;
		entry.key = Trim(AnsiToUnicode(line.substr(0, line.find_first_of(":")).c_str()));
		entry.value = Trim(AnsiToUnicode(line.substr(line.find_first_of(":") + 1).c_str()));

		entry.comment = AnsiToUnicode(line.substr(line.find_first_of(":") + 1, line.find(UnicodeToAnsi(entry.value.c_str())) - line.find_first_of(":") - 1).c_str());
		entry.pointer = NULL;

		//Store them!
		contents.insert(pair<wstring, ConfigEntry>(entry.key, entry));
		orderedKeyVals.push_back(pair<wstring, wstring>(entry.key, entry.value));
	}
	file.close();
	return true;
}

bool Config::Write() {
	//If this is a config with a pre-set of values, can't parse!
	if (configName.length() == 0)
		return false;

	
	//Open the configuration file
	fstream file(BH::path + configName);
	if (!file.is_open())
		return false;

	//Read in the configuration value
	vector<string> configLines;
	char line[2048];
	while (!file.eof()) {
		file.getline(line, 2048);
		configLines.push_back(line);
	}
	file.close();
	
	map<ConfigEntry, string> changed;
	for (map<wstring, ConfigEntry>::iterator it = contents.begin(); it != contents.end(); ++it) {
		string newValue;
		if (!HasChanged((*it).second, newValue))
			continue;
		pair<ConfigEntry, string> change;
		change.first = (*it).second;
		change.second = newValue.c_str();
		changed.insert(change);
	}
	
	if (changed.size() == 0)
		return true;
	
	
	for (vector<string>::iterator it = configLines.begin(); it < configLines.end(); it++) {
		//Remove any comments from the config
		string comment;
		if ((*it).find_first_of("//") != string::npos) {
			comment = (*it).substr((*it).find_first_of("//"));
			(*it) = (*it).erase((*it).find_first_of("//"));
		}

		//Insure we have something in the line now.
		if ((*it).length() == 0) {
			*it = comment;
			continue;
		}

		string key = Trim((*it).substr(0, (*it).find_first_of(":")));
		*it = *it + comment;

		for (map<ConfigEntry, string>::iterator cit = changed.begin(); cit != changed.end(); ++cit)
		{
			if ((*cit).first.key.compare(AnsiToUnicode(key.c_str())) != 0)
				continue;

			if ((*cit).second.size() == 0)
			{
				*it = "//Purge";
				contents[AnsiToUnicode(key.c_str())].value = L"";
				changed.erase((*cit).first);
				break;
			}

			stringstream newLine;
			newLine << key << ":" << UnicodeToAnsi((*cit).first.comment.c_str()) << (*cit).second << comment;
			*it = newLine.str();
			contents[AnsiToUnicode(key.c_str())].value = AnsiToUnicode((*cit).second.c_str());

			changed.erase((*cit).first);
			break;
		}
	}

	for (map<ConfigEntry, string>::iterator cit = changed.begin(); cit != changed.end(); ++cit)
	{
		stringstream newConfig;

		newConfig << UnicodeToAnsi((*cit).first.key.c_str()) << ": " << (*cit).second;

		configLines.push_back(newConfig.str());
	}
	
	ofstream outFile(BH::path + configName);
	for (vector<string>::iterator it = configLines.begin(); it < configLines.end(); it++) {
		if ((*it).compare("//Purge") == 0)
			continue;

		if (std::next(it) == configLines.end())
			outFile << (*it);
		else
			outFile << (*it) << endl;
	}
	outFile.close();

	return true;
}

std::string Config::GetConfigName() {
	return BH::path + configName;
}

void Config::SetConfigName(std::string name) {
	configName = name;
}

/* ReadBoolean(std::string key, bool value)
*	Reads in a boolean from the key-pair.
*/
bool Config::ReadBoolean(std::wstring key, bool& value) {
	//Check if configuration value exists
	if (contents.find(key) == contents.end()) {
		contents[key].key = key;
		contents[key].value = value;
	}

	contents[key].type = CTBoolean;
	contents[key].pointer = (void*)&value;

	//Convert string to boolean
	const wchar_t* szValue = contents[key].value.c_str();
	if ((_wcsicmp(szValue, L"1") == 0) || (_wcsicmp(szValue, L"y") == 0) || (_wcsicmp(szValue, L"yes") == 0) || (_wcsicmp(szValue, L"true") == 0))
		value = true;
	else
		value = false;
	return value;
}

/* ReadInt(std::string key, int value)
*	Reads in a decimal or hex(which is converted to decimal) from the key-pair.
*/
int Config::ReadInt(std::wstring key, int& value) {
	//Check if configuration value exists
	if (contents.find(key) == contents.end()) {
		contents[key].key = key;
		contents[key].value = value;
	}

	contents[key].type = CTInt;
	contents[key].pointer = (void*)&value;

	if (!contents[key].value.find(L"0x")) {
		from_wstring<int>(value, contents[key].value, std::hex);
	}
	else {
		from_wstring<int>(value, contents[key].value, std::dec);
	}
	return value;
}

/* ReadInt(std::string key, int value)
*	Reads in a decimal or hex(which is converted to decimal) from the key-pair.
*/
unsigned int Config::ReadInt(std::wstring key, unsigned int& value) {
	//Check if configuration value exists
	if (contents.find(key) == contents.end()) {
		contents[key].key = key;
		contents[key].value = value;
	}

	contents[key].type = CTInt;
	contents[key].pointer = &value;

	if (!contents[key].value.find(L"0x")) {
		from_wstring<unsigned int>(value, contents[key].value, std::hex);
	}
	else {
		from_wstring<unsigned int>(value, contents[key].value, std::dec);
	}
	return value;
}

std::wstring Config::ReadString(std::wstring key, std::wstring &value) {
	//Check if configuration value exists
	if (contents.find(key) == contents.end())
	{
		contents[key].key = key;
		contents[key].value = value;
	}

	contents[key].pointer = &value;
	contents[key].type = CTString;

	value = contents[key].value;
	return value;
}

/* ReadToggle(std::string key, std::string toggle, bool state)
*	Reads in a toggle from the key->pair
*	Config Example:
*		Key: True, VK_A
*/
Toggle Config::ReadToggle(std::wstring key, std::wstring toggle, bool state, Toggle& value) {
	//Check if configuration value exists.
	Toggle ret;
	if (contents.find(key) == contents.end()) {
		contents[key].key = key;
		contents[key].value = ((state) ? L"True, " : L"False, ") + toggle;
	}

	contents[key].toggle = &value;
	contents[key].type = CTToggle;

	ret.toggle = GetKeyCode(UnicodeToAnsi(Trim(contents[key].value.substr(contents[key].value.find_first_of(L",") + 1)).c_str())).value;
	ret.state = StringToBool(Trim(contents[key].value.substr(0, contents[key].value.find_first_of(L","))));

	value = ret;
	return ret;
}

/* ReadKey(std::string key, std::string toggle)
*	Reads in a key from the key->pair.
*/
unsigned int Config::ReadKey(std::wstring key, std::wstring toggle, unsigned int &value) {
	//Check if configuration value exists.
	if (contents.find(key) == contents.end()) {
		contents[key].key = key;
		contents[key].value = toggle;
	}

	contents[key].pointer = &value;
	contents[key].type = CTKey;

	//Grab the proper key code and make s ure it's valid
	KeyCode ret = GetKeyCode(UnicodeToAnsi(contents[key].value.c_str()));
	if (ret.value == 0) {
		value = GetKeyCode(UnicodeToAnsi(toggle.c_str())).value;
	}
	value = ret.value;

	return ret.value;
}

/* ReadArray(std::string key)
*	Reads in a index-based array from the array
*/
vector<wstring> Config::ReadArray(std::wstring key, vector<wstring> &value) {
	int currentIndex = 0;
	value.clear();
	while (true) {
		wstringstream index;
		index << currentIndex;
		wstring currentKey = key + L"[" + index.str() + L"]";
		if (contents.find(currentKey) == contents.end())
			break;
		value.push_back(contents[currentKey].value);
		contents[currentKey].pointer = &value;
		contents[currentKey].type = CTArray;
		currentIndex++;
	}
	return value;
}

/* ReadGroup(std::string key)
*	Reads in a map from the key->pair
*	Config Example:
*		Value[Test]: 0
*		Value[Pickles]: 1
*/
map<wstring, wstring> Config::ReadAssoc(std::wstring key, map<wstring, wstring> &value) {

	for (map<wstring, ConfigEntry>::iterator it = contents.begin(); it != contents.end(); it++) {
		if (!(*it).first.find(key + L"[")) {
			pair<wstring, wstring> assoc;
			//Pull the value from between the []'s
			assoc.first = (*it).first.substr((*it).first.find(L"[") + 1, (*it).first.length() - (*it).first.find(L"[") - 2);
			// Check if key is already defined in map
			if (value.find(assoc.first) == value.end()) {
				assoc.second = (*it).second.value;
				value.insert(assoc);
			}
			else {
				value[key] = (*it).second.value;
			}

			(*it).second.pointer = &value;
			(*it).second.type = CTAssoc;
		}
	}

	return value;
}

map<wstring, bool> Config::ReadAssoc(std::wstring key, map<wstring, bool> &value) {

	for (map<wstring, ConfigEntry>::iterator it = contents.begin(); it != contents.end(); it++) {
		if (!(*it).first.find(key + L"[")) {
			pair<wstring, bool> assoc;
			assoc.first = (*it).first.substr((*it).first.find(L"[") + 1, (*it).first.length() - (*it).first.find(L"[") - 2);
			transform(assoc.first.begin(), assoc.first.end(), assoc.first.begin(), ::tolower);

			if (value.find(assoc.first) == value.end()) {
				assoc.second = StringToBool((*it).second.value);
				value.insert(assoc);
			}
			else {
				value[assoc.first] = StringToBool((*it).second.value);
			}

			(*it).second.pointer = &value;
			(*it).second.type = CTAssocBool;
		}
	}

	return value;
}

map<wstring, unsigned int> Config::ReadAssoc(std::wstring key, map<wstring, unsigned int> &value) {

	for (map<wstring, ConfigEntry>::iterator it = contents.begin(); it != contents.end(); it++) {
		if ((*it).first.find(key + L"[") != wstring::npos) {
			pair<wstring, unsigned int> assoc;
			//Pull the value from between the []'s
			assoc.first = (*it).first.substr((*it).first.find(L"[") + 1, (*it).first.length() - (*it).first.find(L"[") - 2);
			//Simply store the value that was after the :
			if ((*it).second.value.find(L"0x") != wstring::npos)
				from_wstring<unsigned int>(assoc.second, (*it).second.value, std::hex);
			else
				from_wstring<unsigned int>(assoc.second, (*it).second.value, std::dec);

			if (value.find(assoc.first) == value.end()) {
				value.insert(assoc);
			}
			else {
				value[assoc.first] = assoc.second;
			}

			(*it).second.pointer = &value;
			(*it).second.type = CTAssocInt;
		}
	}

	return value;
}

vector<pair<wstring, wstring>> Config::ReadMapList(std::wstring key, vector<pair<wstring, wstring>>& values) {

	for (vector<pair<wstring, wstring>>::iterator it = orderedKeyVals.begin(); it != orderedKeyVals.end(); it++) {
		if (!(*it).first.find(key + L"[")) {
			pair<wstring, wstring> assoc;
			//Pull the value from between the []'s
			assoc.first = (*it).first.substr((*it).first.find(L"[") + 1, (*it).first.length() - (*it).first.find(L"[") - 2);
			//Also store the value
			assoc.second = (*it).second;
			values.push_back(assoc);
		}
	}

	return values;
}

list<wstring> Config::GetDefinedKeys() {
	std::list<wstring> ret;

	for (map<wstring, ConfigEntry>::iterator it = contents.begin(); it != contents.end(); it++) {
		wstring key = (*it).first;

		if (key.find(L"[") != wstring::npos)
			key = key.substr(0, key.find(L"["));

		ret.push_back(key);
	}
	ret.unique();
	return ret;
}

bool Config::HasChanged(ConfigEntry entry, string& value) {
	if (entry.type != CTToggle && entry.pointer == NULL)
		return false;

	switch (entry.type) {
	case CTBoolean: {
		bool currentBool = *((bool*)entry.pointer);
		bool storedBool = StringToBool(entry.value);

		if (storedBool == currentBool)
			return false;

		value = currentBool ? "True" : "False";
		return true;
	}
	case CTInt: {
		int currentInt = *((int*)entry.pointer);

		int storedInt = 0;
		std::stringstream stream;
		bool hex = false;
		if (entry.value.find(L"0x") != wstring::npos) {
			from_wstring<int>(storedInt, entry.value, std::hex);
			stream << std::hex;
			hex = true;
		}
		else {
			from_wstring<int>(storedInt, entry.value, std::dec);
		}

		if (currentInt == storedInt)
			return false;

		stream << currentInt;
		value = ((hex) ? "0x" : "") + stream.str();
		return true;
	}
	case CTString: {
		wstring currentString = *((wstring*)entry.pointer);

		if (currentString.compare(entry.value) == 0)
			return false;

		value = UnicodeToAnsi(currentString.c_str());
		return true;
	}
	case CTArray: {
		vector<wstring> valTest = *((vector<wstring>*)entry.pointer);
		wstring ind = entry.key.substr(entry.key.find(L"[") + 1, entry.key.length() - entry.key.find(L"[") - 2);
		int index = _wtoi(ind.c_str());

		if (index >= valTest.size()) {
			value = "";
			return true;
		}

		wstring currentString = valTest.at(index);

		if (currentString.compare(entry.value) == 0)
			return false;

		value = UnicodeToAnsi(currentString.c_str());
		return true;
	}

	case CTAssoc: {
		wstring assocKey = entry.key.substr(entry.key.find(L"[") + 1, entry.key.length() - entry.key.find(L"[") - 2);
		map<wstring, wstring> valTest = *((map<wstring, wstring>*)entry.pointer);

		wstring currentString = valTest[assocKey];

		if (currentString.compare(entry.value) == 0)
			return false;

		value = UnicodeToAnsi(currentString.c_str());
		return true;
	}
	case CTAssocBool: {
		wstring assocKey = entry.key.substr(entry.key.find(L"[") + 1, entry.key.length() - entry.key.find(L"[") - 2);
		transform(assocKey.begin(), assocKey.end(), assocKey.begin(), ::tolower);
		map<wstring, bool> valTest = *((map<wstring, bool>*)entry.pointer);

		bool currentBool = valTest[assocKey];

		if (currentBool == StringToBool(entry.value))
			return false;

		value = currentBool ? "True" : "False";
		return true;
	}
	case CTAssocInt: {
		wstring assocKey = entry.key.substr(entry.key.find(L"[") + 1, entry.key.length() - entry.key.find(L"[") - 2);
		map<wstring, unsigned int> valTest = *((map<wstring, unsigned int>*)entry.pointer);
		int currentInt = valTest[assocKey];

		int storedInt = 0;
		std::stringstream stream;
		bool hex = false;
		if (entry.value.find(L"0x") != wstring::npos) {
			from_wstring<int>(storedInt, entry.value, std::hex);
			stream << std::hex;
			hex = true;
		}
		else {
			from_wstring<int>(storedInt, entry.value, std::dec);
		}

		if (currentInt == storedInt)
			return false;

		stream << currentInt;
		value = ((hex) ? "0x" : "") + stream.str();
		return true;
	}
	case CTToggle: {
		unsigned int toggle = GetKeyCode(UnicodeToAnsi(Trim(entry.value.substr(entry.value.find_first_of(L",") + 1)).c_str())).value;
		bool state = StringToBool(Trim(entry.value.substr(0, entry.value.find_first_of(L","))));

		if (entry.toggle->toggle == toggle && entry.toggle->state == state)
			return false;

		stringstream stream;
		KeyCode newKey = GetKeyCode(entry.toggle->toggle);

		stream << ((entry.toggle->state) ? "True" : "False") << ", " << newKey.name;

		value = stream.str();
		return true;
	}
	case CTKey: {
		unsigned int currentKey = *((unsigned int*)entry.pointer);
		KeyCode code = GetKeyCode(UnicodeToAnsi(entry.value.c_str()));

		if (code.value == currentKey)
			return false;

		KeyCode newCode = GetKeyCode(currentKey);
		value = newCode.name;
		return true;
	}
	}
	return false;
}

bool Config::StringToBool(std::wstring input) {
	const wchar_t* boolStr = input.c_str();
	return (_wcsicmp(boolStr, L"1") == 0) || (_wcsicmp(boolStr, L"y") == 0) || (_wcsicmp(boolStr, L"yes") == 0) || (_wcsicmp(boolStr, L"true") == 0);
}
