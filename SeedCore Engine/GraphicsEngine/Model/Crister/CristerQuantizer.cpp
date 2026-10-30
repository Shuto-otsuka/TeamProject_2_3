#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <GraphicsEngine/Model/Crister/Crister.h>

namespace SeedCore
{
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
	void CristerQuantizer::BakeMesh(Crister& crister)const
	{
		/// [EN] Compute the dequantisation AABBs, then quantise every vertex into the 16-byte GPU format (and skinning attributes, if skinned). Extents are clamped away from zero so flat axes still dequantise to the shared min value. Must run after BuildMeshlets, since QEM appends fresh vertex copies per LOD.
		/// [JP] 逆量子化 AABB を計算し、全頂点を 16 バイトの GPU フォーマット（とスキンがあればスキニング属性）へ量子化する。extent はゼロから離してクランプし、平坦な軸でも共通の min 値へ正しく逆量子化されるようにする。QEM が LOD ごとに新頂点を追加するため、BuildMeshlets の後に実行すること。
		{
			Vector3 positionMax = Vector3(0, 0, 0);
			Vector2 texcoordMax = Vector2(0, 0);
			crister.geometry_.positionMin_ = Vector3(0, 0, 0);
			crister.geometry_.texcoordMin_ = Vector2(0, 0);

			/// [EN] Grow the position and texcoord bounds from the first vertex over every vertex; with no vertices the AABB stays at the origin.
			/// [JP] 最初の頂点から始めて、全頂点で位置とテクスチャ座標の境界を広げる。頂点が無ければ AABB は原点のまま。
			if (!crister.geometry_.vertices_.empty())
			{
				crister.geometry_.positionMin_ = crister.geometry_.vertices_[0].position_;
				positionMax = crister.geometry_.vertices_[0].position_;
				crister.geometry_.texcoordMin_ = crister.geometry_.vertices_[0].texcoord_;
				texcoordMax = crister.geometry_.vertices_[0].texcoord_;
				for (const Vertex& vertex : crister.geometry_.vertices_)
				{
					crister.geometry_.positionMin_ = Vector3::Min(crister.geometry_.positionMin_, vertex.position_);
					positionMax = Vector3::Max(positionMax, vertex.position_);
					crister.geometry_.texcoordMin_ = Vector2::Min(crister.geometry_.texcoordMin_, vertex.texcoord_);
					texcoordMax = Vector2::Max(texcoordMax, vertex.texcoord_);
				}
			}

			/// [EN] The extent divides the position during quantisation, so a flat axis is kept slightly above zero.
			/// [JP] 量子化では大きさで位置を割るので、平坦な軸もわずかに 0 より大きく保つ。
			crister.geometry_.positionExtent_ = Vector3::Max(positionMax - crister.geometry_.positionMin_, Vector3(1e-6f, 1e-6f, 1e-6f));
			crister.geometry_.texcoordExtent_ = Vector2::Max(texcoordMax - crister.geometry_.texcoordMin_, Vector2(1e-6f, 1e-6f));
		}

		/// [EN] Quantise every vertex into the 16-byte GPU format against the AABB just computed.
		/// [JP] 求めた AABB に対して、全頂点を 16 バイトの GPU フォーマットへ量子化する。
		crister.geometry_.compressedVertices_.resize(crister.geometry_.vertices_.size());
		for (Size vertexIndex = 0; vertexIndex < crister.geometry_.vertices_.size(); vertexIndex++)
		{
			crister.geometry_.compressedVertices_[vertexIndex] = EncodeVertex(crister.geometry_.vertices_[vertexIndex], crister.geometry_.positionMin_, crister.geometry_.positionExtent_, crister.geometry_.texcoordMin_, crister.geometry_.texcoordExtent_);
		}

		/// [EN] Skinned models also get one CompressedSkinVertex per vertex; static models skip this array entirely.
		/// [JP] スキン付きモデルは頂点ごとに CompressedSkinVertex も持つ。静的モデルはこの配列ごと持たない。
		if (!crister.scene_.skins_.empty())
		{
			crister.geometry_.compressedSkinVertices_.resize(crister.geometry_.vertices_.size());
			for (Size vertexIndex = 0; vertexIndex < crister.geometry_.vertices_.size(); vertexIndex++)
			{
				const Vertex& vertex = crister.geometry_.vertices_[vertexIndex];
				CompressedSkinVertex& skin = crister.geometry_.compressedSkinVertices_[vertexIndex];

				/// [EN] Pack the four joint indices as 16 bits each, two per word.
				/// [JP] 4つのジョイントインデックスを 16bit ずつ、1ワードに2つ詰める。
				skin.jointsXY_ = (vertex.joints_.x & 0xFFFF) | ((vertex.joints_.y & 0xFFFF) << 16);
				skin.jointsZW_ = (vertex.joints_.z & 0xFFFF) | ((vertex.joints_.w & 0xFFFF) << 16);

				/// [EN] Pack the four weights as 8-bit UNORM; the shader renormalises them because the rounded values rarely sum to exactly 1.
				/// [JP] 4つの重みを 8bit UNORM で詰める。丸めた値の合計はちょうど 1 になることが少ないので、シェーダーで正規化し直す。
				skin.weights_ =
					QuantizeUnorm8(vertex.weights_.x) |
					(QuantizeUnorm8(vertex.weights_.y) << 8) |
					(QuantizeUnorm8(vertex.weights_.z) << 16) |
					(QuantizeUnorm8(vertex.weights_.w) << 24);
			}
		}

		/// [EN] The full-precision vertices are no longer needed; release their memory rather than only emptying them.
		/// [JP] フル精度の頂点はもう要らないので、空にするだけでなくメモリも返す。
		crister.geometry_.vertices_.clear();
		crister.geometry_.vertices_.shrink_to_fit();
	}

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
	CompressedVertex CristerQuantizer::EncodeVertex(const Vertex& vertex, const Vector3& positionMin, const Vector3& positionExtent, const Vector2& texcoordMin, const Vector2& texcoordExtent)const
	{
		/// [EN] Position and texcoord map into [0,1] within their AABBs and are stored as 16-bit UNORM.
		/// [JP] 位置とテクスチャ座標は、それぞれの AABB の中で [0,1] へ写し、16bit UNORM で持つ。
		Uint32 quantizedX = QuantizeUnorm16((vertex.position_.x - positionMin.x) / positionExtent.x);
		Uint32 quantizedY = QuantizeUnorm16((vertex.position_.y - positionMin.y) / positionExtent.y);
		Uint32 quantizedZ = QuantizeUnorm16((vertex.position_.z - positionMin.z) / positionExtent.z);
		Uint32 quantizedU = QuantizeUnorm16((vertex.texcoord_.x - texcoordMin.x) / texcoordExtent.x);
		Uint32 quantizedV = QuantizeUnorm16((vertex.texcoord_.y - texcoordMin.y) / texcoordExtent.y);

		/// [EN] Tangent is 8+7-bit octahedral plus a 1-bit handedness sign; error in the tangent-space rotation of a normal map is harder to see than error in the normal itself, so the coarser precision is enough.
		/// [JP] タンジェントは 8+7bit octahedral + 利き手符号 1bit。法線マップの接空間回転誤差は法線本体より知覚されにくいため粗くて足りる。
		Uint32 tangentOctahedral = EncodeOctahedralNormal(Vector3(vertex.tangent_.x, vertex.tangent_.y, vertex.tangent_.z));

		/// [EN] Requantise the 16-bit octahedral components to 8 and 7 bits with rounding, leaving the top bit for the sign.
		/// [JP] 16bit の octahedral 成分を丸めながら 8bit と 7bit に量子化し直し、最上位の1bit を符号に空ける。
		Uint32 tangentOctX8 = ((tangentOctahedral & 0xFFFF) * 255 + 32767) / 65535;
		Uint32 tangentOctY7 = ((tangentOctahedral >> 16) * 127 + 32767) / 65535;
		Uint32 tangentSign = vertex.tangent_.w >= 0.0f ? 1u : 0u;
		Uint32 quantizedTangent = tangentOctX8 | (tangentOctY7 << 8) | (tangentSign << 15);

		/// [EN] Lay the fields out exactly as DecodeModelVertex in Model.hlsli reads them.
		/// [JP] Model.hlsli の DecodeModelVertex が読むのと全く同じ配置で詰める。
		CompressedVertex compressed;
		compressed.positionXY_ = quantizedX | (quantizedY << 16);
		compressed.positionZTexU_ = quantizedZ | (quantizedU << 16);
		compressed.texVTangent_ = quantizedV | (quantizedTangent << 16);
		compressed.normal_ = EncodeOctahedralNormal(vertex.normal_);
		return compressed;
	}

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
	Uint32 CristerQuantizer::EncodeOctahedralNormal(Vector3 direction)const
	{
		/// [EN] Project the direction onto the octahedron |x| + |y| + |z| = 1; a zero vector falls back to +Z.
		/// [JP] 方向を八面体 |x| + |y| + |z| = 1 へ投影する。零ベクトルは +Z にする。
		Float sum = std::abs(direction.x) + std::abs(direction.y) + std::abs(direction.z);
		if (sum < 1e-8f)
		{
			direction = Vector3(0, 0, 1);
			sum = 1.0f;
		}

		Float x = direction.x / sum;
		Float y = direction.y / sum;

		/// [EN] Fold the lower hemisphere over the diagonals so the whole sphere fits in the [-1,1] square.
		/// [JP] 下半球を対角線で折り返し、球全体を [-1,1] の正方形に収める。
		if (direction.z < 0.0f)
		{
			Float foldedX = (1.0f - std::abs(y)) * (x >= 0.0f ? 1.0f : -1.0f);
			Float foldedY = (1.0f - std::abs(x)) * (y >= 0.0f ? 1.0f : -1.0f);
			x = foldedX;
			y = foldedY;
		}

		/// [EN] Remap [-1,1] to [0,1] and pack x and y as 16 bits each.
		/// [JP] [-1,1] を [0,1] へ写し、x と y を 16bit ずつ詰める。
		return QuantizeUnorm16(x * 0.5f + 0.5f) | (QuantizeUnorm16(y * 0.5f + 0.5f) << 16);
	}

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
	Uint32 CristerQuantizer::QuantizeUnorm16(Float value01)const
	{
		/// [EN] Clamp to [0,1], then round to the nearest of 65536 steps.
		/// [JP] [0,1] にクランプし、65536 段階の最も近い値へ丸める。
		Float clamped = value01 < 0.0f ? 0.0f : (value01 > 1.0f ? 1.0f : value01);
		return static_cast<Uint32>(clamped * 65535.0f + 0.5f);
	}

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
	Uint32 CristerQuantizer::QuantizeUnorm8(Float value01)const
	{
		/// [EN] Clamp to [0,1], then round to the nearest of 256 steps.
		/// [JP] [0,1] にクランプし、256 段階の最も近い値へ丸める。
		Float clamped = value01 < 0.0f ? 0.0f : (value01 > 1.0f ? 1.0f : value01);
		return static_cast<Uint32>(clamped * 255.0f + 0.5f);
	}
}