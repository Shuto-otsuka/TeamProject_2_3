#pragma once

namespace SeedCore
{
	class RenderResource
	{
	public:
		constexpr RenderResource() = default;

		consteval explicit RenderResource(const Char* name) :name_(name)
		{
			/// No Code
		}

		NODISCARD constexpr std::string_view Name()const
		{
			return name_;
		}

		constexpr Bool operator==(const RenderResource& other)const
		{
			return name_ == other.name_;
		}

		constexpr Bool operator!=(const RenderResource& other)const
		{
			return !(*this == other);
		}

	private:
		std::string_view name_;
	};
}