#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* One glTF scene: a name plus the list of its root-level node
	* indices into nodes_. defaultStage_ selects which Stage renders by
	* default when none is explicitly chosen.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF の1シーンぶん: 名前と、nodes_ を指すルートレベルのノード
	* インデックス一覧。defaultStage_ が、明示的な選択が無い時に
	* デフォルトで描画される Stage を選ぶ。
	*/
	struct Stage
	{
		/// [EN] Name of the glTF scene.
		/// [JP] glTF シーンの名前。
		std::string name_;

		/// [EN] Indices into nodes_ of the scene's root-level nodes.
		/// [JP] シーンのルートレベルのノードの、nodes_ でのインデックス。
		DynamicArray<Int> nodes_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("nodes", nodes_);
		}
	};

	/**
	* [EN]
	* One node in the flattened glTF node hierarchy: local S/R/T, an
	* optional mesh_/skin_/light_ reference, and children_ indices into
	* nodes_. globalTransform_ is cumulated top-down by
	* CumulateTransforms()/ModelLoader::CumulateTransforms from every
	* ancestor's local transform.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 平坦化された glTF ノード階層の1ノード: ローカル S/R/T、任意の
	* mesh_/skin_/light_ 参照、nodes_ を指す children_ インデックス。
	* globalTransform_ は CumulateTransforms()/
	* ModelLoader::CumulateTransforms が全祖先のローカルトランスフォーム
	* から上から下へ累積計算する。
	*/
	struct Node
	{
		/// [EN] Node name from the source file; not guaranteed unique.
		/// [JP] 元ファイルでのノード名。一意とは限らない。
		std::string name_;

		/// [EN] Source mesh this node places, matched against SubMesh::meshIndex_; -1 if none.
		/// [JP] このノードが配置する元のメッシュ。SubMesh::meshIndex_ と照合する。無ければ -1。
		Int mesh_ = -1;

		/// [EN] Index into Crister::skins_ of the skin that deforms this node's mesh; -1 if none.
		/// [JP] このノードのメッシュを変形するスキンの、Crister::skins_ でのインデックス。無ければ -1。
		Int skin_ = -1;

		/// [EN] KHR_lights_punctual: index into Crister::lights_, -1 if none.
		/// [JP] KHR_lights_punctual: Crister::lights_ へのインデックス。無ければ -1。
		Int light_ = -1;

		/// [EN] Indices into nodes_ of this node's children.
		/// [JP] このノードの子の、nodes_ でのインデックス。
		DynamicArray<Int> children_;

		/// [EN] Index into nodes_ of this node's parent; -1 for a root node.
		/// [JP] このノードの親の、nodes_ でのインデックス。ルートノードなら -1。
		Int parentIndex_ = -1;

		/// [EN] Local rotation relative to the parent.
		/// [JP] 親に対するローカル回転。
		Quaternion rotation_ = { 0,0,0,1 };

		/// [EN] Local scale relative to the parent.
		/// [JP] 親に対するローカルスケール。
		Vector3 scale_ = { 1,1,1 };

		/// [EN] Local translation relative to the parent.
		/// [JP] 親に対するローカル平行移動。
		Vector3 translation_ = { 0,0,0 };

		/// [EN] Model-space transform, the local S/R/T multiplied by every ancestor's.
		/// [JP] モデル空間での変換。ローカルの S/R/T に全ての祖先のものを掛けたもの。
		Matrix globalTransform_ = Matrix::Identity;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("mesh", mesh_);
			archive.Field("skin", skin_);
			archive.Field("light", light_);
			archive.Field("children", children_);
			archive.Field("rotation", rotation_);
			archive.Field("scale", scale_);
			archive.Field("translation", translation_);
			archive.Field("global_transform", globalTransform_);
		}
	};

	/**
	* [EN]
	* glTF skin: the joint list (indices into nodes_) and their matching
	* inverse-bind matrices, indexed 1:1. Referenced by Node::skin_ and
	* SubMesh::skinIndex_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF のスキン: ジョイント一覧（nodes_ へのインデックス）と、それに
	* 1対1対応する逆バインド行列。Node::skin_ と SubMesh::skinIndex_ から
	* 参照される。
	*/
	struct Skin
	{
		/// [EN] Inverse bind matrix of each joint, taking a model-space bind-pose vertex into that joint's space; index-aligned with joints_.
		/// [JP] 各ジョイントの逆バインド行列。バインドポーズのモデル空間の頂点を、そのジョイントの空間へ移す。joints_ とインデックスが揃っている。
		DynamicArray<Matrix> inverseBindMatrices_;

		/// [EN] Indices into nodes_ of the joints, in the order CompressedSkinVertex joint indices refer to them.
		/// [JP] ジョイントの nodes_ でのインデックス。CompressedSkinVertex のジョイントインデックスが指す順に並ぶ。
		DynamicArray<Int> joints_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("inverse_bind_matrices", inverseBindMatrices_);
			archive.Field("joints", joints_);
		}
	};

	/**
	* [EN]
	* KHR_lights_punctual point/spot light, resolved to world-space
	* position/direction (via the referencing Node's globalTransform_)
	* at load time. Directional lights are not represented here; see
	* ModelLoader::FetchLights.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* KHR_lights_punctual のポイント/スポットライト。ロード時に参照ノードの
	* globalTransform_ からワールド空間の位置/向きに解決済み。
	* ディレクショナルライトはここに含めない（ModelLoader::FetchLights 参照）。
	*/
	struct PunctualLight
	{
		/**
		* [EN]
		* Which KHR_lights_punctual light kind this PunctualLight represents.
		* Directional is not a value here since directional lights are not
		* represented by PunctualLight at all (see the struct comment above).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この PunctualLight が表す KHR_lights_punctual のライト種別。
		* ディレクショナルは PunctualLight そのもので表現しない
		* （上の構造体コメント参照）ため、値として存在しない。
		*/
		enum class Type : Uint32
		{
			/// [EN] Omnidirectional point light.
			/// [JP] 全方向へ発光するポイントライト。
			Point = 0,

			/// [EN] Cone-shaped spot light (uses direction_/innerConeAngle_/outerConeAngle_).
			/// [JP] 円錐状に発光するスポットライト（direction_/innerConeAngle_/outerConeAngle_ を使う）。
			Spot = 1,
		};

		/// [EN] Kind of light.
		/// [JP] ライトの種類。
		Type type_ = Type::Point;

		/// [EN] Position of the light, resolved from its node.
		/// [JP] ライトの位置。ノードから解決したもの。
		Vector3 position_ = { 0,0,0 };

		/// [EN] Direction the spot light points in; used by Spot only.
		/// [JP] スポットライトが向く方向。Spot だけが使う。
		Vector3 direction_ = { 0,0,-1 };

		/// [EN] Linear RGB color of the light.
		/// [JP] ライトの色(リニア RGB)。
		Float colorRGB_[3] = { 1,1,1 };

		/// [EN] Luminous intensity in candela.
		/// [JP] 光度(カンデラ)。
		Float intensity_ = 1.0f;

		/// [EN] Distance at which the light's influence ends. glTF uses 0 for unbounded, which FetchLights resolves to a finite fallback.
		/// [JP] ライトの影響が届かなくなる距離。glTF では 0 が無限遠を表し、FetchLights がそれを有限の代わりの値に解決する。
		Float range_ = 0.0f;

		/// [EN] Angle from the spot direction where the falloff begins, in radians; used by Spot only.
		/// [JP] スポットの方向から測った、減衰が始まる角度(ラジアン)。Spot だけが使う。
		Float innerConeAngle_ = 0.0f;

		/// [EN] Angle from the spot direction where the light reaches zero, in radians; 45 degrees by default as in the glTF specification. Used by Spot only.
		/// [JP] スポットの方向から測った、光が 0 になる角度(ラジアン)。既定は glTF 仕様と同じ 45 度。Spot だけが使う。
		Float outerConeAngle_ = 0.785398163f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("type", type_);
			archive.Field("position", position_);
			archive.Field("direction", direction_);
			archive.Field("color_rgb", colorRGB_);
			archive.Field("intensity", intensity_);
			archive.Field("range", range_);
			archive.Field("inner_cone_angle", innerConeAngle_);
			archive.Field("outer_cone_angle", outerConeAngle_);
		}
	};

	/**
	* [EN]
	* Scene data of a Crister: the stages, the flattened node hierarchy
	* with each node's global transform, the skins, and the punctual
	* lights of the source file.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister のシーンのデータ。ステージ、各ノードのグローバル変換を持つ
	* 平坦化したノード階層、スキン、元ファイルのパンクチュアルライトを
	* 持つ。
	*/
	struct CristerScene
	{
		/// [EN] Scenes of the source file.
		/// [JP] 元ファイルのシーン。
		DynamicArray<Stage> stages_;

		/// [EN] Flattened node hierarchy of the source file.
		/// [JP] 元ファイルのノード階層を平坦化したもの。
		DynamicArray<Node> nodes_;

		/// [EN] Skins, referred to by Node::skin_ and SubMesh::skinIndex_.
		/// [JP] スキン。Node::skin_ と SubMesh::skinIndex_ から参照される。
		DynamicArray<Skin> skins_;

		/// [EN] Point and spot lights of the source file.
		/// [JP] 元ファイルのポイントライトとスポットライト。
		DynamicArray<PunctualLight> lights_;

		/// [EN] Index into stages_ of the scene drawn when none is chosen.
		/// [JP] シーンが選ばれていないときに描画する、stages_ でのインデックス。
		Int defaultStage_ = 0;
	};
}