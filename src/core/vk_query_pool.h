/*
*  H2Vk - QueryTimestamp class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <string>
#include <map>
#include <utility>

#include "core/utilities/vk_resources.h"

/**
 * Queries timestamps provide a mechanism to return information about the processing of a sequence of Vulkan commands.
 * Query results and availabilities are stored in a Query pool.
 * @brief Query pool wrapper. 
 */
class QueryTimestamp final {
public:
    struct Query {
        std::string name;
        uint32_t start;
        uint32_t end;
    };

    /** @brief query pool */
    VkQueryPool _pool = VK_NULL_HANDLE;

    /** @brief Collection of pair of query used for timing */
    std::vector<QueryTimestamp::Query> _queries {};

    /** @brief Results associated per query pair */
    std::vector<std::pair<std::string, float>> _results {};

    /** @brief Buffer used to write query pool results */
    std::vector<uint64_t> _timestamps {};

    QueryTimestamp() = default;
    QueryTimestamp(const QueryTimestamp&) = delete;
    QueryTimestamp(QueryTimestamp &&) noexcept = delete;
    QueryTimestamp& operator=(const QueryTimestamp&) = delete;
    QueryTimestamp& operator=(QueryTimestamp &&) noexcept = delete;

    static void init(Device& device, QueryTimestamp* _pool, VkQueryType type, uint32_t count, VkQueryPipelineStatisticFlags bitmask = 0);
    
    void record(const char* label, uint32_t start, uint32_t end);
    uint32_t write(const VkCommandBuffer cmd, VkPipelineStageFlagBits flag);
    void results();
    void reset(const VkCommandBuffer cmd);
    void destroy();

private:
    /** @brief logical device */
    VkDevice _device = VK_NULL_HANDLE;

    /** @brief number of writing command called */
    uint32_t _count = 0;

    /** @brief How many nanoseconds a timestep translates to on the device */
    float _timestampPeriod = 1.f;
};