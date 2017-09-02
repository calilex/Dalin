//
// Connector.h
//
// Copyright (c) 2017 Jiawei Feng
//

#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "InetAddress.h"
#include "TimerId.h"
#include "../base/Noncopyable.h"

#include <functional>
#include <memory>

namespace Dalin {
namespace Net {

class Channel;
class EventLoop;

class Connector : Noncopyable {
public:
    typedef std::function<void (int sockfd)> NewConnectionCallback;

    Connector(EventLoop *loop, const InetAddress &serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void start(); // can be called in any thread
    void restart(); // must be called in loop thread
    void stop(); // can be called in any thread

    const InetAddress &serverAddress() const
    {
        return serverAddr_;
    }

private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setState(States s)
    {
        state_ = s;
    }

    void startInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop *loop_;
    InetAddress serverAddr_;
    bool connect_;
    States state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDaleyMs_;
    TimerId timerId_;
};

typedef std::shared_ptr<Connector> ConnectorPtr;

}
}

#endif
