/*
* Job manager
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <functional>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <functional>
#include <deque>

/**
 * @brief Thread safe queue
 */
template<typename T>
class ThreadSafeQueue final {
public:
    ThreadSafeQueue() {};
    ~ThreadSafeQueue() {};

    bool push_back(const T& in) {
        bool result = false;
        {
            std::lock_guard<std::mutex> lock(_m);
            _q.push_back(in);
            _c.notify_one();
            result = true;
        }

        return result;
    }

    bool pop_front(T& out) {
        std::lock_guard<std::mutex> lock(_m);
        if (!_q.empty()) {
            out = _q.front();
            _q.pop_front();
            return true;
        }

        return false;
    }

private:
    std::deque<T> _q;
    std::condition_variable _c;
    std::mutex _m;
};

/**
 * Details associated to a dispatched job 
 * @brief Job information
 */
struct JobDispatchData {
    /** @brief job id */
    uint32_t _id;
    /** @brief group id */
    uint32_t _groupId;
};

/**
 * @brief Job manager
 */
namespace JobManager {
    /** @brief number of worker threads */
    uint32_t _numThreads = 0;
    /** @brief condition variable used by main thread to wake secondary threads*/
    std::condition_variable _cv;
    /** @brief Associated to condition variable */
    std::mutex _mutex;
    /** @brief Job pool */
    ThreadSafeQueue<std::function<void()>> _jobPool;
    /** @brief Label of latest job submitted by main thread*/
    uint32_t _currentLabel;
    /** @brief State of execution. Latest job executed. */
    std::atomic<uint32_t> _finishedLabel;


    void init();
    void execute(std::function<void()> const& function);
    void dispatch(uint32_t count, uint32_t groupSize, std::function<void(JobDispatchData)> const& job);
    void poll();
    bool is_busy();
    void wait();
};
