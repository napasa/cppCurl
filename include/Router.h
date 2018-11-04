#pragma once
#include <vector>
#include <string>
#include <curl/curl.h>
#ifdef _DEBUG
#include <cassert>
#endif

#ifdef NETWORK_EXPORTS  
#define NETWORK_API __declspec(dllexport)   
#else  
#define NETWORK_API __declspec(dllimport)   
#endif  

#define ROUTER Http::Router::GetInstance()
namespace Http
{
	typedef std::basic_string<char, std::char_traits<char>, std::allocator<char> > string;
	/*Memory used for storing response Content*/
	class NETWORK_API Memory
	{
	public:
		Memory();
		Memory(const Memory &memory);
		Memory(Memory &&memory);
		Memory & operator=(const Memory &memory) = delete;
		bool operator==(const Memory&)const;
		//Setter and getter
	public:
		char * MemoryAddr() const { return memoryAddr; }
		void MemoryAddr(char * val) { memoryAddr = val; }
		size_t Size() const { return size; }
		void Size(size_t val) { size = val; }
		virtual ~Memory();
	private:
		char *memoryAddr;
		size_t size;
	};


	//name,value,type
	class  UploadedData
	{
	public:
		enum FIELD{
			STRING,
			FILE
		};
		NETWORK_API UploadedData(){}
		NETWORK_API UploadedData(FIELD dataType, const std::string &key, const std::string &value) :field(dataType), key(key), value(value){}
		NETWORK_API UploadedData(const UploadedData &postedData) :field(postedData.field), key(postedData.key), value(postedData.value){}
		NETWORK_API UploadedData(UploadedData &&postedData) :field(postedData.field), key(std::move(postedData.key)), value(std::move(postedData.value)){}
		NETWORK_API UploadedData &operator=(const UploadedData &uploadData){
			field = uploadData.Field(); key = uploadData.Key(), value = uploadData.Value();
			return *this;
		}
		NETWORK_API bool operator==(const UploadedData &postData)const{
			return field == postData.field && key == postData.key&&value == postData.value;
		}
		NETWORK_API FIELD Field() const { return field; }
		NETWORK_API	void Field(FIELD val) { field = val; }
		NETWORK_API const std::string &Key()const{ return key; }
		NETWORK_API void Key(const std::string &val) { key = val; }
		NETWORK_API const std::string &Value()const { return value; }
		NETWORK_API void Value(const std::string &val) { value = val; }
	private:
		FIELD field;
		std::string key;
		std::string value;
	};

	/*HTTP request*/
	//class TaskQueue;
	class  Request
	{
	public:
		enum TYPE :int{
			GET=0,
			POST=1
		};
	public:
		NETWORK_API Request() = delete;
		NETWORK_API Request(const std::string &url, void * userData = nullptr);
		NETWORK_API Request(const std::string &url, const std::vector<UploadedData> &uploadeddatas, void *userData = nullptr);
		NETWORK_API Request(const Request &request);
		NETWORK_API Request(Request &&request);
		NETWORK_API Request & operator=(const Request &) = delete;
		NETWORK_API bool operator==(const Request&request)const;
		NETWORK_API ~Request();
		//Setter and getter
	public:
		NETWORK_API const std::string &Url() const { return url; }
		NETWORK_API void Url(const std::string &val) { url = val; }
		NETWORK_API void Unhandled(bool ) { unhandled = false; }
		NETWORK_API  bool Unhandled() const { return unhandled; }
		NETWORK_API void * UserData() const { return userData; }
		NETWORK_API void UserData(void * val) { userData = val; }
		NETWORK_API std::vector<UploadedData> &Uploadeddatas(){ return uploadeddatas; }
		NETWORK_API Http::Request::TYPE Type() const { assert(type == GET || type == POST); return type; }
		NETWORK_API void Type(Http::Request::TYPE val) { type = val; assert(type == GET || type == POST); }
	protected:
		std::string url;
		bool unhandled;
		void *userData;
		std::vector<UploadedData> uploadeddatas;
		TYPE type;
	};



	/*HTTP response*/
	class NETWORK_API Response : public Memory {
	public:
		Response();
		Response(const Response &response);
		Response(Response &&response);
		Response & operator=(const Response &) = delete;
		bool operator==(const Response &&response)const;
		~Response() {}
		//Setter and getter
	public:
		CURLcode CurlCode() const { return curlCode; }
		void CurlCode(CURLcode val) { curlCode = val; }
		CURL * Curl() const { return curl; }
		void Curl(CURL * val) { curl = val; }
	private:
		CURLcode curlCode;
		CURL *curl;
	};

	class NETWORK_API Action;
	/*HTTP task. Queue model*/
	class Task : public Request, public Response {
	public:
		Task() = delete;
		Task(const Task &task) : Request(task), Response(task),
			action(task.action), mark(task.mark) {}
		Task(Task &&task);
		Task(const std::string &url, Action *action, void * userdata = nullptr)
			:Request(url, userdata), Response(), action(action), mark(Task::markCouter++) {}
		Task(const std::string &url, const std::vector<UploadedData> &uploadData, Action *action, void *userData = nullptr)
			:Request(url, uploadData, userData),action(action), mark(Task::markCouter++){}
		bool operator==(const Task &task)const;
		~Task() {  }
		//Setter and getter
	public:
		Http::Action * Action() const { return action; }
		void Action(Http::Action * val) { action = val; }
		long long Mark() const { return mark; }
		void Mark(long long val) { mark = val; }
	private:
		Http::Action *action;
		long long mark;
		static long long markCouter;
	};

	/*HTTP action for response from server, overload do func to perform action to response*/
	class NETWORK_API Action {
	public:
		Action() {}
		virtual void Do(const Http::Task&task) = 0;
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow, const Http::Task&task)=0;
		~Action() {}
	};

	class  Router
	{
	public:
		NETWORK_API static  Router & GetInstance();
		NETWORK_API void Get(const std::string &url, Action *httpAction, void * userData = nullptr);
		NETWORK_API void Post(const std::string &url, const std::vector<UploadedData> &uploadedDatas, Action *httpAction, void *userData = nullptr);
		NETWORK_API void Run(Task &&task);
		NETWORK_API ~Router();
		NETWORK_API Router(const Router &http) = delete;
		NETWORK_API Router& operator=(const Router&) = delete;
		NETWORK_API std::vector<Action *> ActionList() const { return actionList; }
		NETWORK_API void ActionList(std::vector<Action *> val) { actionList = val; }
	private:
		std::vector<Action *> actionList;
		Router() {}
	};

}
