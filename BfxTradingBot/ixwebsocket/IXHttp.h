/*
 *  IXHttp.h
 *  Author: Benjamin Sergeant
 *  Copyright (c) 2019 Machine Zone, Inc. All rights reserved.
 */

#pragma once

#include "IXWebSocketHttpHeaders.h"
#include "IXProgressCallback.h"
#include <tuple>

namespace ix
{
    enum class HttpErrorCode : int
    {
        Ok = 0,
        CannotConnect = 1,
        Timeout = 2,
        Gzip = 3,
        UrlMalformed = 4,
        CannotCreateSocket = 5,
        SendError = 6,
        ReadError = 7,
        CannotReadStatusLine = 8,
        MissingStatus = 9,
        HeaderParsingError = 10,
        MissingLocation = 11,
        TooManyRedirects = 12,
        ChunkReadError = 13,
        CannotReadBody = 14,
        Invalid = 100
    };

    struct HttpResponse
    {
        int statusCode;
        std::string description;
        HttpErrorCode errorCode;
        WebSocketHttpHeaders headers;
        std::string payload;
        std::string errorMsg;
        uint64_t uploadSize;
        uint64_t downloadSize;

        HttpResponse(int s = 0,
                     const std::string& des = std::string(),
                     const HttpErrorCode& c = HttpErrorCode::Ok,
                     const WebSocketHttpHeaders& h = WebSocketHttpHeaders(),
                     const std::string& p = std::string(),
                     const std::string& e = std::string(),
                     uint64_t u = 0,
                     uint64_t d = 0)
            : statusCode(s)
            , description(des)
            , errorCode(c)
            , headers(h)
            , payload(p)
            , errorMsg(e)
            , uploadSize(u)
            , downloadSize(d)
        {
            ;
        }
    };

    using HttpResponsePtr = std::shared_ptr<HttpResponse>;
    using HttpParameters = std::map<std::string, std::string>;
    using Logger = std::function<void(const std::string&)>;
    using OnResponseCallback = std::function<void(const HttpResponsePtr&)>;

    struct HttpRequestArgs
    {
        std::string url;
        std::string verb;
        WebSocketHttpHeaders extraHeaders;
        std::string body;
        int connectTimeout;
        int transferTimeout;
        bool followRedirects;
        int maxRedirects;
        bool verbose;
        bool compress;
        Logger logger;
        OnProgressCallback onProgressCallback;
    };

    using HttpRequestArgsPtr = std::shared_ptr<HttpRequestArgs>;

    struct HttpRequest
    {
        std::string uri;
        std::string method;
        std::string version;
        WebSocketHttpHeaders headers;

        HttpRequest(const std::string& u,
                    const std::string& m,
                    const std::string& v,
                    const WebSocketHttpHeaders& h = WebSocketHttpHeaders())
            : uri(u)
            , method(m)
            , version(v)
            , headers(h)
        {
        }
    };

    using HttpRequestPtr = std::shared_ptr<HttpRequest>;

    class Http
    {
    public:
        static std::tuple<bool, std::string, HttpRequestPtr> parseRequest(std::shared_ptr<Socket> socket);
        static bool sendResponse(HttpResponsePtr response, std::shared_ptr<Socket> socket);

        static std::tuple<std::string, std::string, std::string> parseRequestLine(const std::string& line);
        static std::string trim(const std::string& str);
    };
}
