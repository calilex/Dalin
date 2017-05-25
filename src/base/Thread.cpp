//
// Thread.cpp
//
// Copyright (c) 2017 Jiawei Feng
//

#include "CurrentThread.h"
#include "Thread.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sys/prctl.h>
#include <exception>

namespace Xgeer {
namespace CurrentThread {
    __thread pid_t t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char *t_threadName = "unknown";

    void cacheTid()
    {
        if (t_cachedTid == 0) {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }

    bool isMainThread()
    {
        return tid() == getpid();
    }

    void sleepUsec(int64_t usec)
    {
        struct timespec ts = {0, 0};
        ts.tv_sec = static_cast<time_t>(usec / (1000 * 1000));
        ts.tv_nsec = static_cast<long>(usec % (1000 * 1000) * 1000);
        ::nanosleep(&ts, NULL);
    }
}

namespace Detail {
    void *startThread(void *arg)
    {
        Thread *thread = static_cast<Thread *>(arg);
        thread->run();
        return NULL;
    }
}
}

using namespace Xgeer;

Thread::Thread(const ThreadFunc &func, const string &name)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_) {
        pthread_detach(pthreadId_);
    }
}

void Thread::setDefaultName()
{
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread%d", CurrentThread::tid());
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;

    if (pthread_create(&pthreadId_, NULL, Detail::startThread, this)) {
        started_ = false;
        printf("Failed in pthread_create\n"); // FIXME: log
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}

void Thread::run()
{
    tid_ = CurrentThread::tid();

    CurrentThread::t_threadName = name_.empty() ? "XgeerThread" : name_.c_str();
    ::prctl(PR_SET_NAME, CurrentThread::t_threadName);

    try {
        func_();
        CurrentThread::t_threadName = "finished";
    }
    catch (std::exception &ex) {
        CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...) {
        CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw;
    }
}
