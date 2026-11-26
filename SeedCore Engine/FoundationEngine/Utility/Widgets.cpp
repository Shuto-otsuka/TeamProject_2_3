#include <FoundationEngine/Utility/Widgets.h>

namespace SeedCore
{
	namespace ImGui
	{
		class ExpressionOperations
		{
		public:
			static Bool Add(Double left, Double right, Double& result)
			{
				result = left + right;
				return std::isfinite(result);
			}

			static Bool Subtract(Double left, Double right, Double& result)
			{
				result = left - right;
				return std::isfinite(result);
			}

			static Bool Multiply(Double left, Double right, Double& result)
			{
				result = left * right;
				return std::isfinite(result);
			}

			static Bool Divide(Double left, Double right, Double& result)
			{
				if (right == 0.0)
				{
					return false;
				}
				result = left / right;
				return std::isfinite(result);
			}

			static Bool Power(Double left, Double right, Double& result)
			{
				result = std::pow(left, right);
				return std::isfinite(result);
			}

			static Bool Modulo(Double left, Double right, Double& result)
			{
				if (right == 0.0)
				{
					return false;
				}
				result = std::fmod(left, right);
				return std::isfinite(result);
			}

			static Bool Factorial(Double operand, Double& result)
			{
				if (operand < 0.0 || operand != std::trunc(operand))
				{
					return false;
				}

				Double product = 1.0;
				for (Int factor = 2; factor <= operand; ++factor)
				{
					product *= factor;
					if (!std::isfinite(product))
					{
						return false;
					}
				}
				result = product;
				return true;
			}
		};

		Double CalclateRandom(Double first, Double last)
		{
			if (first == last)
			{
				return first;
			}
			std::random_device random;
			const Double fraction = std::generate_canonical<Double, std::numeric_limits<Double>::digits>(random);
			return std::lerp(first, last, fraction);
		}

		Double CalclateLinear(Double first, Double last, Size selectionIndex, Size selectionCount)
		{
			if (selectionCount <= 1)
			{
				return first;
			}
			return std::lerp(first, last, static_cast<Double>(selectionIndex) / static_cast<Double>(selectionCount - 1));
		}

		Bool CalclateNumber(std::string_view expression, Double& result, Size selectionIndex, Size selectionCount)
		{
			return CalclateNumber(expression, result, selectionIndex, selectionCount, std::numeric_limits<Double>::quiet_NaN());
		}

		Bool CalclateNumber(std::string_view expression, Double& result, Size selectionIndex, Size selectionCount, Double currentValue)
		{
			constexpr Size bufferSize = 128;
			if (expression.empty() || expression.size() >= bufferSize || selectionIndex >= selectionCount || expression.find('\0') != std::string_view::npos)
			{
				return false;
			}
			const Size first = expression.find_first_not_of(" \t");
			if (first == std::string_view::npos)
			{
				return false;
			}
			expression.remove_prefix(first);
			const auto apply = [](Char op, Double left, Double right, Double& value)
			{
				switch (op)
				{
				case '+':
					return ExpressionOperations::Add(left, right, value);
				case '-':
					return ExpressionOperations::Subtract(left, right, value);
				case '*':
					return ExpressionOperations::Multiply(left, right, value);
				case '/':
					return ExpressionOperations::Divide(left, right, value);
				case '^':
					return ExpressionOperations::Power(left, right, value);
				case '%':
					return ExpressionOperations::Modulo(left, right, value);
				default:
					return false;
				}
			};

			Char assignment = '\0';
			if (expression.size() >= 2 && expression[1] == '=')
			{
				assignment = expression.front();
				if (!std::isfinite(currentValue) || (assignment != '+' && assignment != '-' && assignment != '*' && assignment != '/' && assignment != '^' && assignment != '%'))
				{
					return false;
				}
				expression.remove_prefix(2);
			}
			const Char* end = expression.data() + expression.size();
			Double numbers[bufferSize];
			Size numberCount = 0;
			Char operators[bufferSize];
			Size operatorCount = 0;
			const Char* cursor = expression.data();

			Bool expectNumber = true;

			for (;;)
			{
				while (cursor != end && (*cursor == ' ' || *cursor == '\t'))
				{
					++cursor;
				}
				const Char token = cursor == end ? '\0' : *cursor;
				if (expectNumber)
				{
					if (token == 'R' || token == 'L')
					{
						++cursor;
						while (cursor != end && (*cursor == ' ' || *cursor == '\t'))
						{
							++cursor;
						}
						if (cursor == end || *cursor != '(')
						{
							return false;
						}
						operators[operatorCount++] = token;
						++cursor;
						continue;
					}
					if (token == '+' || token == '-' || token == '(')
					{
						if (token != '+')
						{
							operators[operatorCount++] = token == '-' ? '~' : '(';
						}
						++cursor;
						continue;
					}

					Double number = 0.0;
					const std::from_chars_result parsed = std::from_chars(cursor, end, number);
					if (parsed.ec != std::errc{} || !std::isfinite(number))
					{
						return false;
					}
					numbers[numberCount++] = number;
					cursor = parsed.ptr;
					expectNumber = false;
					continue;
				}

				if (token == '!')
				{
					Double& operand = numbers[numberCount - 1];
					if (!ExpressionOperations::Factorial(operand, operand))
					{
						return false;
					}
					++cursor;
					continue;
				}

				if (token != '+' && token != '-' && token != '*' && token != '/' && token != '%' && token != '^' && token != ')' && token != ',' && token != '\0')
				{
					return false;
				}

				while (operatorCount != 0)
				{
					const Char op = operators[operatorCount - 1];
					if (op == '(' || op == 'R' || op == 'L' || op == 'r' || op == 'l')
					{
						break;
					}
					if (token == '^' || ((token == '*' || token == '/' || token == '%') && (op == '+' || op == '-')))
					{
						break;
					}
					--operatorCount;
					if (op == '~')
					{
						numbers[numberCount - 1] = -numbers[numberCount - 1];
						continue;
					}

					const Double right = numbers[numberCount - 1];
					--numberCount;
					Double& left = numbers[numberCount - 1];
					if (!apply(op, left, right, left))
					{
						return false;
					}
				}

				if (token == '\0')
				{
					if (operatorCount != 0 || numberCount != 1)
					{
						return false;
					}
					Double next = numbers[0];
					if (assignment != '\0' && !apply(assignment, currentValue, next, next))
					{
						return false;
					}
					result = next;
					return true;
				}
				if (token == ',')
				{
					if (operatorCount == 0 || (operators[operatorCount - 1] != 'R' && operators[operatorCount - 1] != 'L'))
					{
						return false;
					}
					operators[operatorCount - 1] = operators[operatorCount - 1] == 'R' ? 'r' : 'l';
					expectNumber = true;
				}
				else if (token == ')')
				{
					if (operatorCount == 0 || operators[operatorCount - 1] == 'R' || operators[operatorCount - 1] == 'L')
					{
						return false;
					}
					const Char group = operators[--operatorCount];
					if (group == 'r' || group == 'l')
					{
						const Double last = numbers[--numberCount];
						Double& first = numbers[numberCount - 1];
						first = group == 'r' ? CalclateRandom(first, last) : CalclateLinear(first, last, selectionIndex, selectionCount);
					}
				}
				else
				{
					operators[operatorCount++] = token;
					expectNumber = true;
				}
				++cursor;
			}
		}

		template<typename T>
		Bool Calclate(const Char* label, T* value, Int components, T min, T max, Float speed, Bool slider, const Char* format, ImGuiSliderFlags flags)
		{
			constexpr ImGuiDataType dataType = std::is_same_v<T, Int> ? ImGuiDataType_S32 : ImGuiDataType_Float;
			constexpr Size bufferSize = 128;

			if (::ImGui::GetCurrentWindow()->SkipItems)
			{
				return false;
			}
			ImGuiContext& context = *::ImGui::GetCurrentContext();
			const Bool editingText = context.ActiveId != 0 && context.ActiveId == context.InputTextState.ID;
			const Bool openExpression = ((::ImGui::GetIO().KeyCtrl && ::ImGui::IsMouseClicked(ImGuiMouseButton_Left)) || ::ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) && ::ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			::ImGui::BeginGroup();
			::ImGui::PushID(label);
			::ImGui::PushMultiItemsWidths(components, ::ImGui::CalcItemWidth());
			Bool changed = false;
			const Float spacing = ::ImGui::GetStyle().ItemInnerSpacing.x;
			for (Int axis = 0; axis < components; ++axis)
			{
				if (axis != 0)
				{
					::ImGui::SameLine(0.0f, spacing);
				}
				::ImGui::PushID(axis);
				const ImGuiID id = (editingText || openExpression) ? ::ImGui::GetID("##axis") : 0;
				Bool clicked = false;
				if (openExpression)
				{
					const ImVec2 start = ::ImGui::GetCursorScreenPos();
					const ImVec2 end = ImVec2(start.x + ::ImGui::CalcItemWidth(), start.y + ::ImGui::GetFrameHeight());
					clicked = ::ImGui::IsMouseHoveringRect(start, end);
				}
				const Bool canType = !(flags & (ImGuiSliderFlags_NoInput | ImGuiSliderFlags_ReadOnly)) && !(context.CurrentItemFlags & (ImGuiItemFlags_Disabled | ImGuiItemFlags_ReadOnly));
				if (canType && (clicked || (editingText && context.ActiveId == id)))
				{
					Char buffer[bufferSize]{};
					std::to_chars(buffer, buffer + sizeof(buffer) - 1, value[axis]);
					::ImGui::PushItemFlag(ImGuiItemFlags_LiveEditOnInputText, false);
					::ImGui::PushItemFlag(ImGuiItemFlags_NoMarkEdited, true);
					const Bool submitted = ::ImGui::InputText("##axis", buffer, sizeof(buffer), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
					::ImGui::PopItemFlag();
					::ImGui::PopItemFlag();

					Double result = 0.0;
					if (submitted && CalclateNumber(buffer, result, 0, 1, value[axis]))
					{
						if ((flags & ImGuiSliderFlags_ClampOnInput) && min <= max && (min != 0.0f || max != 0.0f || (flags & ImGuiSliderFlags_ClampZeroRange)))
						{
							result = std::clamp(result, static_cast<Double>(min), static_cast<Double>(max));
						}
						if constexpr (std::is_integral_v<T>)
						{
							result = std::trunc(result);
						}
						if (result >= (std::numeric_limits<T>::lowest)() && result <= (std::numeric_limits<T>::max)())
						{
							const T next = static_cast<T>(result);
							if (value[axis] != next)
							{
								value[axis] = next;
								changed = true;
								context.LastItemData.ItemFlags &= ~ImGuiItemFlags_NoMarkEdited;
								::ImGui::MarkItemEdited(id);
							}
						}
					}
				}
				else
				{
					if (slider)
					{
						changed |= ::ImGui::SliderScalar("##slider", dataType, &value[axis], &min, &max, format, flags);
					}
					else
					{
						changed |= ::ImGui::DragScalar("##drag", dataType, &value[axis], speed, &min, &max, format, flags);
					}
				}
				::ImGui::PopID();
				::ImGui::PopItemWidth();
			}
			::ImGui::PopID();
			const Char* end = ::ImGui::FindRenderedTextEnd(label);
			if (end != label)
			{
				::ImGui::SameLine(0.0f, spacing);
				::ImGui::TextUnformatted(label, end);
			}
			::ImGui::EndGroup();
			return changed;
		}

		Bool DragFloat(const Char* label, Float* value, Float speed, Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 1, min, max, speed, false, format, flags);
		}

		Bool DragFloat2(const Char* label, Float value[2], Float speed, Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 2, min, max, speed, false, format, flags);
		}

		Bool DragFloat3(const Char* label, Float value[3], Float speed, Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 3, min, max, speed, false, format, flags);
		}

		Bool DragFloat4(const Char* label, Float value[4], Float speed, Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 4, min, max, speed, false, format, flags);
		}

		Bool SliderFloat(const Char* label, Float* value, Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 1, min, max, 0.0f, true, format, flags);
		}

		Bool SliderFloat2(const Char* label, Float value[2], Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 2, min, max, 0.0f, true, format, flags);
		}

		Bool SliderFloat3(const Char* label, Float value[3], Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 3, min, max, 0.0f, true, format, flags);
		}

		Bool SliderFloat4(const Char* label, Float value[4], Float min, Float max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 4, min, max, 0.0f, true, format, flags);
		}

		Bool DragInt(const Char* label, Int* value, Float speed, Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 1, min, max, speed, false, format, flags);
		}

		Bool DragInt2(const Char* label, Int value[2], Float speed, Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 2, min, max, speed, false, format, flags);
		}

		Bool DragInt3(const Char* label, Int value[3], Float speed, Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 3, min, max, speed, false, format, flags);
		}

		Bool DragInt4(const Char* label, Int value[4], Float speed, Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 4, min, max, speed, false, format, flags);
		}

		Bool SliderInt(const Char* label, Int* value, Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 1, min, max, 0.0f, true, format, flags);
		}

		Bool SliderInt2(const Char* label, Int value[2], Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 2, min, max, 0.0f, true, format, flags);
		}

		Bool SliderInt3(const Char* label, Int value[3], Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 3, min, max, 0.0f, true, format, flags);
		}

		Bool SliderInt4(const Char* label, Int value[4], Int min, Int max, const Char* format, ImGuiSliderFlags flags)
		{
			return Calclate(label, value, 4, min, max, 0.0f, true, format, flags);
		}

		Bool ColorEdit3(const Char* label, Float color[3], ImGuiColorEditFlags flags)
		{
			return SeedCore::ImGui::ColorEdit4(label, color, flags | ImGuiColorEditFlags_NoAlpha);
		}

		Bool ColorEdit4(const Char* label, Float color[4], ImGuiColorEditFlags flags)
		{
			ImGuiContext& context = *::ImGui::GetCurrentContext();
			const Bool editingText = context.ActiveId != 0 && context.ActiveId == context.InputTextState.ID;
			const Bool openExpression = ((::ImGui::GetIO().KeyCtrl && ::ImGui::IsMouseClicked(ImGuiMouseButton_Left)) || ::ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) && ::ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			if (!editingText && !openExpression)
			{
				return ::ImGui::ColorEdit4(label, color, flags);
			}
			const ImGuiColorEditFlags options = ::ImGui::GetIO().ConfigColorEditFlags;
			const ImGuiColorEditFlags display = (flags & ImGuiColorEditFlags_DisplayMask_) ? flags : options;
			if (::ImGui::GetCurrentWindow()->SkipItems || (flags & ImGuiColorEditFlags_NoInputs) || (display & ImGuiColorEditFlags_DisplayHex))
			{
				return ::ImGui::ColorEdit4(label, color, flags);
			}

			const Int channels = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
			const Bool preview = !(flags & ImGuiColorEditFlags_NoSmallPreview);
			const Bool previewLeft = preview && ::ImGui::GetStyle().ColorButtonPosition == ImGuiDir_Left;
			const Float spacing = ::ImGui::GetStyle().ItemInnerSpacing.x;
			const Float previewWidth = preview ? ::ImGui::GetFrameHeight() + spacing : 0.0f;
			const Float width = std::max(1.0f, ::ImGui::CalcItemWidth() - previewWidth);
			const ImVec2 origin = ::ImGui::GetCursorScreenPos();
			const ImVec2 start = ImVec2(origin.x + (previewLeft ? previewWidth : 0.0f), origin.y);
			const ImVec2 end = ImVec2(start.x + width, start.y + ::ImGui::GetFrameHeight());
			const ImGuiID colorID = ::ImGui::GetID(label);
			::ImGui::PushID(label);
			::ImGui::PushID("##channels");
			Bool editing = false;
			for (Int channel = 0; channel < channels; ++channel)
			{
				::ImGui::PushID(channel);
				editing |= ::ImGui::GetActiveID() == ::ImGui::GetID("##axis");
				::ImGui::PopID();
			}
			::ImGui::PopID();
			::ImGui::PopID();
			const Bool clicked = openExpression && ::ImGui::IsMouseHoveringRect(start, end);
			if (!editing && !clicked)
			{
				return ::ImGui::ColorEdit4(label, color, flags);
			}

			const Bool inputHSV = ((flags & ImGuiColorEditFlags_InputMask_ ? flags : options) & ImGuiColorEditFlags_InputHSV) != 0;
			const Bool displayHSV = (display & ImGuiColorEditFlags_DisplayHSV) != 0;
			const Bool useFloat = ((flags & ImGuiColorEditFlags_DataTypeMask_ ? flags : options) & ImGuiColorEditFlags_Float) != 0;
			Float shown[4] = {color[0], color[1], color[2], channels == 4 ? color[3] : 1.0f};
			if (inputHSV && !displayHSV)
			{
				::ImGui::ColorConvertHSVtoRGB(color[0], color[1], color[2], shown[0], shown[1], shown[2]);
			}
			else if (!inputHSV && displayHSV)
			{
				::ImGui::ColorConvertRGBtoHSV(color[0], color[1], color[2], shown[0], shown[1], shown[2]);
				if (context.ColorEditSavedID == colorID && context.ColorEditSavedColor == ::ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], 0.0f)))
				{
					if (shown[1] == 0.0f)
					{
						shown[0] = context.ColorEditSavedHue;
					}
					if (shown[2] == 0.0f)
					{
						shown[1] = context.ColorEditSavedSat;
					}
				}
			}

			const Float original[4] = {shown[0], shown[1], shown[2], shown[3]};
			const Bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
			const ImGuiSliderFlags bounds = hdr ? 0 : ImGuiSliderFlags_AlwaysClamp;
			::ImGui::BeginGroup();
			::ImGui::PushID(label);
			Bool changed = false;
			if (previewLeft)
			{
				changed |= ::ImGui::ColorEdit4("##preview", color, flags | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
				::ImGui::SameLine(0.0f, spacing);
			}
			::ImGui::SetNextItemWidth(width);
			if (useFloat)
			{
				Calclate("##channels", shown, channels, 0.0f, hdr ? 0.0f : 1.0f, 1.0f / 255.0f, false, "%.3f", bounds);
			}
			else
			{
				Int bytes[4]{};
				Int previous[4]{};
				for (Int channel = 0; channel < channels; ++channel)
				{
					const Double scaled = std::round(static_cast<Double>(shown[channel]) * 255.0);
					bytes[channel] = static_cast<Int>(std::clamp(scaled, static_cast<Double>((std::numeric_limits<Int>::lowest)()), static_cast<Double>((std::numeric_limits<Int>::max)())));
					previous[channel] = bytes[channel];
				}
				Calclate("##channels", bytes, channels, 0, hdr ? 0 : 255, 1.0f, false, "%d", bounds);
				for (Int channel = 0; channel < channels; ++channel)
				{
					if (bytes[channel] != previous[channel])
					{
						shown[channel] = static_cast<Float>(bytes[channel]) / 255.0f;
					}
				}
			}
			if (shown[0] != original[0] || shown[1] != original[1] || shown[2] != original[2])
			{
				if (inputHSV && !displayHSV)
				{
					::ImGui::ColorConvertRGBtoHSV(shown[0], shown[1], shown[2], color[0], color[1], color[2]);
				}
				else if (!inputHSV && displayHSV)
				{
					::ImGui::ColorConvertHSVtoRGB(shown[0], shown[1], shown[2], color[0], color[1], color[2]);
					context.ColorEditSavedID = colorID;
					context.ColorEditSavedHue = shown[0];
					context.ColorEditSavedSat = shown[1];
					context.ColorEditSavedColor = ::ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], 0.0f));
				}
				else
				{
					std::copy_n(shown, 3, color);
				}
				changed = true;
			}
			if (channels == 4 && shown[3] != original[3])
			{
				color[3] = shown[3];
				changed = true;
			}
			if (preview && !previewLeft)
			{
				::ImGui::SameLine(0.0f, spacing);
				changed |= ::ImGui::ColorEdit4("##preview", color, flags | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
			}
			::ImGui::PopID();
			const Char* labelEnd = ::ImGui::FindRenderedTextEnd(label);
			if (!(flags & ImGuiColorEditFlags_NoLabel) && labelEnd != label)
			{
				::ImGui::SameLine(0.0f, spacing);
				::ImGui::TextUnformatted(label, labelEnd);
			}
			::ImGui::EndGroup();
			return changed;
		}

		Bool SliderAngle(const Char* label, Float* radians, Float minDegrees, Float maxDegrees, const Char* format, ImGuiSliderFlags flags)
		{
			Float degrees = *radians * (180.0f / IM_PI);
			if (!Calclate(label, &degrees, 1, minDegrees, maxDegrees, 0.0f, true, format, flags))
			{
				return false;
			}
			*radians = degrees * (IM_PI / 180.0f);
			return true;
		}
	}
}
