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
	void CristerQuantizer::Reconstruct(Crister& crister)const
	{
		/// [EN] Nothing to do when full-precision vertices already exist, or when there is nothing to decode.
		/// [JP] フル精度の頂点が既にあるか、デコードするものが無ければ何もしない。
		if (!crister.geometry_.vertices_.empty() || crister.geometry_.compressedVertices_.empty())
		{
			return;
		}

		/// [EN] Decode every compressed vertex, with its skin attributes when present, back into vertices_ at the same global index.
		/// [JP] 全ての圧縮頂点を、スキン属性があればそれも含めて、同じグローバルインデックスのまま vertices_ へデコードし直す。
		crister.geometry_.vertices_.reserve(crister.geometry_.compressedVertices_.size());
		for (Size vertexIndex = 0; vertexIndex < crister.geometry_.compressedVertices_.size(); vertexIndex++)
		{
			Vertex vertex = DecodeVertex(crister, crister.geometry_.compressedVertices_[vertexIndex]);
			if (vertexIndex < crister.geometry_.compressedSkinVertices_.size())
			{
				DecodeSkin(crister.geometry_.compressedSkinVertices_[vertexIndex], vertex.joints_, vertex.weights_);
			}
			crister.geometry_.vertices_.push_back(vertex);
		}

		/// [EN] Rebuild a flat triangle list per SubMesh from its LOD 0 meshlets, resolving each corner to its global vertex index.
		/// [JP] SubMesh ごとに LOD 0 の meshlet からフラットな三角形リストを作り直し、各角をグローバル頂点インデックスへ解決する。
		DynamicArray<Uint32> flatIndices;
		for (SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			Uint32 indexStart = static_cast<Uint32>(flatIndices.size());
			if (subMesh.clusterCount_ > 0 && subMesh.clusterOffset_ < crister.geometry_.clusters_.size())
			{
				const Cluster& cluster = crister.geometry_.clusters_[subMesh.clusterOffset_];
				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							flatIndices.push_back(crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]]);
						}
					}
				}
			}

			/// [EN] Every SubMesh now indexes the whole vertex array directly, and morphs are dropped since the exported mesh has no LOD remap to keep them aligned.
			/// [JP] 各 SubMesh は頂点配列全体を直接指すようにする。書き出すメッシュには揃えを保つ LOD の対応が無いので、モーフは捨てる。
			subMesh.vertexOffset_ = 0;
			subMesh.vertexCount_ = static_cast<Uint32>(crister.geometry_.vertices_.size());
			subMesh.indexOffset_ = indexStart;
			subMesh.indexCount_ = static_cast<Uint32>(flatIndices.size()) - indexStart;
			subMesh.morphs_.clear();
		}

		/// [EN] vertexIndices_ now holds the flat triangle list instead of meshlet-local indices.
		/// [JP] vertexIndices_ は meshlet ローカルなインデックスではなく、フラットな三角形リストを持つようになる。
		crister.geometry_.vertexIndices_ = std::move(flatIndices);
	}

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
	Vertex CristerQuantizer::DecodeVertex(const Crister& crister, const CompressedVertex& compressed)const
	{
		/// [EN] Decode each field separately; the skin attributes live in a different array and are not part of this.
		/// [JP] 各フィールドを個別にデコードする。スキン属性は別の配列にあり、ここには含まない。
		Vertex vertex;
		vertex.position_ = DecodePosition(crister, compressed);
		vertex.normal_ = DecodeOctahedralNormal(compressed.normal_);
		vertex.tangent_ = DecodeTangent(compressed);
		vertex.texcoord_ = DecodeTexcoord(crister, compressed);
		return vertex;
	}

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
	Vector3 CristerQuantizer::DecodePosition(const Crister& crister, const CompressedVertex& compressed)const
	{
		/// [EN] Each 16-bit UNORM maps back across the AABB extent from its minimum corner.
		/// [JP] 各 16bit UNORM を、AABB の最小コーナーから大きさの範囲へ戻す。
		return crister.geometry_.positionMin_ + Vector3(
			static_cast<Float>(compressed.positionXY_ & 0xFFFF) / 65535.0f * crister.geometry_.positionExtent_.x,
			static_cast<Float>(compressed.positionXY_ >> 16) / 65535.0f * crister.geometry_.positionExtent_.y,
			static_cast<Float>(compressed.positionZTexU_ & 0xFFFF) / 65535.0f * crister.geometry_.positionExtent_.z);
	}

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
	Vector2 CristerQuantizer::DecodeTexcoord(const Crister& crister, const CompressedVertex& compressed)const
	{
		/// [EN] u sits in the upper half of positionZTexU_ and v in the lower half of texVTangent_.
		/// [JP] u は positionZTexU_ の上位16bit、v は texVTangent_ の下位16bit にある。
		return crister.geometry_.texcoordMin_ + Vector2(
			static_cast<Float>(compressed.positionZTexU_ >> 16) / 65535.0f * crister.geometry_.texcoordExtent_.x,
			static_cast<Float>(compressed.texVTangent_ & 0xFFFF) / 65535.0f * crister.geometry_.texcoordExtent_.y);
	}

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
	Vector4 CristerQuantizer::DecodeTangent(const CompressedVertex& compressed)const
	{
		/// [EN] The tangent is the upper 16 bits: octahedral x in 8 bits, y in 7 bits and the handedness sign on top.
		/// [JP] 接線は上位16bit。octahedral の x が 8bit、y が 7bit、最上位が利き手の符号。
		Uint32 tangentBits = compressed.texVTangent_ >> 16;

		/// [EN] Widen both components back to 16 bits so the normal decoder can be reused.
		/// [JP] 法線のデコードを使い回せるよう、両成分を 16bit へ広げ直す。
		Uint32 octX16 = static_cast<Uint32>(static_cast<Float>(tangentBits & 0xFF) / 255.0f * 65535.0f + 0.5f);
		Uint32 octY16 = static_cast<Uint32>(static_cast<Float>((tangentBits >> 8) & 0x7F) / 127.0f * 65535.0f + 0.5f);
		Float sign = (tangentBits >> 15) ? 1.0f : -1.0f;

		Vector3 xyz = DecodeOctahedralNormal(octX16 | (octY16 << 16));
		return Vector4(xyz.x, xyz.y, xyz.z, sign);
	}

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
	Vector3 CristerQuantizer::DecodeOctahedralNormal(Uint32 packed)const
	{
		/// [EN] Unpack x and y back to [-1,1] and rebuild z from the octahedron's |x| + |y| + |z| = 1.
		/// [JP] x と y を [-1,1] へ戻し、八面体の |x| + |y| + |z| = 1 から z を作り直す。
		Float ex = static_cast<Float>(packed & 0xFFFF) / 65535.0f * 2.0f - 1.0f;
		Float ey = static_cast<Float>(packed >> 16) / 65535.0f * 2.0f - 1.0f;

		Vector3 n(ex, ey, 1.0f - std::abs(ex) - std::abs(ey));

		/// [EN] A negative z means the point was folded from the lower hemisphere; unfold it.
		/// [JP] z が負なら下半球から折り返した点なので、元に戻す。
		if (n.z < 0.0f)
		{
			Float nx = (1.0f - std::abs(n.y)) * (n.x >= 0.0f ? 1.0f : -1.0f);
			Float ny = (1.0f - std::abs(n.x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
			n.x = nx;
			n.y = ny;
		}

		/// [EN] The octahedron point is not unit length; normalise it back onto the sphere.
		/// [JP] 八面体上の点は長さ 1 ではないので、正規化して球面へ戻す。
		n.Normalize();
		return n;
	}

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
	void CristerQuantizer::DecodeSkin(const CompressedSkinVertex& compressed, XmUint4& outJoints, Vector4& outWeights)const
	{
		/// [EN] Unpack the four 16-bit joint indices.
		/// [JP] 16bit のジョイントインデックスを4つ取り出す。
		outJoints.x = compressed.jointsXY_ & 0xFFFF;
		outJoints.y = compressed.jointsXY_ >> 16;
		outJoints.z = compressed.jointsZW_ & 0xFFFF;
		outJoints.w = compressed.jointsZW_ >> 16;

		outWeights = Vector4(static_cast<Float>(compressed.weights_ & 0xFF), static_cast<Float>((compressed.weights_ >> 8) & 0xFF), static_cast<Float>((compressed.weights_ >> 16) & 0xFF), static_cast<Float>(compressed.weights_ >> 24)) / 255.0f;

		/// [EN] Dividing by the sum both undoes the 8-bit scale and makes the weights add up to 1.
		/// [JP] 合計で割ることで、8bit の倍率を戻すと同時に重みの合計を 1 にする。
		Float weightSum = outWeights.x + outWeights.y + outWeights.z + outWeights.w;
		outWeights /= Max(weightSum, 1e-6f);
	}

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