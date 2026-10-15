#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/World/Actor/Blueprint.h>

namespace SeedCore
{
	class Actor;
	class World;
	class ResourceCache;

	/// [EN] Alias for the reflected field data captured/restored per component; see BlueprintField.
	/// [JP] コンポーネントごとに取得/復元される、リフレクションされたフィールドデータのエイリアス。BlueprintField を参照。
	using PrefabComponentField = BlueprintField;

	/// [EN] Alias for a single captured/restored component; see BlueprintComponent.
	/// [JP] 取得/復元される単一のコンポーネントのエイリアス。BlueprintComponent を参照。
	using PrefabComponent = BlueprintComponent;

	/// [EN] Alias for a single captured/restored actor node (name, components, parent linkage); see BlueprintNode.
	/// [JP] 取得/復元される単一の actor ノード（名前、コンポーネント、親への関連付け）のエイリアス。BlueprintNode を参照。
	using PrefabNode = BlueprintNode;

	/**
	* [EN]
	* Contents of one .prefab file: a serializable snapshot of an Actor
	* subtree (root plus every descendant), stored as a flat,
	* parent-index-linked array of PrefabNode so it round-trips through
	* JSON. Capture() records a live subtree; Instantiate() recreates it
	* (as a new, independent copy of actors) in a World. Gameplay code
	* spawns prefabs through Prefab, and the host through PrefabSystem.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* .prefab ファイル1つ分の中身。Actor のサブツリー（ルートとその全子孫）の
	* シリアライズ可能なスナップショットで、JSON で往復できるよう、親
	* インデックスで連結された PrefabNode のフラットな配列として保存される。
	* Capture() は生きたサブツリーを記録し、Instantiate() はそれを World 内に
	* （actor の新しい独立したコピーとして）再生成する。ゲームプレイコードは
	* Prefab から、ホストは PrefabSystem からプレハブを生成する。
	*/
	class SEEDCORE_API PrefabBlueprint
	{
	public:
		/**
		* [EN]
		* Records root and every descendant into nodes_, replacing any
		* previously captured data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* root とその全子孫を nodes_ へ記録し、以前に取得していたデータを
		* 置き換える。
		*/
		void Capture(Actor root);

		/**
		* [EN]
		* Recreates the captured subtree as new actors in world, parented
		* under parent (if given). Returns the root of the newly created
		* subtree, or nullptr if nothing was instantiated. sourceAssetID
		* is stamped onto the new root so later "apply to prefab" edits
		* know which .prefab asset to overwrite.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 取得済みのサブツリーを、world 内に新しい actor 群として再生成
		* する。parent が指定されていればその下へ配置する。新しく生成
		* されたサブツリーのルートを返す。何もインスタンス化されなければ
		* nullptr を返す。sourceAssetID は新しいルートへ刻印され、後の
		* 「プレハブに適用」編集がどの .prefab アセットを上書きすべきかを
		* 判断できるようにする。
		*/
		Actor Instantiate(World& world, ResourceCache& cache, Actor parent = {}, Uint32 sourceAssetID = 0)const;

		/**
		* [EN]
		* Writes this prefab's captured data to path as JSON. Returns
		* whether the file was written successfully.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この prefab の取得済みデータを、JSON として path へ書き込む。
		* ファイルが正常に書き込まれたかどうかを返す。
		*/
		Bool Write(const std::filesystem::path& path);

		/**
		* [EN]
		* Reads captured data from path (JSON), replacing this prefab's
		* current data. Returns whether reading succeeded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* path（JSON）から取得済みデータを読み込み、この prefab の現在の
		* データを置き換える。読み込みに成功したかどうかを返す。
		*/
		Bool Read(const std::filesystem::path& path);

		/**
		* [EN]
		* Serialization hook (save side): writes nodes_ and basePrefabAssetID_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(保存側): nodes_ と basePrefabAssetID_ を
		* 書き込む。
		*/
		template<class Archive>
		void Save(Archive& archive)const
		{
			archive.Field("nodes", nodes_);
			archive.Field("basePrefabAssetID", basePrefabAssetID_);
		}

		/**
		* [EN]
		* Serialization hook (load side): reads nodes_ and basePrefabAssetID_.
		* Both are optional -- a missing key is swallowed rather than
		* failing the whole load.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(読み込み側): nodes_ と basePrefabAssetID_ を
		* 読み込む。どちらも任意項目 -- キーが無くても読み込み全体を
		* 失敗させず読み飛ばす。
		*/
		template<class Archive>
		void Load(Archive& archive)
		{
			archive.TryField("nodes", nodes_);
			archive.TryField("basePrefabAssetID", basePrefabAssetID_);
		}

	private:
		/// [EN] Flat, parent-index-linked array of every captured actor node in the subtree (index 0 is always the root).
		/// [JP] サブツリー内の全取得済み actor ノードの、親インデックスで連結されたフラットな配列（インデックス0は常にルート）。
		DynamicArray<PrefabNode> nodes_;

		/// [EN] AssetRecord ID of the prefab this one was originally derived from, if any (0 if none).
		/// [JP] このプレハブが元々派生した元プレハブのアセット ID（無ければ0）。
		Uint32 basePrefabAssetID_ = 0;
	};
}
