#ifndef JSONCONFIG_H
#define JSONCONFIG_H
#include<string>

using std::string;

class cJSON;

class JsonConfig
{
public:
    JsonConfig();
    virtual ~JsonConfig();
    int Read(const string& configName);
    int GetInt(string root, string key, int defaultValue);
    float GetFloat(string root, string key, float defaultValue);
    bool GetBool(string root, string key, bool defaultValue);
    string GetValue(string root, string key, const string& defaultValue="");

private:
    cJSON *mPmonitor_json;
};

#endif // !JSONCONFIG_H
