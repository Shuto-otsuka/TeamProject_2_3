#pragma once
#include <FoundationEngine/Utility/Types.h>

#include <string>
#include <memory_resource>
#include <unordered_set>

namespace SeedCore
{
	class SEEDCORE_API InternPool
	{
	public:
		static InternPool& Singleton();

	public:
		struct TransparentHash
		{
			using is_transparent = void;

			Size operator()(std::u8string_view view)const noexcept;

			Size operator()(const std::pmr::u8string& string)const noexcept;
		};

		struct TransparentEqual
		{
			using is_transparent = void;

			Bool operator()(std::u8string_view a, std::u8string_view b)const noexcept;

			Bool operator()(const std::pmr::u8string& a, std::u8string_view b)const noexcept;

			Bool operator()(std::u8string_view a, const std::pmr::u8string& b)const noexcept;

			Bool operator()(const std::pmr::u8string& a, const std::pmr::u8string& b)const noexcept;
		};

	public:
		InternPool();

		template<typename T>
		T Intern(std::u8string_view view)
		{
			if (view.empty())
			{
				return T();
			}

			std::lock_guard<std::mutex> lock(mutex_);

			auto it = strings_.find(view);
			if (it != strings_.end())
			{
				return T(it->c_str(), it->size());
			}
			auto result = strings_.emplace(std::pmr::u8string(view, &arena_));

			return T(result.first->c_str(), result.first->size());
		}

	private:
		std::mutex mutex_;

		std::pmr::monotonic_buffer_resource arena_;

		std::pmr::unordered_set<std::pmr::u8string, TransparentHash, TransparentEqual> strings_;
	};

	SEEDCORE_API std::wstring ConvertToWideString(std::string_view view);

	SEEDCORE_API std::string ConvertToCharString(std::wstring_view view);
}