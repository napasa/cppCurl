#pragma once
#include <string>
#include <map>
#include <vector>
#include "curl/curl.h"

#ifdef NETWORK_EXPORTS
    #define NETWORK_API __declspec(dllexport)
#else
    #define NETWORK_API __declspec(dllimport)
#endif

namespace Http {
class URL {
public:
    typedef std::string AttribKey;
    typedef std::string AttribValue;
    typedef std::map<AttribKey, AttribValue> _AttribMap;
    class  AttribMap : public _AttribMap {
    public:
        NETWORK_API AttribMap(std::initializer_list<value_type> init);
        NETWORK_API AttribValue& operator[](const AttribKey& key);
        NETWORK_API AttribMap() {}
        NETWORK_API const std::string ToString()const;
    };
public:
    NETWORK_API URL(const std::string& host, const std::string& path, const AttribMap& queryString);
    // default url is urlencode
    NETWORK_API URL();
    // default url is urlencode
    NETWORK_API URL(const std::string& url);
    NETWORK_API URL(const URL& url);
    NETWORK_API URL(const URL&& url);
    NETWORK_API URL& operator=(const URL& url);
    NETWORK_API bool operator==(const URL& url)const;
    NETWORK_API const std::string& ToString()const;
    NETWORK_API const AttribMap& GetAttribMap()const;
    NETWORK_API void Escape(CURL* eh);
    NETWORK_API virtual ~URL();
private:
    std::string scheme;
    std::string host;
    std::string path;
    AttribMap queryString;
    std::string stringizedUrl;
};

}
