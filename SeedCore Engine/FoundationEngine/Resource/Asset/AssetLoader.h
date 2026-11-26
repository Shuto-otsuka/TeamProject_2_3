#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	template<class T>
	class AssetLoader :public NonCopyable
	{
	public:
		T* Get(const Handle<T>& handle)
		{
			return pool_.Get(handle);
		}

		Handle<T> Adopt(T&& value)
		{
			Handle<T> handle = pool_.Create();
			T* slot = pool_.Get(handle);
			if (!slot)
			{
				return Handle<T>::null();
			}
			*slot = std::move(value);
			return handle;
		}

	protected:
		StablePool<T> pool_;
	};
}
