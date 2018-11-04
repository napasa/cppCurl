#include "stdafx.h"
#include <sstream>
#include <random>
#include <array>
#include "Url.h"

namespace Http {
Url::Url(const std::string& host, const std::string& path, const AttribMap& queryString) : scheme("http"), host(host), path(path), queryString(queryString) {
    FormatUrl();
}


Url::Url(const char* url)
    : url(url) {
}


Url::Url(const std::string& url)
    : url(url) {

}

const std::string& Url::String() const {
    return url;
}


NETWORK_API const Url::AttribMap& Url::GetAttribMap() const {
    return queryString;
}

void Url::FormatUrl() {
    url.reserve((host.length() + path.length()) * 5);
    url = scheme + "://" + host + path + "?";

    for (auto const& attribPair : queryString) {
        url += (attribPair.first + "=" + attribPair.second);
        url.append("&");
    }
    url.pop_back();
}

Url::~Url() {
}


Url::AttribMap::AttribMap(std::initializer_list<value_type> init) : _AttribMap(init) {
}


NETWORK_API Url::AttribValue& Url::AttribMap::operator[](const AttribKey& key) {
    return _AttribMap::operator[](key);
}

NETWORK_API const std::string Url::AttribMap::ToString() const {
    std::string _attribs;
    size_type increment = 0;
    for (auto const& attribPair : *this) {
        _attribs += (attribPair.first + "=" + attribPair.second);
#define LAST_ELEMENT_INDEX (size()-1)
        if (increment++ < LAST_ELEMENT_INDEX) _attribs.append("&");
    }
    return _attribs;
}

}