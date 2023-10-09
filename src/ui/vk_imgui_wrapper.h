/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <functional>
#include <array>

#include "imgui_internal.h"

namespace ImGui {
    bool InputFloat3(const char* label, const std::function<std::array<float, 3> ()>& getter, const std::function<void (std::array<float, 3>)>& setter, const char* format = "%.3f", ImGuiInputTextFlags flags = 0) {
        std::array<float, 3> value = getter();
        PushID(label);
        bool updated = ImGui::InputFloat3(label, value.data(), format, flags);
        PopID();
        if (updated) setter(value);
        return updated;
    }

    bool InputFloat2(const char* label, const std::function<std::array<float, 2> ()>& getter, const std::function<void (std::array<float, 2>)>& setter, const char* format = "%.3f", ImGuiInputTextFlags flags = 0) {
        std::array<float, 2> value = getter();
        bool updated = ImGui::InputFloat2(label, value.data(), format, flags);
        if (updated) setter(value);
        return updated;
    }

    bool SliderFloat(const char* label, const std::function<float ()>& getter, const std::function<void (float)>& setter, float v_min, float v_max, const char* format = "%.3f", ImGuiInputTextFlags flags = 0) {
        float value = getter();
        bool updated =  ImGui::SliderFloat(label, &value, v_min, v_max, format, flags);
        if (updated) setter(value);
        return updated;
    }

    bool Combo(const char* label, const std::function<int ()>& getter, const std::function<void (int)>& setter, const char* const items[], int items_count, int height_in_items = -1) {
        int current_item = getter();
        bool updated = ImGui::Combo(label, &current_item, items, items_count, height_in_items);
        if (updated) setter(current_item);
        return updated;
    }

//    bool Checkbox(const char* label, const std::function<bool ()>& getter, const std::function<void (bool)>& setter) {
//        bool current_item = getter();
//        bool updated = ImGui::Checkbox(label, current_item);
//        if (updated) setter(current_item);
//        return updated;
//    }
}
