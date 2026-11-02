#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	/**
	* [EN]
	* The part every format-specific loader shares: the handle pool that
	* owns the loaded objects, and the two operations that only touch that
	* pool. Each loader adds its own Load and Clear on top, with the same
	* shape for every asset type, so the public surface of every loader is
	* Load / Get / Clear / Adopt and nothing else. A loader's format
	* parsing and its writing of cache files stay private, reached only by
	* its paired resource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのフォーマット固有ローダーに共通する部分。読み込んだオブジェクトを
	* 所有するハンドルのプールと、そのプールにしか触れない2つの操作を持つ。
	* 各ローダーはこの上に、全アセット種別で同じ形の Load と Clear を足す。
	* これにより、どのローダーも public は Load / Get / Clear / Adopt だけに
	* なる。フォーマットの解析とキャッシュファイルの書き出しは private に
	* 置き、対になるリソースからだけ使う。
	*/
	template<class T>
	class AssetLoader :public NonCopyable
	{
	public:
		/**
		* [EN]
		* Returns the object handle names, or nullptr when the handle is
		* stale or empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示すオブジェクトを返す。handle が古いか空なら nullptr を
		* 返す。
		*/
		T* Get(const Handle<T>& handle)
		{
			return pool_.Get(handle);
		}

		/**
		* [EN]
		* Moves an object that was already built elsewhere into the pool and
		* returns the handle it is now owned under. This is how an object
		* that never came from a file of its own (such as a texture baked
		* into a material) joins the pool. Only instantiated for an asset
		* type that is actually adopted, so a type that cannot be moved
		* still compiles.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 別の場所で組み立て済みのオブジェクトをプールへ移し、以後それを所有
		* するハンドルを返す。自分のファイルを持たないオブジェクト（マテリアル
		* に焼き込まれたテクスチャなど）がプールに加わる手段。実際に Adopt
		* されるアセット種別でだけ実体化されるため、ムーブできない型でも
		* コンパイルが通る。
		*/
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
		/// [EN] Owns every object this loader has loaded or adopted, handing out versioned handles to them.
		/// [JP] このローダーが読み込んだ、または Adopt した全オブジェクトを所有し、それらへのバージョン付きハンドルを発行する。
		StablePool<T> pool_;
	};
}
