#include <Windows.h>

#include <FoundationEngine/Pool/InternPool.h>

namespace SeedCore
{
	Size InternPool::TransparentHash::operator()(std::u8string_view view)const noexcept
	{
		return std::hash<std::u8string_view>{}(view);
	}

	Size InternPool::TransparentHash::operator()(const std::pmr::u8string& string)const noexcept
	{
		return std::hash<std::u8string_view>{}(std::u8string_view{ string });
	}
}

namespace SeedCore
{
	Bool InternPool::TransparentEqual::operator()(std::u8string_view a, std::u8string_view b)const noexcept
	{
		return a == b;
	}

	Bool InternPool::TransparentEqual::operator()(const std::pmr::u8string& a, std::u8string_view b)const noexcept
	{
		return a == b;
	}

	Bool InternPool::TransparentEqual::operator()(std::u8string_view a, const std::pmr::u8string& b)const noexcept
	{
		return a == b;
	}

	Bool InternPool::TransparentEqual::operator()(const std::pmr::u8string& a, const std::pmr::u8string& b)const noexcept
	{
		return a == b;
	}
}

namespace SeedCore
{
	InternPool& InternPool::Singleton()
	{
		static InternPool instance;
		return instance;
	}

	InternPool::InternPool() : arena_(std::pmr::get_default_resource()), strings_(&arena_)
	{
		/// No Code
	}
}

namespace SeedCore
{
	std::wstring ConvertToWideString(std::string_view view)
	{
		if (view.empty())
		{
			return std::wstring();
		}

		Size length = MultiByteToWideChar(CP_UTF8, 0, view.data(), (Int)view.size(), nullptr, 0);
		if (length == static_cast<Size>(-1))
		{
			return std::wstring();
		}

		std::wstring wstring(static_cast<Int>(length), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, view.data(), (Int)view.size(), wstring.data(), static_cast<Int>(length));
		return wstring;
	}

	std::string ConvertToCharString(std::wstring_view view)
	{
		if (view.empty())
		{
			return std::string();
		}

		Size length = WideCharToMultiByte(CP_UTF8, 0, view.data(), static_cast<Int>(view.size()), nullptr, 0, nullptr, nullptr);
		if (length == static_cast<Size>(-1))
		{
			return std::string();
		}

		std::string string(static_cast<Int>(length), '\0');
		WideCharToMultiByte(CP_UTF8, 0, view.data(), static_cast<Int>(view.size()), string.data(), static_cast<Int>(length), nullptr, nullptr);
		return string;
	}
}