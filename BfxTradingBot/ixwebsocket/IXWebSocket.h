/*
 *  IXWebSocket.h
 *  Author: Benjamin Sergeant
 *  Copyright (c) 2017-2018 Machine Zone, Inc. All rights reserved.
 *
 *  WebSocket RFC
 *  https://tools.ietf.org/html/rfc6455
 */

#pragma once

#include "IXProgressCallback.h"
#include "IXWebSocketCloseConstants.h"
#include "IXWebSocketErrorInfo.h"
#include "IXWebSocketHttpHeaders.h"
#include "IXWebSocketMessage.h"
#include "IXWebSocketPerMessageDeflateOptions.h"
#include "IXWebSocketSendInfo.h"
#include "IXWebSocketTransport.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace ix
{
    // https://developer.mozilla.org/en-US/docs/Web/API/WebSocket#Ready_state_constants
    enum class ReadyState
    {
        Connecting = 0,
        Open = 1,
        Closing = 2,
        Closed = 3
    };

    using OnMessageCallback = std::function<void(const WebSocketMessagePtr&,void* usrPtr)>;

    using OnTrafficTrackerCallback = std::function<void(size_t size, bool incoming)>;

    class WebSocket
    {
    public:
        WebSocket();
        ~WebSocket();

        void setUrl(const std::string& url);
        void setPerMessageDeflateOptions(
            const WebSocketPerMessageDeflateOptions& perMessageDeflateOptions);
        void setHeartBeatPeriod(int heartBeatPeriodSecs);
        void setPingInterval(int pingIntervalSecs); // alias of setHeartBeatPeriod
        void setPingTimeout(int pingTimeoutSecs);
        void enablePong();
        void disablePong();
        void disablePerMessageDeflate();

        // Run asynchronously, by calling start and stop.
        void start();

        // stop is synchronous
        void stop(uint16_t code = WebSocketCloseConstants::kNormalClosureCode,
                  const std::string& reason = WebSocketCloseConstants::kNormalClosureMessage);

        // Run in blocking mode, by connecting first manually, and then calling run.
        WebSocketInitResult connect(int timeoutSecs);
        void run();

        // send is in binary mode by default
        WebSocketSendInfo send(const std::string& data,
                               bool binary = false,
                               const OnProgressCallback& onProgressCallback = nullptr);
        WebSocketSendInfo sendBinary(const std::string& text,
                                     const OnProgressCallback& onProgressCallback = nullptr);
        WebSocketSendInfo sendText(const std::string& text,
                                   const OnProgressCallback& onProgressCallback = nullptr);
        WebSocketSendInfo ping(const std::string& text);

        void close(uint16_t code = WebSocketCloseConstants::kNormalClosureCode,
                   const std::string& reason = WebSocketCloseConstants::kNormalClosureMessage);

        void setOnMessageCallback(const OnMessageCallback& callback);
        static void setTrafficTrackerCallback(const OnTrafficTrackerCallback& callback);
        static void resetTrafficTrackerCallback();

        ReadyState getReadyState() const;
        static std::string readyStateToString(ReadyState readyState);

        const std::string& getUrl() const;
        const WebSocketPerMessageDeflateOptions& getPerMessageDeflateOptions() const;
        int getHeartBeatPeriod() const;
        int getPingInterval() const;
        int getPingTimeout() const;
        size_t bufferedAmount() const;

        void enableAutomaticReconnection();
        void disableAutomaticReconnection();
        bool isAutomaticReconnectionEnabled() const;
        void setUsrPtr(void* ptr);

    private:
        WebSocketSendInfo sendMessage(const std::string& text,
                                      SendMessageKind sendMessageKind,
                                      const OnProgressCallback& callback = nullptr);

        bool isConnected() const;
        bool isClosing() const;
        void checkConnection(bool firstConnectionAttempt);
        static void invokeTrafficTrackerCallback(size_t size, bool incoming);

        // Server
        WebSocketInitResult connectToSocket(int fd, int timeoutSecs);

        WebSocketTransport _ws;

        std::string _url;
        WebSocketPerMessageDeflateOptions _perMessageDeflateOptions;
        mutable std::mutex _configMutex; // protect all config variables access

        OnMessageCallback _onMessageCallback;
        static OnTrafficTrackerCallback _onTrafficTrackerCallback;

        std::atomic<bool> _stop;
        std::atomic<bool> _automaticReconnection;
        std::thread _thread;
        std::mutex _writeMutex;

        std::atomic<int> _handshakeTimeoutSecs;
        static const int kDefaultHandShakeTimeoutSecs;

        // enable or disable PONG frame response to received PING frame
        bool _enablePong;
        static const bool kDefaultEnablePong;

        // Optional ping and pong timeout
        int _pingIntervalSecs;
        int _pingTimeoutSecs;
        static const int kDefaultPingIntervalSecs;
        static const int kDefaultPingTimeoutSecs;
        void* usrPtr;

        friend class WebSocketServer;
    };
} // namespace ix
