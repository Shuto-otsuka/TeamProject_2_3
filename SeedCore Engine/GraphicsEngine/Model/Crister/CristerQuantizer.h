#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>

namespace SeedCore
{
	class Crister;

	/**
	* [EN]
	* Quantises full-precision vertices into the 16-byte CompressedVertex
	* and the CompressedSkinVertex formats against a dequantisation AABB,
	* and bakes a Crister's working vertices into them. The math matches
	* Model.hlsli's DecodeModelVertex.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フル精度の頂点を、逆量子化 AABB に対して 16 バイトの
	* CompressedVertex と CompressedSkinVertex のフォーマットへ量子化し、
	* Crister の作業用の頂点をそれらへ焼き込む。計算式は Model.hlsli の
	* DecodeModelVertex と一致する。
	*/
	class SEEDCORE_API CristerQuantizer
	{
	public:
		/**
		* [EN]
		* Computes the quantisation AABB and bakes vertices_ (and, if
		* skinned, skin attributes) into compressedVertices_ /
		* compressedSkinVertices_, then frees vertices_. Must run after
		* BuildMeshlets (LOD vertex duplication must already be final)
		* and before serialising to the .crister cache.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 量子化 AABB を計算し、vertices_（スキンがあればスキン属性も）を
		* compressedVertices_ / compressedSkinVertices_ へ焼き込んで
		* vertices_ を解放する。BuildMeshlets の後（LOD 頂点複製が確定済み）、
		* .crister キャッシュへのシリアライズ前に実行すること。
		*/
		void BakeMesh(Crister& crister)const;

		/**
		* [EN]
		* Quantises a single full-precision Vertex into the 16-byte GPU
		* format, given the position/texcoord dequantisation AABBs to
		* quantise against. Extracted out of BakeMesh()'s per-vertex loop
		* body so SoftbodyMesh can re-quantise a Softbody's deformed
		* vertices every frame against a freshly computed AABB (the mesh
		* moves, so a fixed bake-time AABB would clip) using the exact same
		* math BakeMesh() bakes into the cache with — must stay in sync with
		* Model.hlsli's DecodeModelVertex, same as BakeMesh().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フル精度の Vertex 1 つを、指定された位置/UV の逆量子化 AABB に
		* 対して 16 バイトの GPU フォーマットへ量子化する。BakeMesh() の
		* 頂点ループ本体から切り出したもの — SoftbodyMesh が Softbody の
		* 変形後頂点を毎フレーム、その都度計算した AABB（メッシュが動くため
		* 焼き込み時固定の AABB ではクリップしてしまう）に対して、
		* BakeMesh() がキャッシュへ焼き込むのと全く同じ計算式で再量子化
		* できるようにする — BakeMesh() と同様、Model.hlsli の
		* DecodeModelVertex と同期を保つこと。
		*/
		NODISCARD CompressedVertex EncodeVertex(const Vertex& vertex, const Vector3& positionMin, const Vector3& positionExtent, const Vector2& texcoordMin, const Vector2& texcoordExtent)const;

	private:
		/**
		* [EN]
		* Packs a unit direction into the 16+16-bit octahedral encoding
		* (Shader/Normal.hlsli::OctNormalEncode's CPU-side counterpart).
		* Static for the same reason as EncodeVertex: SoftbodyMesh calls this
		* indirectly via EncodeVertex with its own bounds, not this Crister's.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単位方向ベクトルを 16+16bit のオクタヘドラル符号化へ詰める
		* （Shader/Normal.hlsli::OctNormalEncode の CPU 側対応）。EncodeVertex
		* と同じ理由で static — SoftbodyMesh が EncodeVertex 経由で、この
		* Crister とは別の自身の境界を使って間接的に呼ぶため。
		*/
		NODISCARD Uint32 EncodeOctahedralNormal(Vector3 direction)const;

		/**
		* [EN]
		* Quantises a [0,1] float to a 16-bit UNORM, clamping out-of-range
		* input first. Shared by EncodeVertex (position/texcoord) and
		* EncodeOctahedralNormal.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* [0,1] の float を 16bit UNORM へ量子化する。範囲外の入力は先に
		* クランプする。EncodeVertex（position/texcoord）と
		* EncodeOctahedralNormal が共有する。
		*/
		NODISCARD Uint32 QuantizeUnorm16(Float value01)const;

		/**
		* [EN]
		* Quantises a [0,1] float to an 8-bit UNORM, clamping out-of-range
		* input first. Used by BakeMesh for skin weights.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* [0,1] の float を 8bit UNORM へ量子化する。範囲外の入力は先に
		* クランプする。BakeMesh がスキンウェイトに使う。
		*/
		NODISCARD Uint32 QuantizeUnorm8(Float value01)const;
	};
}