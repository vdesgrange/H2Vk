/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_types.h"
#include "core/vk_semaphore.h"
#include "core/vk_fence.h"
#include "core/vk_command_pool.h"
#include "core/vk_command_buffer.h"
#include "core/vk_buffer.h"

constexpr uint32_t FRAME_OVERLAP = 2;
constexpr uint32_t MAX_OBJECTS = 10000;

inline FrameData g_frames[FRAME_OVERLAP];