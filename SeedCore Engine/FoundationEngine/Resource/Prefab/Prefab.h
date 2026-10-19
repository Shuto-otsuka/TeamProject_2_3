#pragma once
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	class PrefabSystem;

	/**
	* [EN]
	* Gameplay-facing prefab API: what game code uses to spawn prefabs at
	* runtime without threading a World/ResourceCache reference through
	* itself. Forwards to the PrefabSystem the host owns, bound when that
	* system is constructed. Saving prefabs lives on PrefabSystem, and a
	* prefab's contents on PrefabBlueprint.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームプレイ向けのプレハブAPI。ゲームのコードが World/ResourceCache
	* 参照を自前で引き回さずに、ランタイムでプレハブを生成するのに使う。
	* ホストが所有する PrefabSystem へ転送する。転送先は、その
	* PrefabSystem の構築時に束縛される。プレハブの保存は PrefabSystem、
	* プレハブの中身は PrefabBlueprint にある。
	*/
	class SEEDCORE_API Prefab
	{
	public:
		/**
		* [EN]
		* Loads the .prefab asset identified by assetID and instantiates it
		* in the bound World, parented under parent when given. Returns the
		* new subtree's root Actor, or nullptr if no PrefabSystem is bound,
		* the asset could not be loaded, or nothing was instantiated. The
		* new root is stamped with assetID as its source prefab, so
		* inspector "apply to prefab" edits target the right asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で識別される .prefab アセットを読み込み、束縛済みの World
		* 内、parent が指定されていればその下へインスタンス化する。新しい
		* サブツリーのルート Actor を返す。PrefabSystem が束縛されていない、
		* アセットを読み込めなかった、または何もインスタンス化されなかった
		* 場合は nullptr を返す。新しいルートには assetID が元プレハブとして
		* 刻印されるため、インスペクタの「プレハブに適用」編集が正しい
		* アセットを対象にできる。
		*/
		static Actor Spawn(Uint32 assetID, Actor parent = {});

		/**
		* [EN]
		* Resolves name to an asset ID, then spawns it like
		* Spawn(Uint32, Actor). name must include the ".prefab" extension;
		* it is matched against asset filenames (a bare filename is enough
		* -- the subfolder does not matter) and is not stem-matched, so
		* "Enemy" will not find "Enemy.prefab". Pass a string literal so
		* Tools/Python/RuntimePackager.py can statically detect the
		* reference and bundle the asset into a package build. Logs a
		* warning and returns nullptr if name does not resolve.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* name をアセット ID へ解決し、Spawn(Uint32, Actor) と同じように
		* 生成する。name は ".prefab" 拡張子を含める必要がある。アセットの
		* ファイル名と照合され（サブフォルダは問わず、ファイル名だけで
		* 良い）、stem 一致はしないため "Enemy" では "Enemy.prefab" は
		* 見つからない。Tools/Python/RuntimePackager.py が参照を静的に
		* 検出してパッケージビルドへアセットを同梱できるよう、文字列
		* リテラルを渡すこと。name が解決できなければ警告をログ出力して
		* nullptr を返す。
		*/
		static Actor Spawn(const String& name, Actor parent = {});

	private:
		/// [EN] PrefabSystem binds itself as system_ when constructed and unbinds when destroyed.
		/// [JP] PrefabSystem は構築時に自身を system_ として束縛し、破棄時に外す。
		friend class PrefabSystem;

	private:
		/// [EN] PrefabSystem every call forwards to; owned by the host, null while none is bound.
		/// [JP] 全ての呼び出しの転送先となる PrefabSystem。ホストが所有し、束縛されていない間は null。
		static PrefabSystem* system_;
	};
}
