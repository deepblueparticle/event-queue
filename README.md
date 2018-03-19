A demo implementation of an event queue.


Task description
================
The task was to implement an "event queue" class with an interface: 

```cpp
class EventQueue
{
public:
    void run();
    void quit();
    Task setInterval(int interval, Callback callback);
    Task setTimeout(int timeout, Callback callback);
};
```
Here `setInterval` and `setTimeout` add events to be executed repeatedly or once respectively. After `run` is called the task callbacks are executed in the loop (in the same thread) according to their timings. The method `quit` exits the loop, it can be called from another thread or added for execution to the queue beforehand.  

The class `Task` has the interface:
```cpp
class Task
{
public:
    bool completed() const;
    void cancel();
};
```
The method `cancel` removes the tasks from the queue. The method `completed` checks whether a task was completed: a "timeout" task is completed when it was executed, a "interval" task is completed when it was canceled. 

It was also asked to call `cancel` method when a `Task` object is destroyed, but as specified it looks like not the best idea. If we pass or return `Task` by value then temporary objects are created and destroyed and the corresponding event will be removed from the queue unexpectedly. To overcome this problems I decided to return `TaskPtr = std::unique_ptr<Task>` from these methods. This way we still can cancel `Task` manually and when a pointer goes out of a scope the `Task` is destroyed and the corresponding event is removed from the queue as well. So it seems like a satisfactory solution. Note that passing `TaskPtr` around relies on move semantics.


Building
========
Only C++14 compliant compiler is required. There is a single executable target `event_queue_demo` in CMakeLists.txt, which shows `EventQueue` usage in multi- and single-threaded contexts.
