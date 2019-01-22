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
class NETWORK_API Memory {
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
class  UploadedData {
public:
    enum FIELD {
        STRING,
        FILE
    };
    NETWORK_API UploadedData() {}
    NETWORK_API UploadedData(FIELD dataType, const std::string& key, const std::string& value);
    NETWORK_API UploadedData(const UploadedData& postedData);
    NETWORK_API UploadedData(UploadedData&& postedData);
    NETWORK_API UploadedData& operator=(const UploadedData& uploadData);
    NETWORK_API bool operator==(const UploadedData& postData)const;
    NETWORK_API FIELD Field() const;
    NETWORK_API	void Field(FIELD val);
    NETWORK_API const std::string& Key()const;
    NETWORK_API void Key(const std::string& val);
    NETWORK_API const std::string& Value()const;
    NETWORK_API void Value(const std::string& val);
private:
    FIELD field;
    std::string key;
    std::string value;
};

/*HTTP request*/
//class TaskQueue;
class  Request {
public:
    enum TYPE : int {
        GET = 0,
        POST = 1
    };
public:
    NETWORK_API Request() = delete;
    NETWORK_API Request(const URL& url, void* userData = nullptr);
    NETWORK_API Request(const URL& url, const std::vector<UploadedData>& uploadeddatas, void* userData = nullptr);
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
    NETWORK_API void* UserData() const;
    NETWORK_API void UserData(void* val);
    NETWORK_API std::vector<UploadedData>& Uploadeddatas();
    NETWORK_API Http::Request::TYPE Type() const;
    NETWORK_API void Type(Http::Request::TYPE val);
protected:
    URL url;
    bool unhandled;
    void* userData;
    std::vector<UploadedData> uploadeddatas;
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
    Task(const URL& url, Action* action, void* userdata = nullptr);
    Task(const URL& url, const std::vector<UploadedData>& uploadData, Action* action, void* userData = nullptr);
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
class NETWORK_API Action {
public:
    Action() {}
    virtual void Do(const Http::Task& task) = 0;
    virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow, const Http::Task& task) = 0;
    ~Action() {}
};

class  Router {
public:
    NETWORK_API static  Router& GetInstance();
    NETWORK_API void Get(const URL& url, Action* httpAction, void* userData = nullptr);
    NETWORK_API void Post(const URL& url, const std::vector<UploadedData>& uploadedDatas, Action* httpAction, void* userData = nullptr);
    NETWORK_API void Run(Task&& task);
    NETWORK_API ~Router();
    NETWORK_API Router(const Router& http) = delete;
    NETWORK_API Router& operator=(const Router&) = delete;
    NETWORK_API std::vector<Action*> ActionList() const;
    NETWORK_API void ActionList(std::vector<Action*> val);
private:
    std::vector<Action*> actionList;
    Router() {}
};

}
