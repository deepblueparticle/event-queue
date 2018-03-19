#include "event_queue.h"


namespace {

TimePoint now() {
    return std::chrono::system_clock::now();
};

} // namespace


bool operator<(const EventPtr& lhs, const EventPtr& rhs) {
    return lhs->triggerTime > rhs->triggerTime;
}


TaskPtr EventQueue::addEvent(Event::Kind kind, int interval, Callback callback) {
    if (interval < 0) {
        throw std::runtime_error("interval must be non-negative.");
    }
    
    EventPtr event(new Event(
        kind,
        Event::kActive,
        callback,
        std::chrono::milliseconds(interval),
        now() + std::chrono::milliseconds(interval)
    ));
    
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(event);
    interrupt_ = true;
    lock.unlock();
    
    waiter_.notify_one();
    
    return std::make_unique<Task>(event);
}


void EventQueue::run() {
    bool expected = false;
    if (!run_.compare_exchange_strong(expected, true)) {
        return;
    }
    runThreadId_ = std::this_thread::get_id();
    interrupt_ = false;
    while (run_) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            waiter_.wait(lock, [this]() {return interrupt_;});
            interrupt_ = false;
            if (!run_) {
                break;
            }
        }
        EventPtr event = queue_.top();
        if (event->status != Event::kActive) {
            queue_.pop();
            continue;
        }
        
        waiter_.wait_until(lock, event->triggerTime, [this]() {return interrupt_;});
        
        if (interrupt_) {
            interrupt_ = false;
        } else {
            queue_.pop();
            if (event->status == Event::kActive){
                event->callback();
                if (event->kind == Event::kRepeating) {
                    event->triggerTime = now() + event->triggerInterval;
                    queue_.push(event);
                } else {
                    event->status = Event::kCompleted;
                }
            }
        }
    }
}


void EventQueue::quit() {
    bool wasRunning = run_.exchange(false);
    // We can't do this from the same thread as quit is called from the locked section.
    if (wasRunning && std::this_thread::get_id() != runThreadId_) {
        std::unique_lock<std::mutex> lock(mutex_);
        interrupt_ = true;
        lock.unlock();
        waiter_.notify_one();
    }
}
