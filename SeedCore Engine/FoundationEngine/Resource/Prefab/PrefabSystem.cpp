#include <FoundationEngine/Resource/Prefab/PrefabSystem.h>
#include <FoundationEngine/Resource/Prefab/Prefab.h>
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Resource/Prefab/PrefabPool.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/Name.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the World/ResourceCache prefabs are spawned into and loaded
	* from, and binds this system as the one Prefab forwards to.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プレハブの生成先となる World と読み込み元となる ResourceCache を
	* 束縛し、このシステムを Prefab の転送先として束縛する。
	*/
	Bool PrefabSystem::Initialize(World& world, ResourceCache& cache)
	{
		world_ = &world;
		resource_ = &cache;
		Prefab::system_ = this;
		return true;
	}

	/**
	* [EN]
	* Unbinds this system from Prefab if it is the bound one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このシステムが Prefab に束縛されていれば束縛を外す。
	*/
	PrefabSystem::~PrefabSystem()
	{
		if (Prefab::system_ == this)
		{
			Prefab::system_ = nullptr;
		}
	}

	/**
	* [EN]
	* Loads the .prefab asset identified by assetID and instantiates it
	* in the bound World under parent (when given). Returns the new
	* subtree's root Actor, or nullptr if the asset could not be loaded
	* or nothing was instantiated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で識別される .prefab アセットを読み込み、束縛済みの World
	* 内、parent が指定されていればその下へインスタンス化する。新しい
	* サブツリーのルート Actor を返す。アセットを読み込めなかった、
	* または何もインスタンス化されなかった場合は nullptr を返す。
	*/
	Actor PrefabSystem::Spawn(Uint32 assetID, Actor parent)
	{
		/// [EN] PrefabPool caches by asset ID, so repeatedly spawning the same prefab reloads nothing - each call pays only the per-instance Instantiate() cost.
		/// [JP] PrefabPool はアセット ID でキャッシュするため、同じプレハブを繰り返し spawn しても再読み込みは発生しない - 毎回かかるのはインスタンスごとの Instantiate() のコストだけ。
		PrefabPool& pool = resource_->GetPrefabPool();
		PrefabBlueprint* prefab = pool.Get(pool.Load(assetID, *resource_));
		if (prefab == nullptr)
		{
			return Actor();
		}

		/// [EN] Pass assetID as sourceAssetID so the new root is registered as an instance of this prefab (inspector "apply to prefab" then targets the right asset).
		/// [JP] assetID を sourceAssetID として渡し、新しいルートをこのプレハブのインスタンスとして登録する（インスペクタの「プレハブに適用」が正しいアセットを対象にする）。
		return prefab->Instantiate(*world_, *resource_, parent, assetID);
	}

	/**
	* [EN]
	* Resolves name (which must carry the ".prefab" extension) to an asset
	* ID via the bound ResourceCache, then delegates to
	* Spawn(Uint32, Actor). Logs a warning and returns nullptr if name
	* does not resolve.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name（".prefab" 拡張子を含むこと）を、束縛済みの ResourceCache
	* 経由でアセット ID へ解決し、Spawn(Uint32, Actor) へ委譲する。
	* name が解決できなければ警告をログ出力して nullptr を返す。
	*/
	Actor PrefabSystem::Spawn(const String& name, Actor parent)
	{
		Uint32 assetID = resource_->GetAssetID(name);
		if (assetID == 0)
		{
			SC_LOG_WARNING("Prefab::Spawn: プレハブ \"{}\" が見つかりません。", name.c_str());
			return Actor();
		}

		return Spawn(assetID, parent);
	}

	/**
	* [EN]
	* Captures root into a temporary PrefabBlueprint and writes it to path,
	* creating parent directories as needed. Returns whether saving succeeded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* root を一時的な PrefabBlueprint へ取得し、path へ書き込む（必要なら
	* 親ディレクトリを作成する）。保存に成功したかどうかを返す。
	*/
	Bool PrefabSystem::Save(Actor root, const std::filesystem::path& path)
	{
		PrefabBlueprint prefab;
		prefab.Capture(root);

		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		return prefab.Write(path);
	}

	/**
	* [EN]
	* Captures root and saves it into directory under a name derived from
	* root's Name component (or "Actor" if unnamed), appending a numeric
	* suffix to avoid overwriting an existing file. Returns the path
	* actually written to, or an empty path on failure.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* root を取得し、その Name コンポーネントから導出された名前（名前が
	* 無ければ "Actor"）で directory 内へ保存する。既存ファイルの上書きを
	* 避けるため、数値の接尾辞を付加する。実際に書き込まれたパスを返す。
	* 失敗時は空のパスを返す。
	*/
	std::filesystem::path PrefabSystem::SaveToDirectory(Actor root, const std::filesystem::path& directory)
	{
		PrefabBlueprint prefab;
		prefab.Capture(root);

		std::filesystem::create_directories(directory);

		const Name* name = root.GetComponent<Name>();
		std::string baseName = (name && !name->name_.str().empty()) ? name->name_.str() : "Actor";

		/// [EN] Probe for a free filename, appending "(N)" until one doesn't already exist.
		/// [JP] 空いているファイル名を探索し、既に存在しなくなるまで "(N)" を付加する。
		std::filesystem::path prefabPath = directory / (baseName + ".prefab");
		Int suffix = 1;
		while (std::filesystem::exists(prefabPath))
		{
			prefabPath = directory / (baseName + "(" + std::to_string(suffix++) + ").prefab");
		}

		if (!prefab.Write(prefabPath))
		{
			return {};
		}

		return prefabPath;
	}
}
