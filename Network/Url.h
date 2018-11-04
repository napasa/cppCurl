#pragma once
#include <string>
#include <map>
#include <vector>

#ifdef NETWORK_EXPORTS  
#define NETWORK_API __declspec(dllexport)   
#else  
#define NETWORK_API __declspec(dllimport)   
#endif  

namespace Http
{
	class Url
	{
	public:
		typedef std::string AttribKey;
		typedef std::string AttribValue;
		typedef std::map<AttribKey, AttribValue> _AttribMap;
		class  AttribMap :public _AttribMap
		{
		public:
			NETWORK_API AttribMap(std::initializer_list<value_type> init);
			NETWORK_API AttribValue& operator[](const AttribKey& key);
			NETWORK_API AttribMap() {}
			NETWORK_API const std::string ToString()const;
		};
	public:
		NETWORK_API Url(const std::string &host, const std::string &path, const AttribMap &queryString);
		NETWORK_API Url() {};
		NETWORK_API Url(const char *url);
		NETWORK_API Url(const std::string &url);
		NETWORK_API const std::string & String()const;
		NETWORK_API const AttribMap &GetAttribMap()const;
		NETWORK_API virtual ~Url();
	private:
		void FormatUrl();
	private:
		std::string scheme;
		std::string host;
		std::string path;
		AttribMap queryString;
		std::string url;
	};

}
