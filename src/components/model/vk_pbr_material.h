/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vk_model.h"

namespace PBR {
    // Metal
    static const PBRProperties gold = {{1.0f, 0.765557f, 0.336057f, 1.0f}, 1.0f, 0.1f, 1.0f};
    static const PBRProperties copper = {{0.955008f, 0.637427f, 0.538163f, 1.0f}, 1.0f, 0.1, 1.0f};
    static const PBRProperties chromium = {{0.549585f, 0.556114f, 0.554256f, 1.0f},1.0f, 0.1, 1.0f};
    static const PBRProperties cobalt = {{0.662124f, 0.654864f, 0.633732f, 1.0f},1.0f, 0.1, 1.0f};
    static const PBRProperties nickel = {{0.659777f, 0.608679f, 0.525649f, 1.0f}, 1.0f, 0.1, 1.0f};
    static const PBRProperties titanium = {{0.541931f, 0.496791f, 0.449419f, 1.0f}, 1.0f, 0.1, 1.0f};
    static const PBRProperties platinum = {{0.672411f, 0.637331f, 0.585456f, 1.0f}, 1.0f, 0.1, 1.0f};
};
