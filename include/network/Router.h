#pragma once
#include <vector>
#include <string>
#include "curl/curl.h"
#include "URL.h"
#ifdef _DEBUG
    #include <cassert>
#endif

#ifdef NETWORK_EXPORTS
    #define NETWORK_API __declspec(dllexport)
#else
    #define NETWORK_API __declspec(dllimport)
#endif

#define ROUTER Http::Router::GetInstance()
namespace Http {
typedef std::basic_string<char, std::char_traits<char>, std::allocator<char> > string;
/*Memory used for storing response Content*/
class NETWORK_API Base {
public:
    virtual ~Base() {}
};

class NETWORK_API Memory : public Base {
public:
    Memory();
    Memory(const Memory& memory);
    Memory(Memory&& memory);
    Memory& operator=(const Memory& memory) = delete;
    bool operator==(const Memory&)const;
    //Setter and getter
public:
    char* MemoryAddr() const;
    void MemoryAddr(char* val);
    size_t Size() const;
    void Size(size_t val);
    virtual ~Memory();
private:
    char* memoryAddr;
    size_t size;
};


//name,value,type
class  UploadedData : public Base {
public:
    enum FIELD {
        STRING,
        FILE
    };
    NETWORK_API UploadedData() {}
    NETWORK_API UploadedData(FIELD dataType, const std::string& key, const std::string& value, const std::string& filename = std::string());
    NETWORK_API UploadedData(const UploadedData& uploadedData);
    NETWORK_API UploadedData(UploadedData&& uploadedData);
    NETWORK_API UploadedData& operator=(const UploadedData& uploadData);
    NETWORK_API bool operator==(const UploadedData& postData)const;
    NETWORK_API FIELD Field() const;
    NETWORK_API	void Field(FIELD val);
    NETWORK_API const std::string& Key()const;
    NETWORK_API void Key(const std::string& val);
    NETWORK_API const std::string& Value()const;
    NETWORK_API void Value(const std::string& val);
    NETWORK_API std::string FileName() const { return fileName; }
    NETWORK_API void FileNames(std::string val) { fileName = val; }
private:
    FIELD field;
    std::string key;
    std::string value;
    std::string fileName;
};

/*HTTP request*/
//class TaskQueue;
class  Request : public Base {
public:
    enum TYPE : int {
        GET = 0,
        POST = 1
    };
public:
    NETWORK_API Request() = delete;
    NETWORK_API Request(const URL& url, Base* userData = nullptr);
    NETWORK_API Request(const URL& url, const std::vector<UploadedData>& uploadeddatas, Base* userData = nullptr);
    NETWORK_API Request(const Request& request);
    NETWORK_API Request(Request&& request);
    NETWORK_API Request& operator=(const Request&) = delete;
    NETWORK_API bool operator==(const Request& request)const;
    NETWORK_API ~Request();
    //Setter and getter
public:
    NETWORK_API const URL& Url() const;
    NETWORK_API URL& Url();
    NETWORK_API void Url(const URL& val);
    NETWORK_API void Unhandled(bool);
    NETWORK_API bool Unhandled() const;
    NETWORK_API Base* UserData() const;
    NETWORK_API void UserData(Base* val);
    NETWORK_API std::vector<UploadedData>& Uploadeddatas();
    NETWORK_API Http::Request::TYPE Type() const;
    NETWORK_API void Type(Http::Request::TYPE val);
protected:
    URL url;
    bool unhandled;
    Base* userData;
    std::vector<UploadedData> updDatas;
    TYPE type;
};



/*HTTP response*/
class NETWORK_API Response : public Memory {
public:
    Response();
    Response(const Response& response);
    Response(Response&& response);
    Response& operator=(const Response&) = delete;
    bool operator==(const Response&& response)const;
    ~Response() {}
    curl_off_t Dltotal() const;
    void Dltotal(curl_off_t val);
    //Setter and getter
public:
    CURLcode CurlCode() const;
    void CurlCode(CURLcode val);
    CURL* Curl() const;
    void Curl(CURL* val);
private:
    CURLcode curlCode;
    CURL* curl;
    bool receivedDlTotal;
    curl_off_t dltotal;
};

class NETWORK_API Action;
/*HTTP task. Queue model*/
class Task : public Request, public Response {
public:
    Task() = delete;
    Task(const Task& task);
    Task(Task&& task);
    Task(const URL& url, Action* action, Base* userdata = nullptr);
    Task(const URL& url, const std::vector<UploadedData>& uploadData, Action* action, Base* userData = nullptr);
    bool operator==(const Task& task)const;
    ~Task() {}
    //Setter and getter
public:
    Http::Action* Action() const;
    void Action(Http::Action* val);
    long long Mark() const;
    void Mark(long long val);
private:
    Http::Action* action;
    long long mark;
    static long long markCouter;
};

/*HTTP action for response from server, overload do func to perform action to response*/
class NETWORK_API Action : public Base {
public:
    Action(): progressInterval(0.1), lastTime(0) {}
    virtual void Do(const Http::Task& task) = 0;
    virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow, const Http::Task& task) = 0;
    ~Action() {}
    double ProgressInterval() const { return progressInterval; }
    void ProgressInterval(double val) { progressInterval = val; }
    float LastTime() const { return lastTime; }
    void LastTime(float val) { lastTime = val; }
private:
    double progressInterval;
    float lastTime;
};

class  Router : public Base {
public:
    NETWORK_API static  Router& GetInstance();
    NETWORK_API void Get(const URL& url, Action* httpAction, Base* userData = nullptr);
    NETWORK_API void Post(const URL& url, const std::vector<UploadedData>& uploadedDatas, Action* httpAction, Base* userData = nullptr);
    NETWORK_API void Run(Task&& task);
    NETWORK_API ~Router();
    NETWORK_API Router(const Router& http) = delete;
    NETWORK_API Router& operator=(const Router&) = delete;
private:
    Router() {}
};

}
