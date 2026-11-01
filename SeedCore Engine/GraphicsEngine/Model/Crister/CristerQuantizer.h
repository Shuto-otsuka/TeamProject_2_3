#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>

namespace SeedCore
{
	class Crister;

	/**
	* [EN]
	* Both directions of the Crister vertex format: quantises
	* full-precision vertices into the 16-byte CompressedVertex and the
	* CompressedSkinVertex formats against a dequantisation AABB, bakes a
	* Crister's working vertices into them, and dequantises them back.
	* The math matches Model.hlsli's DecodeModelVertex.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister の頂点フォーマットの行きと帰りの両方を扱う。フル精度の
	* 頂点を、逆量子化 AABB に対して 16 バイトの CompressedVertex と
	* CompressedSkinVertex のフォーマットへ量子化し、Crister の作業用の
	* 頂点をそれらへ焼き込み、またフル精度へ逆量子化する。計算式は
	* Model.hlsli の DecodeModelVertex と一致する。
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
		* Inverse of BakeMesh: decodes compressedVertices_ /
		* compressedSkinVertices_ back into vertices_ and rebuilds
		* vertexIndices_ / subMeshes_ as a flat per-SubMesh triangle list
		* from the LOD 0 meshlets. For a ".crister"-only asset (no source
		* glTF/FBX to re-parse) this is how ModelExporter gets an
		* uncompressed, non-meshlet mesh to export.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* BakeMesh の逆: compressedVertices_ / compressedSkinVertices_ を
		* vertices_ へデコードし直し、LOD 0 メシュレットから vertexIndices_ /
		* subMeshes_ を SubMesh ごとのフラットな三角形リストへ再構築する。
		* ".crister" 単体アセット（再解析できるソース glTF/FBX が無い）を
		* ModelExporter が書き出すための、非圧縮・非メシュレットのメッシュを
		* 得る手段。
		*/
		void Reconstruct(Crister& crister)const;

		/**
		* [EN]
		* Quantises a single full-precision Vertex into the 16-byte GPU
		* format against the given position/texcoord dequantisation AABBs.
		* BakeMesh uses it with the Crister's own AABBs; SoftbodyMesh uses
		* it every frame with an AABB recomputed around the deformed mesh,
		* since a moving mesh would clip against a fixed bake-time AABB.
		* Must stay in sync with Model.hlsli's DecodeModelVertex.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フル精度の Vertex 1 つを、指定された位置/UV の逆量子化 AABB に
		* 対して 16 バイトの GPU フォーマットへ量子化する。BakeMesh は
		* Crister 自身の AABB で、SoftbodyMesh は変形後のメッシュの周りで
		* 毎フレーム計算し直した AABB で使う(動くメッシュは焼き込み時の
		* 固定 AABB ではクリップしてしまうため)。Model.hlsli の
		* DecodeModelVertex と同期を保つこと。
		*/
		NODISCARD CompressedVertex EncodeVertex(const Vertex& vertex, const Vector3& positionMin, const Vector3& positionExtent, const Vector2& texcoordMin, const Vector2& texcoordExtent)const;

		/**
		* [EN]
		* Inverse of EncodeVertex: dequantises a CompressedVertex of the
		* given Crister back into a full-precision Vertex, against that
		* Crister's quantisation AABBs (positionMin_/positionExtent_/
		* texcoordMin_/texcoordExtent_). Skin attributes live in a separate
		* array; see DecodeSkin.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* EncodeVertex の逆。指定した Crister の CompressedVertex を、その
		* Crister の量子化 AABB(positionMin_/positionExtent_/
		* texcoordMin_/texcoordExtent_)に対してフル精度の Vertex へ
		* 逆量子化する。スキン属性は別の配列にある(DecodeSkin 参照)。
		*/
		NODISCARD Vertex DecodeVertex(const Crister& crister, const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Dequantises just the position field of a CompressedVertex,
		* against the Crister's positionMin_/positionExtent_. Callers that
		* only need the position (deduping by position, building a
		* position-only buffer) use this instead of DecodeVertex to skip
		* decoding the normal, tangent and texcoord.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex の position フィールドだけを、Crister の
		* positionMin_/positionExtent_ に対して逆量子化する。位置だけが
		* 要る呼び出し元(位置で重複排除する、位置だけのバッファを作る)は、
		* DecodeVertex の代わりにこれを使い、法線・接線・UV のデコードを
		* 省く。
		*/
		NODISCARD Vector3 DecodePosition(const Crister& crister, const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Dequantises just the texcoord field of a CompressedVertex, against
		* the Crister's texcoordMin_/texcoordExtent_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex の texcoord フィールドだけを、Crister の
		* texcoordMin_/texcoordExtent_ に対して逆量子化する。
		*/
		NODISCARD Vector2 DecodeTexcoord(const Crister& crister, const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Dequantises just the tangent field (xyz direction + handedness
		* sign in .w) of a CompressedVertex. The tangent is not stored
		* against an AABB, so no Crister is needed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex の tangent フィールド(xyz 方向 + .w の利き手
		* 符号)だけを逆量子化する。接線は AABB に対して持っていないので、
		* Crister は要らない。
		*/
		NODISCARD Vector4 DecodeTangent(const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Inverse of EncodeOctahedralNormal: unpacks a 16+16-bit octahedral
		* direction back onto the unit sphere
		* (Shader/Normal.hlsli::OctNormalDecode's CPU-side counterpart).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* EncodeOctahedralNormal の逆。16+16bit のオクタヘドラル符号化された
		* 方向を単位球面上へ戻す(Shader/Normal.hlsli::OctNormalDecode の
		* CPU 側対応)。
		*/
		NODISCARD Vector3 DecodeOctahedralNormal(Uint32 packed)const;

		/**
		* [EN]
		* Dequantises the joint indices and renormalised weights of a
		* CompressedSkinVertex. Weights are renormalised because four 8-bit
		* UNORM values rarely sum to exactly one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedSkinVertex のジョイントインデックスと再正規化済み
		* ウェイトを逆量子化する。4つの 8bit UNORM 値は合計がちょうど1に
		* なることが稀なため再正規化する。
		*/
		void DecodeSkin(const CompressedSkinVertex& compressed, XmUint4& outJoints, Vector4& outWeights)const;

	private:
		/**
		* [EN]
		* Packs a unit direction into the 16+16-bit octahedral encoding
		* (Shader/Normal.hlsli::OctNormalEncode's CPU-side counterpart).
		* Used by EncodeVertex for both the normal and the tangent.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単位方向ベクトルを 16+16bit のオクタヘドラル符号化へ詰める
		* （Shader/Normal.hlsli::OctNormalEncode の CPU 側対応）。EncodeVertex
		* が法線と接線の両方に使う。
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
