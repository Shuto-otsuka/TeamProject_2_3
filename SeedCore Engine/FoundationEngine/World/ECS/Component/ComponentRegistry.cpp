#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/Log/Assert.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	ComponentID ComponentRegistry::Register(String name, String category, const ComponentMetadata* metadata, const ComponentHooks* hooks)
	{
		if (name == String())
		{
			SC_LOG_ERROR("名前のないコンポーネントは登録できません。");
			return nullptr;
		}

		if (ResolveModule(metadata) != ResolveModule(reinterpret_cast<const void*>(&ComponentRegistry::ResolveModule)) && category != String("Custom") && !category.str().starts_with("Custom/"))
		{
			category = String("Custom/" + category.str());
		}

		if (hooks && hooks->upcast_ && metadata && metadata->storage_ == ComponentStorage::Archetype)
		{
			SC_LOG_ERROR("コンポーネント「{}」は ComponentBehaviour なので Archetype に格納できません。", name.str());
			return nullptr;
		}

		ComponentRecord* record = ResolveRecord(name);

		if (!record)
		{
			Uint32 index = 0;
			for (const ResourcePtr<StaticArray<ComponentRecord, 64>>& page : pages_)
			{
				for (ComponentRecord& slot : *page)
				{
					if (slot.name_ == String())
					{
						record = &slot;
						break;
					}
					++index;
				}

				if (record)
				{
					break;
				}
			}

			if (!record)
			{
				pages_.push_back(MakePtr<StaticArray<ComponentRecord, 64>>());
				record = &(*pages_.back())[0];
			}

			record->name_ = name;
			record->index_ = index;
		}

		if (record->metadata_ == metadata && record->hooks_ == hooks)
		{
			record->category_ = category;
			return record;
		}

		if (record->metadata_)
		{
			void* owner = ResolveModule(record->metadata_);
			if (owner)
			{
				if (owner != ResolveModule(metadata))
				{
					return record;
				}

				SC_LOG_ERROR("コンポーネント「{}」は別の型として既に登録されているため、登録しませんでした。", name.str());
				return nullptr;
			}
		}
		else
		{
			records_.push_back(record);
		}

		record->category_ = category;
		record->metadata_ = metadata;
		record->hooks_ = hooks;
		return record;
	}

	void ComponentRegistry::Unregister(ComponentID id)
	{
		if (!id || !id->metadata_)
		{
			return;
		}

		ComponentRecord* record = ResolveRecord(id->name_);
		record->metadata_ = nullptr;
		record->hooks_ = nullptr;
		++record->epoch_;

		SeedCore::erase_if(records_, [id](ComponentID registered) { return registered == id; });
	}

	ComponentID ComponentRegistry::GetComponentID(String name)
	{
		ComponentRecord* record = ResolveRecord(name);
		if (!record || !record->metadata_)
		{
			return nullptr;
		}
		return record;
	}

	String ComponentRegistry::Name(ComponentID id)
	{
		if (!id)
		{
			return String();
		}
		return id->name_;
	}

	std::span<const ComponentID> ComponentRegistry::Registry()
	{
		return records_;
	}

	const ComponentMetadata& ComponentRegistry::Get(ComponentID id)
	{
#if SC_DEBUG
		SC_ASSERT(id && id->metadata_);
#endif
		return *id->metadata_;
	}

	Size ComponentRegistry::GetID(ComponentID id)
	{
		return id->index_;
	}

	Size ComponentRegistry::ComponentSize(ComponentID id)
	{
		return Get(id).size_;
	}

	Size ComponentRegistry::ComponentAlignment(ComponentID id)
	{
		return Get(id).alignment_;
	}

	ComponentRecord* ComponentRegistry::ResolveRecord(String name)
	{
		for (const ResourcePtr<StaticArray<ComponentRecord, 64>>& page : pages_)
		{
			for (ComponentRecord& record : *page)
			{
				if (record.name_ == String())
				{
					return nullptr;
				}

				if (record.name_ == name)
				{
					return &record;
				}
			}
		}
		return nullptr;
	}

	void* ComponentRegistry::ResolveModule(const void* address)
	{
		HMODULE module = nullptr;
		if (address)
		{
			GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(address), &module);
		}
		return module;
	}
}
