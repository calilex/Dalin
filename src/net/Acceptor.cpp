//
// Acceptor.cpp
//
// Copyright (c) 2017 Jiawei Feng
//

#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

using namespace Dalin::Net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
 : loop_(loop),
   acceptSocket_(SocketsOps::createNonblockingOrDie()),
   acceptChannel_(loop, acceptSocket_.fd()),
   listenning_(false)
{
    acceptSocket_.setReuseAddr(false);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();

    InetAddress peerAddr(0);
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        }
        else {
            SocketsOps::close(connfd);
        }
    }
}
