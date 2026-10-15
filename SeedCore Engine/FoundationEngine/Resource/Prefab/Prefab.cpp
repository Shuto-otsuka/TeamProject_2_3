#include <FoundationEngine/Resource/Prefab/Prefab.h>
#include <FoundationEngine/Resource/Prefab/PrefabSystem.h>

namespace SeedCore
{
	PrefabSystem* Prefab::system_ = nullptr;

	/**
	* [EN]
	* Loads the .prefab asset identified by assetID and instantiates it in
	* the bound World under parent (when given). Returns the new subtree's
	* root Actor, or nullptr if no PrefabSystem is bound, the asset could
	* not be loaded, or nothing was instantiated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で識別される .prefab アセットを読み込み、束縛済みの World
	* 内、parent が指定されていればその下へインスタンス化する。新しい
	* サブツリーのルート Actor を返す。PrefabSystem が束縛されていない、
	* アセットを読み込めなかった、または何もインスタンス化されなかった
	* 場合は nullptr を返す。
	*/
	Actor Prefab::Spawn(Uint32 assetID, Actor parent)
	{
		if (!system_)
		{
			return Actor();
		}

		return system_->Spawn(assetID, parent);
	}

	/**
	* [EN]
	* Resolves name (which must carry the ".prefab" extension) to an asset
	* ID, then spawns it like Spawn(Uint32, Actor). Logs a warning and
	* returns nullptr if name does not resolve.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name（".prefab" 拡張子を含むこと）をアセット ID へ解決し、
	* Spawn(Uint32, Actor) と同じように生成する。name が解決できなければ
	* 警告をログ出力して nullptr を返す。
	*/
	Actor Prefab::Spawn(const String& name, Actor parent)
	{
		if (!system_)
		{
			return Actor();
		}

		return system_->Spawn(name, parent);
	}
}
