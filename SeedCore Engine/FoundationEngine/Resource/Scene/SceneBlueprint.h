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
	* How a scene wants to be rendered, carried as the serialized form of
	* the raytracing, screen-space and rasterization contexts. The scene
	* stores them without looking inside: they are produced and read back
	* by the renderer, and only travel through here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* その Scene をどう描くか。レイトレーシング、スクリーンスペース、
	* ラスタライゼーションの各コンテキストを文字列にした形で運ぶ。Scene は
	* 中身を見ずに保持する。作るのも読み戻すのもレンダラで、ここは通り道。
	*/
	struct SceneVisual
	{
		/// [EN] Raytracing context as text; empty when the scene never had one.
		/// [JP] レイトレーシングのコンテキストを文字列にしたもの。持っていない Scene では空。
		String raytracing_;

		/// [EN] Screen-space effect context as text; empty for scenes saved before it existed.
		/// [JP] スクリーンスペース系エフェクトのコンテキスト。この項目が存在する前に保存された Scene では空。
		String screenSpace_;

		/// [EN] Rasterization and SDF fallback context as text; empty for scenes saved before it existed.
		/// [JP] ラスタライゼーションと SDF フォールバックのコンテキスト。この項目が存在する前に保存された Scene では空。
		String rasterization_;
	};

	/**
	* [EN]
	* Contents of one .scene file: a serializable snapshot of every root
	* Actor (and its descendants) in a World, stored as a flat,
	* parent-index-linked array of BlueprintNode so it round-trips
	* through JSON, together with how the scene wants to be rendered and
	* the scene's name. Capture()/Instantiate() move it between the
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
	* その Scene をどう描くか、そのシーンの名前と共に保持する。
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
		* Returns how this scene wants to be rendered, as captured by the
		* last Read(). SceneBlueprint itself knows nothing about what
		* these strings hold - FoundationEngine cannot depend on
		* GraphicsEngine - so the renderer is what produces and reads them
		* back.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Scene をどう描くかを、直近の Read() で取得した内容として
		* 返す。SceneBlueprint 自体はこの文字列の中身を知らない
		* （FoundationEngine は GraphicsEngine に依存できない）ため、作るのも
		* 読み戻すのもレンダラ側になる。
		*/
		NODISCARD const SceneVisual& Visual()const;

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
		* Serialization hook (save side): writes nodes_ and how the scene
		* wants to be rendered.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(保存側): nodes_ と、その Scene をどう描くかを
		* 書き込む。
		*/
		template<class Archive>
		void Save(Archive& archive)const
		{
			archive.Field("nodes", nodes_);
			archive.Field("raytracingSettings", visual_.raytracing_);
			archive.Field("screenSpaceSettings", visual_.screenSpace_);
			archive.Field("rasterizationSettings", visual_.rasterization_);
		}

		/**
		* [EN]
		* Serialization hook (load side): reads nodes_ and the three
		* graphics-settings blobs. Each blob is optional -- scenes saved
		* before a given field existed simply leave it empty -- so a
		* missing key is swallowed rather than failing the whole load.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(読み込み側): nodes_ と 3 つのグラフィックス設定
		* blob を読み込む。各 blob は任意項目 -- そのフィールドが存在する前に
		* 保存されたシーンでは単に空になる -- なので、キーが無くても読み込み
		* 全体を失敗させず読み飛ばす。
		*/
		template<class Archive>
		void Load(Archive& archive)
		{
			archive.TryField("nodes", nodes_);
			archive.TryField("raytracingSettings", visual_.raytracing_);
			archive.TryField("screenSpaceSettings", visual_.screenSpace_);
			archive.TryField("rasterizationSettings", visual_.rasterization_);
		}

	private:
		/// [EN] SceneSystem fills in visual_ before saving and name_ when capturing a world, since only it knows the caller's rendering settings and the current scene's name.
		/// [JP] SceneSystem は保存前に visual_ を、world を取得する際に name_ を書き込む。呼び出し側の描画設定と現在のシーン名を知っているのは SceneSystem だけのため。
		friend class SceneSystem;

	private:
		/// [EN] Flat, parent-index-linked array of every captured root actor and its descendants.
		/// [JP] 取得済みの全ルート actor とその子孫の、親インデックスで連結されたフラットな配列。
		DynamicArray<BlueprintNode> nodes_;

		/// [EN] How this scene wants to be rendered, round-tripped by Save()/Load(). Empty parts mean the scene never carried them.
		/// [JP] この Scene をどう描くか。Save()/Load() で往復する。空の項目は、その Scene が元から持っていなかったことを表す。
		SceneVisual visual_;

		/// [EN] This scene's name (see Name).
		/// [JP] このシーンの名前（Name を参照）。
		String name_;
	};
}
