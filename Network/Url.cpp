#include "stdafx.h"
#include <sstream>
#include <random>
#include <array>
#include "Network/URL.h"

namespace Http {
URL::URL(const std::string& host, const std::string& path, const AttribMap& queryString)
    : stringizedUrl(), scheme("http"), host(host), path(path), queryString(queryString) {
}

URL::URL(const std::string& url)
    : stringizedUrl(url), scheme(), host(), path(), queryString() {

}

URL::URL()
    : stringizedUrl(), scheme(), host(), path(), queryString() {

}
URL::URL(const URL& url)
    : stringizedUrl(url.stringizedUrl), scheme(url.scheme), host(url.host), path(url.path), queryString(url.queryString) {

}


URL::URL(const URL&& url)
    : stringizedUrl(std::move(url.stringizedUrl)), scheme(std::move(url.scheme)), host(std::move(url.host)), path(std::move(url.path)), queryString(std::move(url.queryString)) {

}

URL& URL::operator=(const URL& url) {
    scheme = url.scheme;
    host = url.host;
    path = url.path;
    queryString = url.queryString;
    stringizedUrl = url.stringizedUrl;
    return *this;
}

bool URL::operator==(const URL& url) const {
    return scheme == url.scheme && host == url.host
           && path == url.path && queryString == url.queryString && stringizedUrl == url.stringizedUrl;
}

const std::string& URL::ToString() const {
    return stringizedUrl;
}


const URL::AttribMap& URL::GetAttribMap() const {
    return queryString;
}

void URL::Escape(CURL* eh) {
    if (stringizedUrl.empty()) {
        stringizedUrl.reserve((host.length() + path.length()) * 5);
        stringizedUrl = scheme + "://" + host + path + "?";

        for (auto const& attribPair : queryString) {
            char* value = curl_easy_escape(eh, attribPair.second.c_str(), attribPair.second.size());
            stringizedUrl += (attribPair.first + "=" + value);
            curl_free(value);
            stringizedUrl.append("&");
        }
        // 消除最后一个&
        stringizedUrl.pop_back();
    }
}

URL::~URL() {
}


URL::AttribMap::AttribMap(std::initializer_list<value_type> init) : _AttribMap(init) {
}


URL::AttribValue& URL::AttribMap::operator[](const AttribKey& key) {
    return _AttribMap::operator[](key);
}

const std::string URL::AttribMap::ToString() const {
    std::string attribs;
    size_type increment = 0;
    for (auto const& attribPair : *this) {
        attribs += (attribPair.first + "=" + attribPair.second);
#define LAST_ELEMENT_INDEX (size()-1)
        if (increment++ < LAST_ELEMENT_INDEX) attribs.append("&");
    }
    return attribs;
}

}