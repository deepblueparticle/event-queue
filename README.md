Implementation of an event queue as a test task solution.

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
The method `cancel` removes the tasks from the queue. The method `completed` checks whether a task was completed: a timeout" task is completed when it was executed, an "interval" task is completed when it was canceled. 

It was also asked to call `cancel` method when a `Task` object is destroyed, but as specified it looks like not the best idea. If we pass or return `Task` by value then temporary objects are created and destroyed and the corresponding event will be removed from the queue unexpectedly. To overcome this problems I decided to return `std::unique_ptr<Task>` from these methods. This way we still can cancel `Task` manually and when the pointer goes out of scope the `Task` is destroyed and the corresponding event is removed from the queue as well. So it seems like a satisfactory solution. Note that passing `std::unique_ptr<Task>` around relies on move semantics.

The algorithm
-------------
Events are stored in a priority queue with target times of execution as keys. At each step an event is extracted from the top of the queue. If it was marked as canceled, we move to the next step. Otherwise we wait until the trigger time and execute the callback. If the event was repeating, we insert another event with the same callback and the updated target time.

To allow other threads to modify the queue we use condition variables to implement waiting. If the wait was interrupted, we don't execute the selected event and start the step from the beginning.

There is a special interaction between `run` and `quit` methods to account for multithreading. The problem is that we can't guarantee `run` and `quit` execution order from different threads. It means that a undesired (perhaps rare) situation is possible when `quit` was called before `run` and then the queue runs forever. To avoid that `quit` is implemented such that it waits until `run` is called and then cancels it. The consequence of that is that we should not run `quit`, when there was no `run` calls in this thread before or in other threads.

See the details regarding multithreading in the code. I'm not sure that I get everything right, but it seems reasonable.

Building
========
Only C++14 compliant compiler is required. There is a single executable target `event_queue_demo` in `CMakeLists.txt`, which demonstrates `EventQueue` usage in multi- and single-threaded contexts.
