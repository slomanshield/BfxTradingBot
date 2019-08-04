/*
 *  IXHttpServer.h
 *  Author: Benjamin Sergeant
 *  Copyright (c) 2018 Machine Zone, Inc. All rights reserved.
 */

#pragma once

#include "IXSocketServer.h"
#include "IXWebSocket.h"
#include "IXHttp.h"
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility> // pair

namespace ix
{
    class HttpServer final : public SocketServer
    {
    public:
        using OnConnectionCallback =
            std::function<HttpResponsePtr(HttpRequestPtr, std::shared_ptr<ConnectionState>)>;

        HttpServer(int port = SocketServer::kDefaultPort,
                   const std::string& host = SocketServer::kDefaultHost,
                   int backlog = SocketServer::kDefaultTcpBacklog,
                   size_t maxConnections = SocketServer::kDefaultMaxConnections);
        virtual ~HttpServer();
        virtual void stop() final;

        void setOnConnectionCallback(const OnConnectionCallback& callback);

    private:
        // Member variables
        OnConnectionCallback _onConnectionCallback;
        std::atomic<int> _connectedClientsCount;

        // Methods
        virtual void handleConnection(int fd,
                                      std::shared_ptr<ConnectionState> connectionState) final;
        virtual size_t getConnectedClientsCount() final;

        void setDefaultConnectionCallback();
    };
} // namespace ix

