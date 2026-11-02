#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerEffect.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads Effekseer effects from a ".efkefc" source or from a ".effekseer"
	* cache, which bundles the source with every file the effect reads
	* (images, waves, models, materials, curves) so it plays without them.
	* Writing the cache is private and reached only by EffekseerResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Effekseer のエフェクトを、".efkefc" ソースか ".effekseer" キャッシュ
	* から読み込む。キャッシュはソースと、エフェクトが読む全ファイル（画像、
	* ウェーブ、モデル、マテリアル、カーブ）をまとめたもので、それらが
	* 無くても再生できる。キャッシュの書き出しは private で、
	* EffekseerResource からだけ使う。
	*/
	class EffekseerLoader :public AssetLoader<EffekseerEffect>
	{
	private:
		friend class EffekseerResource;

	public:
		EffekseerLoader() = default;
		~EffekseerLoader() = default;

		/**
		* [EN]
		* Creates an effect from the ".effekseer" cache or ".efkefc" source
		* at filePath and returns its handle, or a null handle when it cannot
		* be read or created. A cache registers its bundled files with the
		* manager's file interface first. Nothing is written. thread is
		* unused: the effect needs no per-thread state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の ".effekseer" キャッシュか ".efkefc" ソースからエフェクトを
		* 作り、そのハンドルを返す。読めない、または作れない場合は null の
		* ハンドルを返す。キャッシュは、同梱したファイルを先にマネージャの
		* ファイルインターフェースへ登録する。何も書き出さない。thread は
		* 使わない（エフェクトはスレッドごとの状態を持たないため）。
		*/
		Handle<EffekseerEffect> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Destroys the effect handle names.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示すエフェクトを破棄する。
		*/
		void Clear(Handle<EffekseerEffect>& handle)noexcept;

	private:
		/**
		* [EN]
		* Creates the effect from the ".efkefc" source at sourcePath to learn
		* which files it reads, then writes the source and every one of
		* those files into the ".effekseer" cache at cachePath. Returns false
		* when the source cannot be read or created, or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* sourcePath の ".efkefc" ソースからエフェクトを作って、それが読む
		* ファイルを調べ、ソースとそれらの全ファイルを cachePath の
		* ".effekseer" キャッシュへ書く。ソースを読めない、作れない、または
		* 書き込みに失敗した場合は false を返す。
		*/
		Bool Bake(String sourcePath, String cachePath);
	};
}
