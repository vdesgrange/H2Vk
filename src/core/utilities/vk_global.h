#pragma once

#include "core/utilities/vk_types.h"
#include "core/vk_semaphore.h"
#include "core/vk_fence.h"
#include "core/vk_command_pool.h"
#include "core/vk_command_buffer.h"
#include "core/vk_buffer.h"

constexpr unsigned int FRAME_OVERLAP = 2;

inline FrameData g_frames[FRAME_OVERLAP];