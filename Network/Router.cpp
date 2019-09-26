#include "stdafx.h"
#include <thread>
#include <memory>
#include <list>
#include <algorithm>
#include <condition_variable>
#include "Network/Router.h"

namespace Http {

Memory::Memory() {
    Size(0);    /* no data at this point */
    MemoryAddr((char*)malloc(1));   /* will be grown as needed by the realloc above */
}

Memory::Memory(const Memory& memory)
    : size(memory.size), memoryAddr(nullptr) {
    Size(memory.Size());
    memoryAddr = (char*)malloc(memory.Size());
    memcpy(memoryAddr, memory.MemoryAddr(), memory.Size());
}

Memory::Memory(Memory&& memory)
    : memoryAddr(memory.memoryAddr), size(memory.size) {
    memory.MemoryAddr(nullptr);
}

bool Memory::operator==(const Memory& memory)const {
    return memoryAddr == memory.memoryAddr && size == memory.size;
}


char* Memory::MemoryAddr() const {
    return memoryAddr;
}


void Memory::MemoryAddr(char* val) {
    memoryAddr = val;
}


size_t Memory::Size() const {
    return size;
}


void Memory::Size(size_t val) {
    size = val;
}

Memory::~Memory() {
    free(MemoryAddr());
    MemoryAddr(0);
}

Request::Request(const URL& url, Base* userData) : url(url), userData(userData), unhandled(true), updDatas(), type(TYPE::GET) {
}

Request::Request(const URL& url, const std::vector<UploadedData>& uploadeddatas, Base* userData /*= nullptr*/)
    : url(url), updDatas(uploadeddatas), userData(userData), type(TYPE::POST) {

}

Request::Request(const Request& request)
    : url(request.Url()), userData(request.UserData()),
      unhandled(request.Unhandled()), updDatas(request.updDatas), type(request.type) {
}


Request::Request(Request&& request)
    : url(std::move(request.url)), userData(request.userData),
      unhandled(request.unhandled), updDatas(std::move(request.updDatas)), type(request.type) {
    request.UserData(nullptr);
}



bool Request::operator==(const Request& request) const {
    return url == request.url && unhandled == request.unhandled && userData == request.userData && updDatas == request.updDatas && type == request.Type();
}

Request::~Request() {
    delete userData;
    UserData(0);
}


const URL& Request::Url() const {
    return url;
}


void Request::Url(const URL& val) {
    url = val;
}

URL& Request::Url() {
    return url;
}

void Request::Unhandled(bool) {
    unhandled = false;
}


bool Request::Unhandled() const {
    return unhandled;
}


Base* Request::UserData() const {
    return userData;
}


void Request::UserData(Base* val) {
    userData = val;
}


std::vector<Http::UploadedData>& Request::Uploadeddatas() {
    return updDatas;
}


Http::Request::TYPE Request::Type() const {
    #ifdef _DEBUG
    assert(type == GET || type == POST);
    #endif
    return type;
}


void Request::Type(TYPE val) {
    type = val;
    #ifdef _DEBUG
    assert(type == GET || type == POST);
    #endif
}

Response::Response() : Memory(), curlCode(CURLE_OK), curl(nullptr), dltotal(0) {}

Response::Response(const Response& response) : Memory(response), curlCode(response.CurlCode()),
    curl(response.Curl()), dltotal(response.Dltotal()) {}


Response::Response(Response&& response): Memory(std::move(response)), curlCode(response.curlCode),
    curl(response.curl),  dltotal(response.dltotal) {

}

curl_off_t Response::Dltotal() const {
    return dltotal;
}

void Response::Dltotal(curl_off_t val) {
    dltotal = val;
}

CURLcode Response::CurlCode() const {
    return curlCode;
}


void Response::CurlCode(CURLcode val) {
    curlCode = val;
}


CURL* Response::Curl() const {
    return curl;
}


void Response::Curl(CURL* val) {
    curl = val;
}

bool Response::operator==(const Response&& response)const {
    if (Memory::operator==(static_cast < const Memory && > (response))) {
        return curlCode == response.curlCode && curl == response.curl;
    }
    return false;
}



Http::Router& Router::GetInstance() {
    static Router instance;
    return instance;
}

Task::Task(Task&& task) :
    Request(std::move(static_cast<Request&>(task))), Response(std::move(static_cast<Response&>(task))),
    action(task.Action()), mark(task.Mark()) {
    task.Action(nullptr);
}


Task::Task(const Task& task) : Request(task), Response(task),
    action(task.action), mark(task.mark) {

}


Task::Task(const URL& url, Http::Action* action, Base* userdata /*= nullptr*/) : Request(url, userdata), Response(), action(action), mark(Task::markCouter++) {

}

Task::Task(const URL& url, const std::vector<UploadedData>& uploadData, Http::Action* action, Base* userData /*= nullptr*/) : Request(url, uploadData, userData), action(action), mark(Task::markCouter++) {

}

bool Task::operator==(const Task& task) const {
    if (Request::operator==(static_cast < const Task && > (task)) &&
            Response::operator==(static_cast < const Task && > (task))) {
        return mark == task.mark && action == task.action;
    }
    return false;
}

Http::Action* Task::Action() const {
    return action;
}

void Task::Action(Http::Action* val) {
    action = val;
}

long long Task::Mark() const {
    return mark;
}

void Task::Mark(long long val) {
    mark = val;
}

long long Task::markCouter = 0;
volatile  bool g_createdExcutor = true;

/*taskQueue maintains a task queue to perform task orderly*/
class TaskQueue {
public:
    std::list<Task>::iterator FrontUnhandledTask() {
        for (auto beg = std::begin(taskQueue); beg != std::end(taskQueue); ++beg) {
            if (beg->Unhandled()) {
                return beg;
            }
        }
        return taskQueue.end();
    }
    bool HasUnhandledTask() {
        return !(FrontUnhandledTask() == taskQueue.end());
    }

    void Push(Task&& task) {
        taskQueue.push_back(std::move(task));
    }
    void Pop(long long mark) {
        taskQueue.remove_if([mark](const Task & task) {
            return mark == task.Mark();
        });
    }
private:
    std::list<Task> taskQueue;
};
TaskQueue g_taskQueue;
/*atomic variable determines TaskExcuter Thread Living Status*/

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    Task* task = (Task*)userp;
    // dltotal == 0则未获取总大小，>0则已获得为realloc，<0 则已realloc
    if (task->Dltotal() > 0) {
        task->MemoryAddr((char*)realloc(task->MemoryAddr(), task->Dltotal() + 1));
        task->Dltotal(-1);
    } else if (task->Dltotal() == 0) {
        task->MemoryAddr((char*)realloc(task->MemoryAddr(), task->Size() + realsize + 1));
    }
    memcpy(&(task->MemoryAddr()[task->Size()]), contents, realsize);
    task->Size(task->Size() + realsize);
    //task->MemoryAddr()[task->Size()] = 0;
    return realsize;
}

static int xferinfo(void* p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    double curtime = 0;
    Task* task = (Task*)p;
    curl_easy_getinfo(task->Curl(), CURLINFO_TOTAL_TIME, &curtime);
    if (task->Dltotal() == 0 && dltotal != 0) {
        task->Dltotal(dltotal);
    }
    if (curtime - task->Action()->LastTime() >= task->Action()->ProgressInterval()) {
        task->Action()->LastTime(curtime);
        return task->Action()->Progress(curtime, (double)dltotal, (double)dlnow, ultotal, ulnow, *task);

    }
    return 0;
}

static int older_progress(void* p, double dltotal, double dlnow, double ultotal, double ulnow) {
    return xferinfo(p, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow);
}


static void init(CURLM* cm) {
    CURL* eh = curl_easy_init();
    Task& unhandledTask = *g_taskQueue.FrontUnhandledTask();
    unhandledTask.Curl(eh);
    //check request type
    Request::TYPE type = unhandledTask.Type();
    if (unhandledTask.Type() == Request::TYPE::POST) {
        struct curl_httppost* formpost = NULL;
        struct curl_httppost* lastptr = NULL;

        struct curl_slist* headerlist = NULL;
        static const char buf[] = "Expect:";

        const std::vector<UploadedData>& uploadedDatas = unhandledTask.Uploadeddatas();
        for (auto data : uploadedDatas) {
            CURLformoption opt;
            if (data.Field() == UploadedData::FIELD::FILE) {
                opt = CURLformoption::CURLFORM_FILE;
            } else {
                opt = CURLformoption::CURLFORM_COPYCONTENTS;
            }
            if (data.FileName().empty()) {
                curl_formadd(&formpost, &lastptr,
                             CURLFORM_COPYNAME, data.Key().c_str(),
                             opt, data.Value().c_str(),
                             CURLFORM_END);
            } else {
                curl_formadd(&formpost, &lastptr,
                             CURLFORM_COPYNAME, data.Key().c_str(),
                             opt, data.Value().c_str(), CURLFORM_FILENAME, data.FileName().c_str(),
                             CURLFORM_END);
            }
        }
        headerlist = curl_slist_append(headerlist, buf);
        curl_easy_setopt(eh, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(eh, CURLOPT_HTTPPOST, formpost);
    }
    //set easy handle option
    curl_easy_setopt(eh, CURLOPT_PRIVATE, (void*)&unhandledTask);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void*)&unhandledTask);
    curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(eh, CURLOPT_NOPROGRESS, 0);
    #if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(eh, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt(eh, CURLOPT_XFERINFODATA, &unhandledTask);
    #else
    curl_easy_setopt(eh, CURLOPT_PROGRESSFUNCTION, older_progress);
    curl_easy_setopt(eh, CURLOPT_PROGRESSDATA, &unhandledTask);
    #endif

    curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
    unhandledTask.Url().Escape(eh);
    curl_easy_setopt(eh, CURLOPT_URL, unhandledTask.Url().ToString().c_str());
    unhandledTask.Unhandled(false);
    curl_multi_add_handle(cm, eh);
}

void Excutor() {

    CURLM* cm = nullptr;
    CURLMsg* msg;
    long L;
    int M, Q, U = -1;
    fd_set R, W, E;
    struct timeval T;
    CURLMcode code;
    cm = curl_multi_init();
    int MAX_SIZE = 9;
    /* we can optionally limit the total amount of connections this multi handle uses */
    curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, MAX_SIZE);

    while (true) {
        while (!g_taskQueue.HasUnhandledTask()) {
            Sleep(100);
        }


        M = Q = U = -1;

        int initNum = 0;
        while (g_taskQueue.HasUnhandledTask() && initNum++ < MAX_SIZE) {
            init(cm);
        }
        while (U) {
            //when running_handles is set to zero (0) on the return of this function, there is no longer any transfers in progress
            curl_multi_perform(cm, &U);
            //when U==0, all in finished
            if (U) {
                FD_ZERO(&R);
                FD_ZERO(&W);
                FD_ZERO(&E);
                if (code = curl_multi_fdset(cm, &R, &W, &E, &M)) {
                    fprintf(stderr, "%s/n", curl_multi_strerror(code));
                    return;
                }
                //An application using the libcurl multi interface should call curl_multi_timeout to figure out how long it should wait for socket actions - at most - before proceeding
                if (code = curl_multi_timeout(cm, &L)) {
                    fprintf(stderr, "%s/n", curl_multi_strerror(code));
                    return;
                }
                //optimum solution for next time timeout
                if (L == -1)
                    L = 100;
                if (M == -1) {
                    Sleep(L);
                } else {
                    T.tv_sec = L / 1000;
                    T.tv_usec = (L % 1000) * 1000;
                    //The select() system call examines the I/O descriptor sets whose addresses are passed	in readfds, writefds, and exceptfds to see if some of their
                    //	descriptors are ready for reading, are ready for writing, or have an exceptional condition pending, respectively
                    if (0 > select(M + 1, &R, &W, &E, &T)) {
                        fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
                                M + 1, L, errno, strerror(errno));
                        return;
                    }
                }
            }

            while ((msg = curl_multi_info_read(cm, &Q))) {
                Task* task;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &task);
                task->CurlCode(msg->data.result);

                CURL* e = msg->easy_handle;
                curl_multi_remove_handle(cm, e);
                curl_easy_cleanup(e);

                /*Execute action indicate by user*/
                task->Action()->Do(*task);
                g_taskQueue.Pop(task->Mark());
            }
            if (g_taskQueue.HasUnhandledTask()) {
                init(cm);
                U++; /* just to prevent it from remaining at 0 if there are more URLs to get */
            }
        }
    }
}



void Router::Get(const URL& url, Action* httpAction, Base* userData /*= nullptr*/) {
    Run(Task(url, httpAction, userData));
}

void Router::Post(const URL& url, const std::vector<UploadedData>& uploadedDatas, Action* httpAction, Base* userData /*= nullptr*/) {
    Run(Task(url, uploadedDatas, httpAction, userData));
}

void Router::Run(Task&& task) {
    g_taskQueue.Push(std::move(task));
    if (g_createdExcutor == true) {
        g_createdExcutor = false;
        std::thread excutor(Excutor);
        excutor.detach();
    }
}

Router::~Router() {
}

UploadedData::UploadedData(FIELD dataType, const std::string& key, const std::string& value, const std::string& filename) : field(dataType), key(key), value(value), fileName(filename) {

}


UploadedData::UploadedData(const UploadedData& uploadedData) : field(uploadedData.field), key(uploadedData.key), value(uploadedData.value), fileName(uploadedData.fileName) {

}

UploadedData::UploadedData(UploadedData&& uploadedData) : field(uploadedData.field), key(std::move(uploadedData.key)), value(std::move(uploadedData.value)), fileName(uploadedData.fileName) {

}


UploadedData& UploadedData::operator=(const UploadedData& uploadedData) {
    field = uploadedData.Field();
    key = uploadedData.Key(), value = uploadedData.Value(), fileName = uploadedData.fileName;
    return *this;
}

bool UploadedData::operator==(const UploadedData& uploadedData) const {
    return field == uploadedData.field && key == uploadedData.key && value == uploadedData.value && fileName == uploadedData.fileName;
}

UploadedData::FIELD UploadedData::Field() const {
    return field;
}

void UploadedData::Field(FIELD val) {
    field = val;
}


const std::string& UploadedData::Key() const {
    return key;
}


void UploadedData::Key(const std::string& val) {
    key = val;
}


const std::string& UploadedData::Value() const {
    return value;
}


void UploadedData::Value(const std::string& val) {
    value = val;
}

}