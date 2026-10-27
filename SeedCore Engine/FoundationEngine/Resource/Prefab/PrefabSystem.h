#pragma once
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/Utility/NonTransferable.h>

namespace SeedCore
{
	class World;
	class ResourceCache;

	/**
	* [EN]
	* Host-side prefab API, owned by the host that runs the main loop.
	* Spawns prefabs into the World given to Initialize, and saves actor
	* subtrees as .prefab files for the editor. Gameplay code reaches the
	* spawning through Prefab, which forwards to the PrefabSystem bound by
	* Initialize.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ホスト向けのプレハブAPI。メインループを回すホストが所有する。
	* Initialize で渡された World へプレハブを生成し、エディタ向けに actor の
	* サブツリーを .prefab ファイルとして保存する。ゲームプレイコードは
	* Prefab 経由で生成に届く。Prefab は、Initialize で束縛された
	* PrefabSystem へ転送する。
	*/
	class SEEDCORE_API PrefabSystem :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Constructs an unbound system; Initialize binds it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 未束縛のシステムを作る。束縛は Initialize が行う。
		*/
		PrefabSystem() = default;

		/**
		* [EN]
		* Unbinds this system from Prefab if it is the bound one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このシステムが Prefab に束縛されていれば束縛を外す。
		*/
		~PrefabSystem();

		/**
		* [EN]
		* Binds the World/ResourceCache prefabs are spawned into and
		* loaded from, and binds this system as the one Prefab forwards to.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プレハブの生成先となる World と読み込み元となる ResourceCache を
		* 束縛し、このシステムを Prefab の転送先として束縛する。
		*/
		Bool Initialize(World& world, ResourceCache& cache);

	public:
		/**
		* [EN]
		* Loads the .prefab asset identified by assetID (via the bound
		* ResourceCache's PrefabPool) and instantiates it in the bound
		* World, parented under parent when given. Returns the new
		* subtree's root Actor, or nullptr if the asset could not be
		* loaded or nothing was instantiated. The new root is stamped with
		* assetID as its source prefab, so inspector "apply to prefab"
		* edits target the right asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で識別される .prefab アセットを（束縛済みの
		* ResourceCache の PrefabPool 経由で）読み込み、束縛済みの World
		* 内、parent が指定されていればその下へインスタンス化する。新しい
		* サブツリーのルート Actor を返す。アセットを読み込めなかった、
		* または何もインスタンス化されなかった場合は nullptr を返す。新しい
		* ルートには assetID が元プレハブとして刻印されるため、インスペクタの
		* 「プレハブに適用」編集が正しいアセットを対象にできる。
		*/
		Actor Spawn(Uint32 assetID, Actor parent = {});

		/**
		* [EN]
		* Resolves name to an asset ID via the bound ResourceCache, then
		* delegates to Spawn(Uint32, Actor). name must include the
		* ".prefab" extension; it is matched against asset filenames (a
		* bare filename is enough -- the subfolder does not matter) and is
		* not stem-matched. Logs a warning and returns nullptr if name
		* does not resolve.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 束縛済みの ResourceCache 経由で name をアセット ID へ解決し、
		* Spawn(Uint32, Actor) へ委譲する。name は ".prefab" 拡張子を
		* 含める必要がある。アセットのファイル名と照合され（サブフォルダ
		* は問わず、ファイル名だけで良い）、stem 一致はしない。name が
		* 解決できなければ警告をログ出力して nullptr を返す。
		*/
		Actor Spawn(const String& name, Actor parent = {});

	public:
		/**
		* [EN]
		* Captures root into a temporary PrefabBlueprint and writes it to
		* path, creating parent directories as needed. Returns whether
		* saving succeeded. Needs nothing bound, so it is static.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* root を一時的な PrefabBlueprint へ取得し、path へ書き込む（必要なら
		* 親ディレクトリを作成する）。保存に成功したかどうかを返す。何も
		* 束縛を必要としないため static。
		*/
		static Bool Save(Actor root, const std::filesystem::path& path);

		/**
		* [EN]
		* Captures root and saves it into directory under a name derived
		* from root's Name component (or "Actor" if unnamed), appending a
		* numeric suffix to avoid overwriting an existing file. Returns
		* the path actually written to, or an empty path on failure.
		* Needs nothing bound, so it is static.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* root を取得し、その Name コンポーネントから導出された名前
		* （名前が無ければ "Actor"）で directory 内へ保存する。既存
		* ファイルの上書きを避けるため、数値の接尾辞を付加する。実際に
		* 書き込まれたパスを返す。失敗時は空のパスを返す。何も束縛を
		* 必要としないため static。
		*/
		static std::filesystem::path SaveToDirectory(Actor root, const std::filesystem::path& directory);

	private:
		/// [EN] World prefabs are spawned into.
		/// [JP] プレハブの生成先となる World。
		World* world_ = nullptr;

		/// [EN] ResourceCache used to resolve prefab names/IDs and load prefab assets.
		/// [JP] プレハブ名/ID の解決とプレハブアセットの読み込みに使う ResourceCache。
		ResourceCache* resource_ = nullptr;
	};
}
