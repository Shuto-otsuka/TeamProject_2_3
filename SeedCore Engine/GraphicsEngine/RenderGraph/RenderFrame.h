#pragma once
#include <GraphicsEngine/RenderGraph/RenderScene.h>

namespace SeedCore
{
	class RenderFrame
	{
	public:
		template<class Type>
		void Write(const Type& value)
		{
			SC_STATIC_ASSERT(std::is_trivially_copyable_v<Type>, "RenderFrame に書く型はコピーだけで移せる型にすること");
			const void* key = RenderTypeKey::Of<Type>();
			for (Entry& entry : entries_)
			{
				if (entry.key_ == key)
				{
					std::memcpy(storage_.data() + entry.offset_, &value, sizeof(Type));
					return;
				}
			}
			Size offset = storage_.size();
			storage_.resize(offset + (sizeof(Type) + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
			std::memcpy(storage_.data() + offset, &value, sizeof(Type));
			entries_.push_back(Entry{ key, offset });
		}

		template<class Type>
		NODISCARD const Type* Find()const
		{
			const void* key = RenderTypeKey::Of<Type>();
			for (const Entry& entry : entries_)
			{
				if (entry.key_ == key)
				{
					return reinterpret_cast<const Type*>(storage_.data() + entry.offset_);
				}
			}
			return nullptr;
		}

	private:
		struct Entry
		{
			const void* key_ = nullptr;

			Size offset_ = 0;
		};

	private:
		DynamicArray<Entry> entries_;

		DynamicArray<std::max_align_t> storage_;
	};
}
