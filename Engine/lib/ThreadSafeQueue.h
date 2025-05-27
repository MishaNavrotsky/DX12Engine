#pragma once
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>

template <typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{ false };  // To indicate when to stop

public:
    void push(const T& item)
    {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(item);
        cv.notify_one();  // Notify one waiting thread
    }

    bool pop(T& item)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty())
        {
            return false;  // Queue is empty
        }
        item = queue.front();
        queue.pop();
        return true;
    }

    bool popWait(T& item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty() || done.load(); });

        if (queue.empty()) return false;

        item = queue.front();
        queue.pop();
        return true;
    }

    void shutdown()
    {
        done.store(true);
        cv.notify_all();  // Wake up all waiting threads
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    // Pop all elements and return them as a vector
    std::vector<T> popAll()
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<T> items;
        while (!queue.empty())
        {
            items.push_back(queue.front());
            queue.pop();
        }
        return items;
    }
};