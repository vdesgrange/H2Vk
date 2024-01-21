/*
*  H2Vk - QueryTimestamp class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_query_pool.h"

/**
 * @brief Destroy query pool
 * @note to be explicitely call before device is destroyed
 */
void QueryTimestamp::destroy() {
    if (_device && _pool) {
        vkDestroyQueryPool(_device, _pool, nullptr);
        _pool = VK_NULL_HANDLE;
    }
}

/**
 * @brief Initialize a query pool
 * @param device device wrapper
 * @param pool pointer to query pool to be initialized
 * @param type query type
 * @param count number of queries
 * @param bitmask specify which counters will be returned in queries on the pool (default : 0)
 */
void QueryTimestamp::init(Device& device, QueryTimestamp* pool, VkQueryType type, uint32_t count, VkQueryPipelineStatisticFlags bitmask) {
    VkQueryPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.queryType = type;
    info.queryCount = count;
    info.pipelineStatistics = bitmask; // Ignored if not VK_QUERY_TYPE_PIPELINE_STATISTICS

    pool->_device = device._logicalDevice;
    pool->_count = count;
    pool->_queries.clear();
    pool->_timestamps.clear();
    pool->_timestamps.shrink_to_fit();
    pool->_timestamps.resize(count);
     // pool->_timestampPeriod = device._gpuProperties.limits.timestampPeriod; // bug moltenvk - incorrect value retrieved

    VK_CHECK(vkCreateQueryPool(device._logicalDevice, &info, nullptr, &pool->_pool));
}

uint32_t QueryTimestamp::write(const VkCommandBuffer cmd, VkPipelineStageFlagBits flag) {
    assert(_pool);
    const uint32_t count = _count; // Detach from CPU
    vkCmdWriteTimestamp(cmd, flag, _pool, count);
    _count++;

    return count;
}

void QueryTimestamp::record(const char* label, uint32_t start, uint32_t end) {
    _queries.emplace_back(QueryTimestamp::Query({label, start, end}));
}

void QueryTimestamp::results() {
    assert(_pool);
    vkGetQueryPoolResults(_device, _pool, 0, _count, _count * sizeof(uint64_t), _timestamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    _results.clear();
    _results.shrink_to_fit();
    _results.reserve(_queries.size());

    // Get result per render pass
    for (const auto& val : _queries) {
        const uint64_t start = _timestamps.at(val.start);
        const uint64_t end = _timestamps.at(val.end);
        const uint64_t diff = end - start; // Nanoseconds * 10^9

        float time = static_cast<float>(diff) * _timestampPeriod * 0.000001f; // Milliseconds = Nanoseconds * 10^-6

        _results.emplace_back(std::make_pair(val.name, time));
    }
}

void QueryTimestamp::reset(const VkCommandBuffer cmd) {
    assert(_pool);
    const uint32_t count = _count; // _timestamps.size(); // Detach from CPU
    vkCmdResetQueryPool(cmd, _pool, 0, count);
    _count = 0;
    _queries.clear();
    _queries.shrink_to_fit();
}