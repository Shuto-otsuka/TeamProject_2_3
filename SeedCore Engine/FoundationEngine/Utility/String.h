#pragma once

namespace SeedCore
{
	class SEEDCORE_API String
	{
	public:
		static String intern(std::string_view view);

		static String intern(std::wstring_view view);

		static String intern(const Char8* string);

		static String intern(std::u8string_view view);

		String() = default;

		String(const Char* text);

		String(std::u8string_view view);

		String(std::string_view view);

		String(std::wstring_view view);

		constexpr std::u8string_view view()const noexcept
		{
			return view_;
		}

		const Char* c_str()const;

		std::string str()const;

		std::wstring w_str()const;

		std::u8string u8str()const;

		friend Bool operator==(String first, String second)noexcept
		{
			return first.view_.data() == second.view_.data();
		}

		friend Bool operator!=(String first, String second)noexcept
		{
			return !(first == second);
		}

		explicit operator std::string()const;

		explicit operator std::u8string()const;

		explicit operator std::string_view()const;

		explicit operator std::u8string_view()const;

	private:
		std::u8string_view view_;

		explicit String(const Char8* view, Size size);

		friend class InternPool;
	};
}

namespace std
{
	template<>
	struct hash<SeedCore::String>
	{
		size_t operator()(const SeedCore::String& string)const noexcept
		{
			return std::hash<const void*>{}(string.view().data());
		}
	};
}