#include <FoundationEngine/Utility/String.h>
#include <FoundationEngine/Pool/InternPool.h>

namespace SeedCore
{
	String String::intern(std::string_view view)
	{
		return intern(std::u8string_view(reinterpret_cast<const Char8*>(view.data()), view.size()));
	}

	String String::intern(std::wstring_view view)
	{
		std::string string = ConvertToCharString(view);
		return intern(std::string_view(string));
	}

	String String::intern(const Char8* string)
	{
		return intern(std::u8string_view(reinterpret_cast<const Char8*>(string)));
	}

	String String::intern(std::u8string_view view)
	{
		return InternPool::Singleton().Intern<String>(view);
	}

	String::String(const Char* text) :String(std::string_view(text))
	{
		/// No Code
	}

	String::String(std::u8string_view view)
	{
		*this = intern(view);
	}

	String::String(std::string_view view)
	{
		*this = intern(std::u8string_view(reinterpret_cast<const Char8*>(view.data()), view.size()));
	}

	String::String(std::wstring_view view)
	{
		*this = intern(view);
	}

	String::String(const Char8* view, Size size) :view_(view, size)
	{
		/// No Code
	}

	const Char* String::c_str()const
	{
		return reinterpret_cast<const Char*>(view_.data());
	}

	std::string String::str()const
	{
		return std::string(c_str(), view_.size());
	}

	std::wstring String::w_str()const
	{
		return ConvertToWideString(this->str());
	}

	std::u8string String::u8str()const
	{
		return std::u8string(view_.data(), view_.size());
	}

	String::operator std::string()const
	{
		return std::string(reinterpret_cast<const Char*>(view_.data()), view_.size());
	}

	String::operator std::u8string()const
	{
		return std::u8string(view_.data(), view_.size());
	}

	String::operator std::string_view()const
	{
		return std::string_view(reinterpret_cast<const Char*>(view_.data()), view_.size());
	}

	String::operator std::u8string_view()const
	{
		return std::u8string_view(view_.data(), view_.size());
	}
}