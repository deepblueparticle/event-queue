#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <queue>
#include <unordered_set>
#include <memory>


using Callback = std::function<void()>;
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;


struct Event {
    typedef enum {kSingle, kRepeating} Kind;
    typedef enum {kActive, kCompleted, kCanceled} Status;
    
    Event(Kind kind, Status status, Callback callback, int interval, TimePoint nextTime)
        : kind(kind)
        , status(status)
        , callback(std::move(callback))
        , interval(interval)
        , nextTime(nextTime)
    {}
    
    Kind kind;
    std::atomic<Status> status;
    Callback callback;
    std::chrono::milliseconds interval;
    TimePoint nextTime;
};

using EventPtr = std::shared_ptr<Event>;


class Task {
public:
    Task(EventPtr event) : event(event) {}
    bool completed() const {
        return event->status == Event::kCompleted;
    }
    void cancel() {
        switch (event->kind) {
            case Event::kSingle:
                event->status = Event::kCanceled;
                break;
            case Event::kRepeating:
                event->status = Event::kCompleted;
                break;
        }
    }

private:
    EventPtr event;
};


class EventQueue {
public:
    Task setTimeout(int interval, Callback callback) {
        return addEvent(Event::kSingle, interval, callback);
    }
    Task setInterval(int interval, Callback callback) {
        return addEvent(Event::kRepeating, interval, callback);
    }
    void run();
    void quit();

private:
    std::thread::id runThreadId_;
    std::atomic_bool run_ = {false};
    std::condition_variable waiter_;
    bool interrupt_;
    std::mutex mutex_;
    std::priority_queue<EventPtr> queue_;
    
    Task addEvent(Event::Kind kind, int interval, Callback callback);
};
