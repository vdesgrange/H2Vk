#pragma once

#include <functional>
#include <array>

#include "imgui_internal.h"

namespace ImGui {

    bool InputFloat3(const char* label, const std::function<std::array<float, 3> ()>& getter, const std::function<void (std::array<float, 3>)>& setter, const char* format = "%.3f", ImGuiInputTextFlags flags = 0) {
        std::array<float, 3> value = getter();
        bool updated = ImGui::InputFloat3(label, value.data(), format, flags);
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

}
