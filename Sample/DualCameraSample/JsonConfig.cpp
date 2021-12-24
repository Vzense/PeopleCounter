#include<fstream>
#include <memory>
#include "JsonConfig.h"
#define ENABLE_TARGET_EXPORT
#define ENABLE_CUSTOM_COMPILER_FLAGS
#include "cJSON.h"

JsonConfig::JsonConfig():
    mPmonitor_json(nullptr)
{
}

JsonConfig::~JsonConfig()
{
    if (nullptr != mPmonitor_json)
    {
        cJSON_Delete(mPmonitor_json);
    }
}

int JsonConfig::Read(const string& configName)
{
    int ret = -1;

    FILE* fp = fopen(configName.c_str(), "r");
    fseek(fp, 0, SEEK_END);
    const int FSize = ftell(fp);
    rewind(fp);

    char* pBuf = new char[FSize + 1];
    memset(pBuf, 0, (FSize + 1));
    fread(pBuf, 1, FSize, fp);

    mPmonitor_json = cJSON_ParseWithLength(pBuf, FSize);

    if (mPmonitor_json != NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        else
        {
            ret = 1;
        }
    }
    delete[] pBuf;

    return ret;
}

int JsonConfig::GetInt(string root, string key, int defaultValue)
{
    int ret = defaultValue;

    const cJSON *object  = cJSON_GetObjectItemCaseSensitive(mPmonitor_json, root.c_str());
    if (object != nullptr
        && true == cJSON_IsObject(object))
    {
        cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key.c_str());

        if (true == cJSON_IsNumber(value))
        {
            ret = value->valueint;
        }
    }

    return ret;
}

float JsonConfig::GetFloat(string root, string key, float defaultValue)
{
    float ret = defaultValue;

    const cJSON *object = cJSON_GetObjectItemCaseSensitive(mPmonitor_json, root.c_str());
    if (object != nullptr
        && true == cJSON_IsObject(object))
    {
        cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key.c_str());

        if (true == cJSON_IsNumber(value))
        {
            ret = value->valuedouble;
        }
    }
    
    return ret;
}

bool JsonConfig::GetBool(string root, string key, bool defaultValue)
{
    bool ret = defaultValue;

    const cJSON *object = cJSON_GetObjectItemCaseSensitive(mPmonitor_json, root.c_str());
    if (object != nullptr
        && true == cJSON_IsObject(object))
    {
        cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key.c_str());

        if (true == cJSON_IsBool(value))
        {
            ret = (true == cJSON_IsTrue(value)) ? true : false;
        }
    }

    return ret;
}

string JsonConfig::GetValue(string root, string key, const string& defaultValue)
{
    string ret = defaultValue;

    const cJSON *object = cJSON_GetObjectItemCaseSensitive(mPmonitor_json, root.c_str());
    if (object != nullptr
        && true == cJSON_IsObject(object))
    {
        cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key.c_str());

        if (true == cJSON_IsString(value)
            && (value->valuestring != nullptr))
        {
            ret = value->valuestring;
        }
    }

    return ret;
}