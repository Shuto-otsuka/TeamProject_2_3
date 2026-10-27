#pragma once
#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>

namespace SeedCore
{
	class World;
	class ResourceCache;
	class Actor;
	class SceneSystem;

	/**
	* [EN]
	* Contents of one .scene file: a serializable snapshot of every root
	* Actor (and its descendants) in a World, stored as a flat,
	* parent-index-linked array of BlueprintNode so it round-trips
	* through JSON, together with the scene's name. Capture()/Instantiate()
	* move it between the
	* blueprint and a World; Read()/Write() move it between the blueprint
	* and a file. Gameplay code switches scenes through Scene, and the
	* host drives switching through SceneSystem.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* .scene ファイル1つ分の中身。World 内の全ルート Actor（とその子孫）の
	* シリアライズ可能なスナップショットで、JSON で往復できるよう親
	* インデックスで連結された BlueprintNode のフラットな配列として、
	* そのシーンの名前と共に保持する。
	* Capture()/Instantiate() はこの設計図と World の間で、Read()/Write()
	* はこの設計図とファイルの間で中身を移す。ゲームプレイコードは Scene
	* からシーンを切り替え、ホストは SceneSystem で切り替えを駆動する。
	*/
	class SEEDCORE_API SceneBlueprint
	{
	public:
		/**
		* [EN]
		* Records every root actor (and its descendants) in world into
		* nodes_, replacing any previously captured data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world 内の全ルート actor（とその子孫）を nodes_ へ記録し、以前に
		* 取得していたデータを置き換える。
		*/
		void Capture(World& world);

		/**
		* [EN]
		* Discards any captured actor data, releasing the memory it held.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 取得済みの actor データを破棄し、それが保持していたメモリを解放する。
		*/
		void Clear();

		/**
		* [EN]
		* Recreates the captured scene as new actors in world, returning
		* every instantiated actor (roots and descendants alike).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 取得済みのシーンを world 内に新しい actor 群として再生成し、
		* インスタンス化された全 actor（ルート・子孫問わず）を返す。
		*/
		DynamicArray<Actor> Instantiate(World& world, ResourceCache& cache)const;

		/**
		* [EN]
		* Writes this scene's captured data to path as JSON. Returns
		* whether the file was written successfully.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このシーンの取得済みデータを、JSON として path へ書き込む。
		* ファイルが正常に書き込まれたかどうかを返す。
		*/
		Bool Write(const std::filesystem::path& path);

		/**
		* [EN]
		* Reads captured data from path (JSON), replacing this scene's
		* current data, and takes path's file name without extension as
		* this scene's name. Returns whether reading succeeded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* path（JSON）から取得済みデータを読み込み、このシーンの現在の
		* データを置き換える。path の拡張子を除いたファイル名を、この
		* シーンの名前とする。読み込みに成功したかどうかを返す。
		*/
		Bool Read(const std::filesystem::path& path);

	public:
		/**
		* [EN]
		* Returns the captured actors of the last Read()/Capture(), in the
		* order they were laid out. Used when a scene that arrived from
		* another member has to be matched against the actors already
		* live in the world rather than instantiated from scratch.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近の Read()/Capture() で取得した actor を、並んでいた順のまま
		* 返す。他のメンバーから届いた Scene を、新規にインスタンス化する
		* のではなく、既に world にいる actor と突き合わせる際に使う。
		*/
		NODISCARD const DynamicArray<BlueprintNode>& Nodes()const;

		/**
		* [EN]
		* Returns this scene's name: the file name without extension it
		* was read from, or the name SceneSystem gave it when capturing a
		* world. Empty if neither has happened.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このシーンの名前を返す。読み込み元の拡張子を除いたファイル名か、
		* SceneSystem が world を取得した際に付けた名前。どちらも無ければ空。
		*/
		NODISCARD const String& Name()const;

	public:
		/**
		* [EN]
		* Serialization hook (save side): writes nodes_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(保存側): nodes_ を書き込む。
		*/
		template<class Archive>
		void Save(Archive& archive)const
		{
			archive.Field("nodes", nodes_);
		}

		/**
		* [EN]
		* Serialization hook (load side): reads nodes_. A missing key is
		* swallowed rather than failing the whole load.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(読み込み側): nodes_ を読み込む。キーが無くても
		* 読み込み全体を失敗させず読み飛ばす。
		*/
		template<class Archive>
		void Load(Archive& archive)
		{
			archive.TryField("nodes", nodes_);
		}

	private:
		/// [EN] SceneSystem fills in name_ when capturing a world, since only it knows the current scene's name.
		/// [JP] SceneSystem は world を取得する際に name_ を書き込む。現在のシーン名を知っているのは SceneSystem だけのため。
		friend class SceneSystem;

	private:
		/// [EN] Flat, parent-index-linked array of every captured root actor and its descendants.
		/// [JP] 取得済みの全ルート actor とその子孫の、親インデックスで連結されたフラットな配列。
		DynamicArray<BlueprintNode> nodes_;

		/// [EN] This scene's name (see Name).
		/// [JP] このシーンの名前（Name を参照）。
		String name_;
	};
}
