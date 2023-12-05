/*
* Job manager
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_job_manager.h"

/**
* Initialize job manager and thread workers.
* @brief Set up job manager
*/
void JobManager::init() {
    _finishedLabel.store(0);

    // Get number of threads supported
    unsigned int nCores = std::thread::hardware_concurrency();
    std::printf("Number of threads supported : %i \n", nCores);
    uint32_t nbThreads = 1u > (nCores / 2) ? 1u : nCores;

    // Initialize workers
    for (uint32_t i = 0; i < nbThreads; ++i) {
        std::thread worker([] {
            std::function<void()> job;

            while(true) {
                // Retrieve job from task pool
                if (_jobPool.pop_front(job)) {
                    job();
                    _finishedLabel.fetch_add(1);
                } else {
                    // Put thread to sleep
                    std::unique_lock<std::mutex> lock(_mutex);
                    _cv.wait(lock);
                }
            }
        });
        worker.detach();
    }
};

/**
 * @brief Wake up and reschedule a thread worker
 */
void JobManager::poll() {
    _cv.notify_one(); // Unblock one of waiting threads
    std::this_thread::yield(); // Reschedule the threads
};

/**
 * @brief Submit job for execution
 */
void JobManager::execute(std::function<void()> const& job) {
    _currentLabel += 1;

    // Submit job until success
    while (!_jobPool.push_back(job)) {
        poll();
    }

    _cv.notify_one();
};

/**
 * Determine if thread workers should still be running.
 * Compare the latest job executed (finishedLabel) to the most recent job submitted (current label).
 * If labels are different, some workers should still be running to complete tasks.
 * @brief Check if thread workers should be busy.
 */
bool JobManager::is_busy() {
    return _finishedLabel.load() < _currentLabel;
};

/**
 * Block main thread utils the job pool is emptied of all pending jobs.
 * @brief Block main thread.
 */
void JobManager::wait() {
    while (is_busy()) {
        poll();
    }
};

/**
 * @brief Dispatch multiple jobs
 */
void JobManager::dispatch(uint32_t jobCount, uint32_t groupSize, std::function<void(JobDispatchData)> const& job) {
    if (jobCount == 0 || groupSize == 0) {
        return;
    }

    // Determine number of group required
    const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

    // Update main thread label to the most recent job submitted
    _currentLabel += groupCount;

    // For each job group
    for (uint32_t groupId = 0; groupId < groupCount; ++groupId) {

        // Set up a new job
        const auto& jobGroup = [jobCount, groupSize, job, groupId]() {
            const uint32_t groupJobOffset = groupId * groupSize;
            const uint32_t groupJobEnd = std::min(groupJobOffset + groupSize, jobCount);

            // Set group data
            JobDispatchData args;
                args._groupId = groupId;

                // Execute the job batch
                for (uint32_t jobId = groupJobOffset; jobId < groupJobEnd; ++jobId) {
                    args._id = jobId;
                    job(args);
                }
            };

            // Submit new job
            while (!_jobPool.push_back(jobGroup)) {
                poll();
            }
        }
    };