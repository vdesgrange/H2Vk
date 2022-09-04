#pragma once

#include "vk_semaphore.h"
#include "vk_fence.h"
#include "vk_command_pool.h"
#include "vk_command_buffer.h"

constexpr unsigned int FRAME_OVERLAP = 2;


//class Frame final {
//public:
//    Frame();
//
//    FrameData& get_current_frame(int frameNumber);
//};