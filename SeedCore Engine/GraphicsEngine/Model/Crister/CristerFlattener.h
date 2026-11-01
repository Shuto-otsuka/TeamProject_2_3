#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>

namespace SeedCore
{
	class Crister;

	/**
	* [EN]
	* Flattens each SubMesh's LOD 0 cluster, once per node placement, into
	* the RT proxy: a compact vertex set, its float3 positions, a 32-bit
	* triangle list, the skin attributes and the morph deltas, plus the
	* bounds of the placed geometry. CristerResidency uploads the result.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各 SubMesh の LOD 0 クラスタを、ノードの配置ごとに RT プロキシへ
	* 展開する。コンパクトな頂点集合、その float3 の位置、32bit の
	* 三角形リスト、スキン属性、モーフデルタと、配置後のジオメトリの
	* 境界を作る。結果は CristerResidency がアップロードする。
	*/
	class SEEDCORE_API CristerFlattener
	{
	public:
		/**
		* [EN]
		* Builds the RT proxy from crister, placing each SubMesh by
		* subMeshPlacements (indexed like the SubMeshes), and records each
		* SubMesh's proxy vertex, triangle and morph delta ranges in it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* crister から RT プロキシを作る。各 SubMesh は subMeshPlacements
		* (SubMesh と同じインデックス)で配置し、各 SubMesh のプロキシの
		* 頂点・三角形・モーフデルタの範囲を SubMesh に記録する。
		*/
		void Flatten(Crister& crister, std::span<const DynamicArray<Matrix>> subMeshPlacements);

		/**
		* [EN]
		* Proxy vertices in the 16-byte GPU format, read by the hit shaders.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ヒットシェーダーが読む、16 バイトの GPU フォーマットのプロキシ頂点。
		*/
		NODISCARD std::span<const CompressedVertex> Vertices()const;

		/**
		* [EN]
		* The proxy vertices' positions as float3, the BLAS input.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プロキシ頂点の位置を float3 にしたもの。BLAS の入力。
		*/
		NODISCARD std::span<const Vector3> Positions()const;

		/**
		* [EN]
		* Flat 32-bit triangle indices into the proxy vertices, three per
		* triangle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プロキシ頂点を指すフラットな 32bit の三角形インデックス。三角形
		* 1つにつき3つ。
		*/
		NODISCARD std::span<const Uint32> TriangleIndices()const;

		/**
		* [EN]
		* Skin attributes index-aligned with the proxy vertices; filled only
		* when SkinVerticesBuilt() is true.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プロキシ頂点とインデックスが揃ったスキン属性。SkinVerticesBuilt()
		* が true のときだけ埋まる。
		*/
		NODISCARD std::span<const CompressedSkinVertex> SkinVertices()const;

		/**
		* [EN]
		* Morph target deltas, laid out per target within each SubMesh's
		* proxy vertex range.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各 SubMesh のプロキシ頂点の範囲の中で、ターゲットごとに並ぶ
		* モーフデルタ。
		*/
		NODISCARD std::span<const Vector3> MorphDeltas()const;

		/**
		* [EN]
		* Whether per-vertex skin attributes were built, which happens only
		* for a model with exactly one skin.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 頂点ごとのスキン属性を作ったか。スキンがちょうど1つのモデルの
		* ときだけ作る。
		*/
		NODISCARD Bool SkinVerticesBuilt()const;

		/**
		* [EN]
		* Minimum corner of the placed geometry's bounds; the quantisation
		* AABB when there is no proxy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 配置後のジオメトリの境界の最小コーナー。プロキシが無ければ量子化の
		* AABB。
		*/
		NODISCARD Vector3 PlacedPositionMin()const;

		/**
		* [EN]
		* Extent of the placed geometry's bounds; the quantisation AABB when
		* there is no proxy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 配置後のジオメトリの境界の大きさ。プロキシが無ければ量子化の
		* AABB。
		*/
		NODISCARD Vector3 PlacedPositionExtent()const;

	private:
		/// [EN] Proxy vertices in the 16-byte GPU format.
		/// [JP] 16 バイトの GPU フォーマットのプロキシ頂点。
		DynamicArray<CompressedVertex> vertices_;

		/// [EN] Proxy vertex positions as float3.
		/// [JP] プロキシ頂点の float3 の位置。
		DynamicArray<Vector3> positions_;

		/// [EN] 32-bit triangle indices into vertices_.
		/// [JP] vertices_ を指す 32bit の三角形インデックス。
		DynamicArray<Uint32> triangleIndices_;

		/// [EN] Skin attributes index-aligned with vertices_.
		/// [JP] vertices_ とインデックスが揃ったスキン属性。
		DynamicArray<CompressedSkinVertex> skinVertices_;

		/// [EN] Morph deltas, per target within each SubMesh's range of vertices_.
		/// [JP] 各 SubMesh の vertices_ の範囲の中で、ターゲットごとに並ぶモーフデルタ。
		DynamicArray<Vector3> morphDeltas_;

		/// [EN] Whether skinVertices_ was built (a model with exactly one skin).
		/// [JP] skinVertices_ を作ったか(スキンがちょうど1つのモデル)。
		Bool skinVerticesBuilt_ = false;

		/// [EN] Minimum corner of the placed geometry's bounds.
		/// [JP] 配置後のジオメトリの境界の最小コーナー。
		Vector3 placedPositionMin_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Extent of the placed geometry's bounds.
		/// [JP] 配置後のジオメトリの境界の大きさ。
		Vector3 placedPositionExtent_ = { 1.0f, 1.0f, 1.0f };
	};
}