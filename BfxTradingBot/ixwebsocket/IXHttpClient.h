/*
 *  IXHttpClient.h
 *  Author: Benjamin Sergeant
 *  Copyright (c) 2019 Machine Zone, Inc. All rights reserved.
 */

#pragma once

#include "IXSocket.h"
#include "IXWebSocketHttpHeaders.h"
#include "IXHttp.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace ix
{
    class HttpClient
    {
    public:
        HttpClient(bool async = false);
        ~HttpClient();

        HttpResponsePtr get(const std::string& url, HttpRequestArgsPtr args);
        HttpResponsePtr head(const std::string& url, HttpRequestArgsPtr args);
        HttpResponsePtr del(const std::string& url, HttpRequestArgsPtr args);

        HttpResponsePtr post(const std::string& url,
                             const HttpParameters& httpParameters,
                             HttpRequestArgsPtr args);
        HttpResponsePtr post(const std::string& url,
                             const std::string& body,
                             HttpRequestArgsPtr args);

        HttpResponsePtr put(const std::string& url,
                            const HttpParameters& httpParameters,
                            HttpRequestArgsPtr args);
        HttpResponsePtr put(const std::string& url,
                            const std::string& body,
                            HttpRequestArgsPtr args);

        HttpResponsePtr request(const std::string& url,
                                const std::string& verb,
                                const std::string& body,
                                HttpRequestArgsPtr args,
                                int redirects = 0);

        // Async API
        HttpRequestArgsPtr createRequest(const std::string& url = std::string(),
                                         const std::string& verb = HttpClient::kGet);

        bool performRequest(HttpRequestArgsPtr request,
                            const OnResponseCallback& onResponseCallback);

        std::string serializeHttpParameters(const HttpParameters& httpParameters);

        std::string urlEncode(const std::string& value);

        const static std::string kPost;
        const static std::string kGet;
        const static std::string kHead;
        const static std::string kDel;
        const static std::string kPut;

    private:
        void log(const std::string& msg, HttpRequestArgsPtr args);

        bool gzipInflate(const std::string& in, std::string& out);

        // Async API background thread runner
        void run();

        // Async API
        bool _async;
        std::queue<std::pair<HttpRequestArgsPtr, OnResponseCallback>> _queue;
        mutable std::mutex _queueMutex;
        std::condition_variable _condition;
        std::atomic<bool> _stop;
        std::thread _thread;

        std::shared_ptr<Socket> _socket;
        std::mutex _mutex; // to protect accessing the _socket (only one socket per client)
    };
} // namespace ix
