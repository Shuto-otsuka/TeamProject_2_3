#pragma once

namespace SeedCore
{
	class RenderFeatureName
	{
	public:
		consteval RenderFeatureName(const Char* name) :name_(name)
		{
			if (name_.empty())
			{
				throw "Feature の名前が空";
			}
			for (Size index = 0;index < name_.size();++index)
			{
				Char character = name_[index];
				Bool isUpper = character >= 'A' && character <= 'Z';
				Bool isLower = character >= 'a' && character <= 'z';
				Bool isDigit = character >= '0' && character <= '9';
				if (index == 0 && !isUpper)
				{
					throw "Feature の名前は大文字で始めること";
				}
				if (!isUpper && !isLower && !isDigit)
				{
					throw "Feature の名前は英数字だけにすること";
				}
			}
		}

		NODISCARD constexpr std::string_view Name()const
		{
			return name_;
		}

		constexpr Bool operator==(const RenderFeatureName& other)const
		{
			return name_ == other.name_;
		}

		constexpr Bool operator!=(const RenderFeatureName& other)const
		{
			return !(*this == other);
		}

	private:
		std::string_view name_;
	};

	class RenderStageName
	{
	public:
		consteval RenderStageName(const Char* name) :name_(name)
		{
			Size dot = name_.find('.');
			if (dot == std::string_view::npos || dot == 0 || dot + 1 == name_.size())
			{
				throw "Stage の名前は \"<Feature>.<動詞>\" の形にすること";
			}
			if (name_.find('.', dot + 1) != std::string_view::npos)
			{
				throw "Stage の名前のドットは1つだけにすること";
			}
			for (Size index = 0;index < name_.size();++index)
			{
				if (index == dot)
				{
					continue;
				}
				Char character = name_[index];
				Bool isUpper = character >= 'A' && character <= 'Z';
				Bool isLower = character >= 'a' && character <= 'z';
				Bool isDigit = character >= '0' && character <= '9';
				if ((index == 0 || index == dot + 1) && !isUpper)
				{
					throw "Stage の名前の Feature と動詞は大文字で始めること";
				}
				if (!isUpper && !isLower && !isDigit)
				{
					throw "Stage の名前は英数字とドット1つだけにすること";
				}
			}
		}

		NODISCARD constexpr std::string_view Name()const
		{
			return name_;
		}

		NODISCARD constexpr std::string_view FeatureName()const
		{
			return name_.substr(0, name_.find('.'));
		}

		constexpr Bool operator==(const RenderStageName& other)const
		{
			return name_ == other.name_;
		}

		constexpr Bool operator!=(const RenderStageName& other)const
		{
			return !(*this == other);
		}

	private:
		std::string_view name_;
	};
}