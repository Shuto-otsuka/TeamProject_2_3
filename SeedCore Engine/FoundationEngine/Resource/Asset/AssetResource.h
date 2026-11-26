#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	template<class T>
	class AssetResource :public Asset, public NonCopyable
	{
	public:
		Handle<T> GetHandle(Uint32 assetID)const
		{
			if (!assetHandleMap_.contains(assetID))
			{
				return Handle<T>::null();
			}
			return assetHandleMap_.at(assetID);
		}

		Bool Contains(Uint32 assetID)const
		{
			return assetHandleMap_.contains(assetID);
		}

		Uint32 Revision()const
		{
			return revision_;
		}

		void Adopt(Uint32 assetID, const Handle<T>& handle)
		{
			if (handle.empty())
			{
				return;
			}
			if (assetHandleMap_.contains(assetID))
			{
				assetHandleMap_.erase(assetID);
			}
			assetHandleMap_.insert({ assetID, handle });
			revision_++;
		}

	protected:
		FlatMap<Uint32, Handle<T>> assetHandleMap_;

		Uint32 revision_ = 0;
	};
}
