#pragma once

namespace SeedCore
{
	template<Size Length>
	struct RenderLiteral
	{
		consteval RenderLiteral(const Char(&text)[Length])
		{
			for (Size index = 0;index < Length;++index)
			{
				text_[index] = text[index];
			}
		}

		NODISCARD constexpr std::string_view View()const
		{
			return std::string_view(text_, Length - 1);
		}

		Char text_[Length] = {};
	};

	template<Size Length>
	struct RenderNameStorage
	{
		consteval RenderNameStorage(std::string_view first, std::string_view second)
		{
			Size cursor = 0;
			for (Char character : first)
			{
				text_[cursor] = character;
				++cursor;
			}
			text_[cursor] = '.';
			++cursor;
			for (Char character : second)
			{
				text_[cursor] = character;
				++cursor;
			}
		}

		NODISCARD constexpr std::string_view View()const
		{
			return std::string_view(text_, Length);
		}

		Char text_[Length + 1] = {};
	};

	class RenderName
	{
	public:
		template<class Type>
		static consteval std::string_view TypeName()
		{
#if defined(__clang__)
			std::string_view signature = __PRETTY_FUNCTION__;
			Size begin = signature.find("[Type = ") + 8;
			Size end = signature.find_first_of("];", begin);
#else
			std::string_view signature = __FUNCSIG__;
			Size begin = signature.find("TypeName<") + 9;
			Size end = signature.rfind(">(void)");
#endif
			std::string_view type = signature.substr(begin, end - begin);
			Size last = type.find_last_of(": ");
			return last == std::string_view::npos ? type : type.substr(last + 1);
		}

		template<class Feature>
		static consteval std::string_view FeatureName()
		{
			return Trim(TypeName<Feature>(), "Feature");
		}

		template<class Stage>
		static consteval std::string_view StageName()
		{
			return Trim(TypeName<Stage>(), "Stage");
		}

	private:
		static consteval std::string_view Trim(std::string_view name, std::string_view suffix)
		{
			if (!name.ends_with(suffix) || name.size() == suffix.size())
			{
				throw "Feature のクラス名は <名前>Feature、Stage の struct 名は <名前>Stage にすること";
			}
			name.remove_suffix(suffix.size());
			for (Size index = 0;index < name.size();++index)
			{
				Char character = name[index];
				Bool isUpper = character >= 'A' && character <= 'Z';
				Bool isLower = character >= 'a' && character <= 'z';
				Bool isDigit = character >= '0' && character <= '9';
				if (index == 0 && !isUpper)
				{
					throw "Feature と Stage の名前は大文字で始めること";
				}
				if (!isUpper && !isLower && !isDigit)
				{
					throw "Feature と Stage の名前は英数字だけにすること";
				}
			}
			return name;
		}
	};

	template<class Feature, RenderLiteral Member>
	inline constexpr RenderNameStorage<RenderName::FeatureName<Feature>().size() + 1 + Member.View().size()> RenderResourceName = RenderNameStorage<RenderName::FeatureName<Feature>().size() + 1 + Member.View().size()>(RenderName::FeatureName<Feature>(), Member.View());

	template<class Feature, class Stage>
	inline constexpr RenderNameStorage<RenderName::FeatureName<Feature>().size() + 1 + RenderName::StageName<Stage>().size()> RenderStageName = RenderNameStorage<RenderName::FeatureName<Feature>().size() + 1 + RenderName::StageName<Stage>().size()>(RenderName::FeatureName<Feature>(), RenderName::StageName<Stage>());
}
