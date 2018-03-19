#include <iostream>
#include <iomanip>
#include <sstream>
#include "event_queue.h"


using SystemClock = std::chrono::system_clock;


std::string timeUpToMilliseconds(const TimePoint& timePoint) {
    auto timePast = timePoint.time_since_epoch();
    auto secondsPast = std::chrono::duration_cast<std::chrono::seconds>(timePast);
    
    std::chrono::milliseconds left =
        std::chrono::duration_cast<std::chrono::milliseconds>(timePast) -
        std::chrono::duration_cast<std::chrono::milliseconds>(secondsPast);
    std::time_t timePointC = TimePoint::clock::to_time_t(timePoint);
    
    std::stringstream ret;
    ret << std::put_time(std::localtime(&timePointC), "%T.") << left.count();
    return ret.str();
}


std::string attachTime(const std::string& message) {
    std::stringstream ss;
    ss << timeUpToMilliseconds(SystemClock::now()) << " " << message;
    return ss.str();
}


Callback createMessageCallback(const std::string& message) {
    return [message]() {
        std::cout << attachTime(message);
    };
}


int main() {
    EventQueue q;
    std::vector<TaskPtr> tasks;
    
    std::cout << attachTime("Adding 2 events in queue\n");
    
    tasks.push_back(q.setTimeout(2500, createMessageCallback("Event 1: single, 2500ms\n")));
    tasks.push_back(q.setInterval(1000, createMessageCallback("Event 2: repeating, 1000ms\n")));
    
    std::cout << attachTime("Starting queue in a separate thread\n");
    std::thread runThread([&q](){q.run();});

    std::cout << attachTime("Pause the main thread for a 5500ms\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(5500));
    
    std::cout << attachTime("Adding 2 another events\n");
    tasks.push_back(q.setInterval(700, createMessageCallback("Event 3: repeating, 700ms\n")));
    tasks.push_back(q.setTimeout(3000, createMessageCallback("Event 4: single, 3000ms\n")));
    
    std::cout << attachTime("Pause the main thread for a 4000ms\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    
    std::cout << attachTime("Cancelling periodic event 2\n");
    tasks[1]->cancel();
    
    std::cout << attachTime("Pause the main thread for a 2000ms\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    std::cout << attachTime("Adding 4 another events\n");
    tasks.push_back(q.setInterval(500, createMessageCallback("Event 5: repeating, 500ms\n")));
    tasks.push_back(q.setTimeout(1500, createMessageCallback("Event 6: single, 1500ms\n")));
    tasks.push_back(q.setTimeout(1000, createMessageCallback("Event 7: single, 1000ms\n")));
    tasks.push_back(q.setTimeout(5000, createMessageCallback("Event 8: single, 6000ms\n")));
    
    std::cout << attachTime("Cancelling timeout event 6, so it won't be executed\n");
    tasks[5]->cancel();
    
    std::cout << attachTime("Stopping the queue\n");
    q.quit();
    runThread.join();
    
    std::cout << attachTime("Pause the main thread for a 5000ms\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    
    std::cout << "\nThe time keep flowing and when we start the queue again there will be a\n"
                 "bunch of events to execute immediately.\n\n";
    
    std::cout << attachTime("Adding quit event, so we can call run from the main thread. "
                            "Timeout is 10 seconds.\n");
    auto eQuit = q.setTimeout(10000, [&q](){
        std::cout << attachTime("quit is called as an event.\n");
        q.quit();
    });
    
    
    std::cout << attachTime("Restarting queue in the same thread\n");
    q.run();
    
    std::cout << "\nNow let's check event statuses:\n\n";
    for (int i = 0; i < tasks.size(); ++i) {
        std::cout << "Event " << i + 1 << " "
                  << (tasks[i]->completed() ? "completed.\n" : "not completed.\n");
    }
    
    std::cout << "\nAll seems correct.\n";
    
    return 0;
}
