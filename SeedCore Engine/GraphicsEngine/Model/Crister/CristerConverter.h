#pragma once
#include <FoundationEngine/Resource/Asset/AxisConvention.h>

namespace SeedCore
{
	class Crister;

	/**
	* [EN]
	* Converts a Crister between coordinate conventions: the source axis
	* convention into the engine space at import, and the editor's axis
	* convention and base transform changes, which re-bake the model and
	* rewrite its .crister cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister を座標の取り決めの間で変換する。インポート時には元の軸の
	* 取り決めをエンジンの空間へ変換し、エディタでの軸の取り決めと基本の
	* 変換の変更では、モデルを焼き直して .crister キャッシュを書き直す。
	*/
	class SEEDCORE_API CristerConverter
	{
	public:
		/**
		* [EN]
		* Re-converts an already-baked Crister (no source glTF required) from
		* one axis convention to another, in place: decodes every quantised
		* vertex/skin back to full precision, applies deltaBasis to
		* positions/normals/tangents/node transforms/skin inverse-bind
		* matrices/light positions-directions/meshlet bounds, optionally
		* reverses triangle winding, recomputes the quantisation AABBs from
		* the transformed data, re-quantises via BakeMesh(), and
		* re-serialises the result to cristerPath. Does NOT touch GPU
		* resources or bindless indices — the caller must force a reload
		* (Unload then Load) afterward. Returns false if this Crister has no
		* compressed vertex data to convert (e.g. textures-only/degenerate asset).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 既に焼き込み済みの Crister（ソース glTF 不要）を、ある軸コンベンション
		* から別のものへその場で再変換する: 量子化済みの頂点/スキンをフル精度へ
		* デコードし、位置/法線/タンジェント/ノードトランスフォーム/スキン
		* 逆バインド行列/ライト位置・向き/メシュレット境界へ deltaBasis を
		* 適用、必要なら三角形の巻き順を反転、変換後データから量子化 AABB を
		* 再計算し、BakeMesh() で再量子化して cristerPath へ再シリアライズ
		* する。GPU リソースや bindless インデックスは触らない —
		* 呼び出し側が後で強制リロード（Unload → Load）すること。この
		* Crister に変換対象の量子化済み頂点データが無い場合（テクスチャのみ
		* 等の縮退アセット）は false を返す。
		*/
		Bool ApplyAxisConversion(Crister& crister, const Matrix& deltaBasis, Bool flipWinding, const std::filesystem::path& cristerPath)const;

		/**
		* [EN]
		* Applies a global position/rotation(euler degrees)/scale/pivot
		* transform to this Crister's node hierarchy and light
		* positions-directions, then re-serialises to cristerPath. Vertices,
		* skin inverse-bind matrices and meshlet bounds stay in mesh-local
		* space: static SubMeshes are placed by their node's global transform
		* at draw time (SubMeshPlacement) and skinned ones through their
		* joints, so both pick the transform up from the nodes.
		* scale/pivot/rotation compose about pivot first, position is a
		* separate world-space offset applied after. Only root-level nodes
		* (stages_[defaultStage_].nodes_) have their local transform
		* updated - CumulateTransforms() then propagates to every
		* descendant, since post-multiplying the whole transform onto just
		* the root telescopes correctly through the local-transform chain
		* (node.globalTransform_ = local * parentGlobal). Returns false if
		* this Crister has no compressed vertex data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* グローバルな位置/回転(オイラー角、度)/スケール/ピボット変換を
		* この Crister のノード階層とライト位置・向きへ適用し、その後
		* cristerPath へ再シリアライズする。頂点・スキン逆バインド行列・
		* メシュレット境界はメッシュローカル空間のまま: 静的 SubMesh は描画時に
		* ノードのグローバル変換で配置され(SubMeshPlacement)、スキン付きは
		* ジョイント経由なので、どちらもノードから変換を受け取る。
		* スケール/ピボット/回転はまずピボットを中心に合成し、position は
		* その後に適用する独立したワールド空間オフセット。ローカル
		* トランスフォームを更新するのはルートノード
		* (stages_[defaultStage_].nodes_)のみ — CumulateTransforms() が
		* 全子孫へ伝播する。ルートだけに変換全体を後乗せすれば、ローカル
		* トランスフォームの連鎖(node.globalTransform_ = local *
		* parentGlobal)を通じて正しく telescope するため。この Crister に
		* 変換対象の量子化済み頂点データが無い場合は false を返す。
		*/
		Bool ApplyTransformConversion(Crister& crister, Vector3 position, Vector3 rotation, Vector3 scale, Vector3 pivot, const std::filesystem::path& cristerPath)const;

		/**
		* [EN]
		* Converts every node/vertex/skin in crister from its source axis
		* convention into the engine's fixed LH/Y-up/Z-forward space, using
		* the basis resolved from axisConvention. Falls back to the default
		* AxisConvention if axisConvention resolves to an invalid basis
		* (never silently skips conversion).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* crister 内の全ノード/頂点/スキンを、axisConvention から解決された
		* 基底を用いて、ソースの軸コンベンションからエンジン固定の
		* LH/Y-up/Z-forward 空間へ変換する。axisConvention が無効な基底に
		* 解決された場合はデフォルトの AxisConvention にフォールバックする
		* （変換を黙ってスキップすることはない）。
		*/
		void ConvertAxisConvention(Crister& crister, const AxisConvention& axisConvention)const;

	private:
		/**
		* [EN]
		* Change-of-basis for a position: transforms vector in place by basis.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 位置の基底変換。vector を basis でその場変換する。
		*/
		void ConvertPositionByBasis(Vector3& vector, const Matrix& basis)const;

		/**
		* [EN]
		* Transforms the .xyz of a Vector4 (e.g. a tangent) by basis; .w (the
		* handedness sign) is not touched here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Vector4（タンジェント等）の .xyz を basis で変換する。.w（利き手
		* 符号）はここでは触らない。
		*/
		void ConvertPositionByBasis(Vector4& vector, const Matrix& basis)const;

		/**
		* [EN]
		* Change-of-basis for a rotation: transforms quaternion in place by
		* basis.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 回転の基底変換。quaternion を basis でその場変換する。
		*/
		void ConvertRotationByBasis(Quaternion& quaternion, const Matrix& basis)const;

		/**
		* [EN]
		* Change-of-basis for a matrix: transforms matrix in place by basis.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 行列の基底変換。matrix を basis でその場変換する。
		*/
		void ConvertMatrixByBasis(Matrix& matrix, const Matrix& basis)const;

		/**
		* [EN]
		* Swaps two corners of every triangle in crister's vertexIndices_ (the
		* flat, pre-meshlet global index list), reversing the winding order.
		* Called only when the resolved conversion requires it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* crister の vertexIndices_（メシュレット化前のフラットなグローバル
		* インデックス列）の全三角形について2頂点を入れ替え、巻き順を反転
		* する。解決済みの変換が要求するときだけ呼ばれる。
		*/
		void FlipTriangleWinding(Crister& crister)const;
	};
}