#pragma once
#include <string_view>

namespace SeedCore
{
	namespace ImGui
	{
		using namespace ::ImGui;

		SEEDCORE_API Bool CalclateNumber(std::string_view expression, Double& result, Size selectionIndex = 0, Size selectionCount = 1);

		SEEDCORE_API Bool CalclateNumber(std::string_view expression, Double& result, Size selectionIndex, Size selectionCount, Double currentValue);

		SEEDCORE_API Bool DragFloat(const Char* label, Float* value, Float speed = 1.0f, Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool DragFloat2(const Char* label, Float value[2], Float speed = 1.0f, Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool DragFloat3(const Char* label, Float value[3], Float speed = 1.0f, Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool DragFloat4(const Char* label, Float value[4], Float speed = 1.0f, Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderFloat(const Char* label, Float* value, Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderFloat2(const Char* label, Float value[2], Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderFloat3(const Char* label, Float value[3], Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderFloat4(const Char* label, Float value[4], Float min = 0.0f, Float max = 0.0f, const Char* format = "%.3f", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool DragInt(const Char* label, Int* value, Float speed = 1.0f, Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool DragInt2(const Char* label, Int value[2], Float speed = 1.0f, Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool DragInt3(const Char* label, Int value[3], Float speed = 1.0f, Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool DragInt4(const Char* label, Int value[4], Float speed = 1.0f, Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderInt(const Char* label, Int* value, Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderInt2(const Char* label, Int value[2], Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderInt3(const Char* label, Int value[3], Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool SliderInt4(const Char* label, Int value[4], Int min = 0, Int max = 0, const Char* format = "%d", ImGuiSliderFlags flags = 0);

		SEEDCORE_API Bool ColorEdit3(const Char* label, Float color[3], ImGuiColorEditFlags flags = 0);

		SEEDCORE_API Bool ColorEdit4(const Char* label, Float color[4], ImGuiColorEditFlags flags = 0);

		SEEDCORE_API Bool SliderAngle(const Char* label, Float* radians, Float minDegrees = -360.0f, Float maxDegrees = 360.0f, const Char* format = "%.0f deg", ImGuiSliderFlags flags = 0);
	}
}