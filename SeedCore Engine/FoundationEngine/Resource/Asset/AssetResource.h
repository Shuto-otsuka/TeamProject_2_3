#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	/**
	* [EN]
	* The part every resource manager shares: the table from asset ID to
	* the handle its loader owns, the queries on that table, and a
	* revision counter that moves whenever the table changes. Each
	* resource adds its typed Load and Resolve on top and overrides the
	* Asset entry points it needs (Unload, Clear, Generate, Bake), so the
	* public surface is the same for every asset type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのリソースマネージャに共通する部分。アセット ID からローダーが
	* 所有するハンドルへの対応表と、その表への問い合わせ、そして表が変わる
	* たびに進む Revision を持つ。各リソースはこの上に型付きの Load と
	* Resolve を足し、必要な Asset の入口（Unload・Clear・Generate・Bake）を
	* 上書きする。これにより public は全アセット種別で同じ形になる。
	*/
	template<class T>
	class AssetResource :public Asset, public NonCopyable
	{
	public:
		/**
		* [EN]
		* Returns the handle registered under assetID, or a null handle when
		* nothing is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で登録されているハンドルを返す。何も無ければ null の
		* ハンドルを返す。
		*/
		Handle<T> GetHandle(Uint32 assetID)const
		{
			if (!assetHandleMap_.contains(assetID))
			{
				return Handle<T>::null();
			}
			return assetHandleMap_.at(assetID);
		}

		/**
		* [EN]
		* Whether anything is registered under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で何かが登録されているかどうか。
		*/
		Bool Contains(Uint32 assetID)const
		{
			return assetHandleMap_.contains(assetID);
		}

		/**
		* [EN]
		* A counter that moves every time an asset is loaded, adopted or
		* unloaded, so a consumer that built something from this resource's
		* contents can tell when to build it again.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットが読み込まれる、Adopt される、解放されるたびに進むカウンタ。
		* このリソースの中身から何かを作った側が、作り直す時期を判断できる
		* ようにするため。
		*/
		Uint32 Revision()const
		{
			return revision_;
		}

		/**
		* [EN]
		* Registers a handle that already exists in the loader under
		* assetID, for an object that was not loaded from assetID's own
		* file. Whatever was registered under assetID before is only
		* forgotten, not released, so a caller replacing a loaded object
		* unloads it first.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーに既にあるハンドルを assetID で登録する。assetID 自身の
		* ファイルから読み込んだのではないオブジェクト向け。それまで assetID
		* で登録されていたものは忘れるだけで解放しないため、読み込み済みの
		* オブジェクトを置き換える側は先に Unload する。
		*/
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
		/// [EN] Asset ID to the handle the loader owns the loaded object under.
		/// [JP] アセット ID から、ローダーが読み込んだオブジェクトを所有しているハンドルへの対応。
		FlatMap<Uint32, Handle<T>> assetHandleMap_;

		/// [EN] Moves on every load, adoption and release; see Revision.
		/// [JP] 読み込み・Adopt・解放のたびに進む。Revision を参照。
		Uint32 revision_ = 0;
	};
}
