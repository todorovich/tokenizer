#pragma once

#include <curl/curl.h>
#include <string>
#include <unordered_map>
#include "CircularPool.hpp"

class CurlGlobal {
public:
    CurlGlobal()  { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlGlobal() { curl_global_cleanup(); }

    CurlGlobal(const CurlGlobal&) = delete;
    CurlGlobal& operator=(const CurlGlobal&) = delete;
    CurlGlobal(CurlGlobal&&) = delete;
    CurlGlobal& operator=(CurlGlobal&&) = delete;
};

class CurlRequest {
public:
    CurlRequest() {
        _easy = curl_easy_init();
        if (_easy) {
            curl_easy_setopt(_easy, CURLOPT_WRITEFUNCTION, &CurlRequest::write_cb);
            curl_easy_setopt(_easy, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(_easy, CURLOPT_FORBID_REUSE, 0L);
            curl_easy_setopt(_easy, CURLOPT_FRESH_CONNECT, 0L);
            curl_easy_setopt(_easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
            curl_easy_setopt(_easy, CURLOPT_TCP_KEEPALIVE, 1L);       // turn on keep-alive probes
            curl_easy_setopt(_easy, CURLOPT_TCP_KEEPIDLE, 30L);        // idle time before probe
            curl_easy_setopt(_easy, CURLOPT_TCP_KEEPINTVL, 15L);
        }
    }

    ~CurlRequest() {
        if (_easy) curl_easy_cleanup(_easy);
    }

    CurlRequest(const CurlRequest&) = delete;
    CurlRequest& operator=(const CurlRequest&) = delete;
    CurlRequest(CurlRequest&&) = default;
    CurlRequest& operator=(CurlRequest&&) = default;

    void reset() {
        if (_easy) {
            curl_easy_reset(_easy);
            curl_easy_setopt(_easy, CURLOPT_WRITEFUNCTION, &CurlRequest::write_cb);
            curl_easy_setopt(_easy, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(_easy, CURLOPT_FORBID_REUSE, 0L);
            curl_easy_setopt(_easy, CURLOPT_FRESH_CONNECT, 0L);
            curl_easy_setopt(_easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
            curl_easy_setopt(_easy, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(_easy, CURLOPT_TCP_KEEPIDLE, 30L);
            curl_easy_setopt(_easy, CURLOPT_TCP_KEEPINTVL, 15L);
            _body.clear();
            _response.clear();
        }
    }

    void set_url(std::string url) {
        _url = std::move(url);
        curl_easy_setopt(_easy, CURLOPT_URL, _url.c_str());
    }

    void set_post_body(std::string data) {
        _body = std::move(data);
        curl_easy_setopt(_easy, CURLOPT_POST, 1L);
        curl_easy_setopt(_easy, CURLOPT_POSTFIELDS, _body.c_str());
        curl_easy_setopt(_easy, CURLOPT_POSTFIELDSIZE, _body.size());
    }

    [[nodiscard]] const std::string& get_post_body() const { return _body; }
    [[nodiscard]] const std::string& response() const { return _response; }
    [[nodiscard]] CURL* handle() const { return _easy; }

private:
    CURL* _easy = nullptr;
    std::string _url;
    std::string _body;
    std::string _response;

    static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* self = static_cast<CurlRequest*>(userdata);
        self->_response.append(ptr, size * nmemb);
        return size * nmemb;
    }
};

class CurlMulti {
public:
    using ResponseCallback = void(*)(long, const std::string&, const std::string&);

    explicit CurlMulti(const size_t max_concurrent = 100)
        : _requests(max_concurrent) {
        _multi = curl_multi_init();
        curl_multi_setopt(_multi, CURLMOPT_PIPELINING, CURLPIPE_HTTP1 | CURLPIPE_MULTIPLEX);
        curl_multi_setopt(_multi, CURLMOPT_MAX_HOST_CONNECTIONS, 0L);
        curl_multi_setopt(_multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 0L);
    }

    ~CurlMulti() {
        if (_multi) curl_multi_cleanup(_multi);
    }

    CurlMulti(const CurlMulti&) = delete;
    CurlMulti& operator=(const CurlMulti&) = delete;
    CurlMulti(CurlMulti&&) = delete;
    CurlMulti& operator=(CurlMulti&&) = delete;

    CurlRequest* try_next_request()
    {
        if (const auto pool_handle = _requests.try_acquire())
        {
            CurlRequest& req = _requests.get(*pool_handle);
            req.reset();
            _active[req.handle()] = *pool_handle;
            return &req;
        }

        return nullptr;
    }

    void enqueue(const CurlRequest& req, ResponseCallback callback)
    {
        _callbacks[req.handle()] = { callback, req.get_post_body() };
        curl_multi_add_handle(_multi, req.handle());
    }

    void run() {
        int running = 0;
        curl_multi_perform(_multi, &running);

        while (running)
        {
            curl_multi_poll(_multi, nullptr, 0, 1000, nullptr);
            curl_multi_perform(_multi, &running);

            int msgs = 0;
            while (const CURLMsg* msg = curl_multi_info_read(_multi, &msgs))
            {
                if (msg->msg == CURLMSG_DONE)
                {
                    CURL* easy = msg->easy_handle;

                    if (auto it = _active.find(easy); it != _active.end())
                    {
                        CurlRequest& req = _requests.get(it->second);

                        if (auto cb = _callbacks.find(easy); cb != _callbacks.end())
                        {
                            long status = 0;
                            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
                            cb->second.first(status, cb->second.second, req.response());
                            _callbacks.erase(cb);
                        }

                        curl_multi_remove_handle(_multi, easy);
                        _requests.release(it->second);
                        _active.erase(it);
                    }
                }
            }
        }
    }

private:
    CURLM* _multi = nullptr;
    CircularPool<CurlRequest> _requests;
    std::unordered_map<CURL*, PoolHandle> _active;
    std::unordered_map<CURL*, std::pair<ResponseCallback, std::string>> _callbacks;
};
