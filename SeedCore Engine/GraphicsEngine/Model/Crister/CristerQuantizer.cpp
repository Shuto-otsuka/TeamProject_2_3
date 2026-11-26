#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <GraphicsEngine/Model/Crister/Crister.h>

namespace SeedCore
{
	void CristerQuantizer::BakeMesh(Crister& crister)const
	{
		{
			Vector3 positionMax = Vector3(0, 0, 0);
			Vector2 texcoordMax = Vector2(0, 0);
			crister.geometry_.positionMin_ = Vector3(0, 0, 0);
			crister.geometry_.texcoordMin_ = Vector2(0, 0);

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

			crister.geometry_.positionExtent_ = Vector3::Max(positionMax - crister.geometry_.positionMin_, Vector3(1e-6f, 1e-6f, 1e-6f));
			crister.geometry_.texcoordExtent_ = Vector2::Max(texcoordMax - crister.geometry_.texcoordMin_, Vector2(1e-6f, 1e-6f));
		}

		crister.geometry_.compressedVertices_.resize(crister.geometry_.vertices_.size());
		for (Size vertexIndex = 0; vertexIndex < crister.geometry_.vertices_.size(); vertexIndex++)
		{
			crister.geometry_.compressedVertices_[vertexIndex] = EncodeVertex(crister.geometry_.vertices_[vertexIndex], crister.geometry_.positionMin_, crister.geometry_.positionExtent_, crister.geometry_.texcoordMin_, crister.geometry_.texcoordExtent_);
		}

		if (!crister.scene_.skins_.empty())
		{
			crister.geometry_.compressedSkinVertices_.resize(crister.geometry_.vertices_.size());
			for (Size vertexIndex = 0; vertexIndex < crister.geometry_.vertices_.size(); vertexIndex++)
			{
				const Vertex& vertex = crister.geometry_.vertices_[vertexIndex];
				CompressedSkinVertex& skin = crister.geometry_.compressedSkinVertices_[vertexIndex];

				skin.jointsXY_ = (vertex.joints_.x & 0xFFFF) | ((vertex.joints_.y & 0xFFFF) << 16);
				skin.jointsZW_ = (vertex.joints_.z & 0xFFFF) | ((vertex.joints_.w & 0xFFFF) << 16);

				skin.weights_ =
					QuantizeUnorm8(vertex.weights_.x) |
					(QuantizeUnorm8(vertex.weights_.y) << 8) |
					(QuantizeUnorm8(vertex.weights_.z) << 16) |
					(QuantizeUnorm8(vertex.weights_.w) << 24);
			}
		}

		crister.geometry_.vertices_.clear();
		crister.geometry_.vertices_.shrink_to_fit();
	}

	void CristerQuantizer::Reconstruct(Crister& crister)const
	{
		if (!crister.geometry_.vertices_.empty() || crister.geometry_.compressedVertices_.empty())
		{
			return;
		}

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

			subMesh.vertexOffset_ = 0;
			subMesh.vertexCount_ = static_cast<Uint32>(crister.geometry_.vertices_.size());
			subMesh.indexOffset_ = indexStart;
			subMesh.indexCount_ = static_cast<Uint32>(flatIndices.size()) - indexStart;
			subMesh.morphs_.clear();
		}

		crister.geometry_.vertexIndices_ = std::move(flatIndices);
	}

	CompressedVertex CristerQuantizer::EncodeVertex(const Vertex& vertex, const Vector3& positionMin, const Vector3& positionExtent, const Vector2& texcoordMin, const Vector2& texcoordExtent)const
	{
		Uint32 quantizedX = QuantizeUnorm16((vertex.position_.x - positionMin.x) / positionExtent.x);
		Uint32 quantizedY = QuantizeUnorm16((vertex.position_.y - positionMin.y) / positionExtent.y);
		Uint32 quantizedZ = QuantizeUnorm16((vertex.position_.z - positionMin.z) / positionExtent.z);
		Uint32 quantizedU = QuantizeUnorm16((vertex.texcoord_.x - texcoordMin.x) / texcoordExtent.x);
		Uint32 quantizedV = QuantizeUnorm16((vertex.texcoord_.y - texcoordMin.y) / texcoordExtent.y);

		Uint32 tangentOctahedral = EncodeOctahedralNormal(vertex.tangent_.xyz);

		Uint32 tangentOctX8 = ((tangentOctahedral & 0xFFFF) * 255 + 32767) / 65535;
		Uint32 tangentOctY7 = ((tangentOctahedral >> 16) * 127 + 32767) / 65535;
		Uint32 tangentSign = vertex.tangent_.w >= 0.0f ? 1u : 0u;
		Uint32 quantizedTangent = tangentOctX8 | (tangentOctY7 << 8) | (tangentSign << 15);

		CompressedVertex compressed;
		compressed.positionXY_ = quantizedX | (quantizedY << 16);
		compressed.positionZTexU_ = quantizedZ | (quantizedU << 16);
		compressed.texVTangent_ = quantizedV | (quantizedTangent << 16);
		compressed.normal_ = EncodeOctahedralNormal(vertex.normal_);
		return compressed;
	}

	Vertex CristerQuantizer::DecodeVertex(const Crister& crister, const CompressedVertex& compressed)const
	{
		Vertex vertex;
		vertex.position_ = DecodePosition(crister, compressed);
		vertex.normal_ = DecodeOctahedralNormal(compressed.normal_);
		vertex.tangent_ = DecodeTangent(compressed);
		vertex.texcoord_ = DecodeTexcoord(crister, compressed);
		return vertex;
	}

	Vector3 CristerQuantizer::DecodePosition(const Crister& crister, const CompressedVertex& compressed)const
	{
		return crister.geometry_.positionMin_ + Vector3(
			static_cast<Float>(compressed.positionXY_ & 0xFFFF) / 65535.0f * crister.geometry_.positionExtent_.x,
			static_cast<Float>(compressed.positionXY_ >> 16) / 65535.0f * crister.geometry_.positionExtent_.y,
			static_cast<Float>(compressed.positionZTexU_ & 0xFFFF) / 65535.0f * crister.geometry_.positionExtent_.z);
	}

	Vector2 CristerQuantizer::DecodeTexcoord(const Crister& crister, const CompressedVertex& compressed)const
	{
		return crister.geometry_.texcoordMin_ + Vector2(
			static_cast<Float>(compressed.positionZTexU_ >> 16) / 65535.0f * crister.geometry_.texcoordExtent_.x,
			static_cast<Float>(compressed.texVTangent_ & 0xFFFF) / 65535.0f * crister.geometry_.texcoordExtent_.y);
	}

	Vector4 CristerQuantizer::DecodeTangent(const CompressedVertex& compressed)const
	{
		Uint32 tangentBits = compressed.texVTangent_ >> 16;

		Uint32 octX16 = static_cast<Uint32>(static_cast<Float>(tangentBits & 0xFF) / 255.0f * 65535.0f + 0.5f);
		Uint32 octY16 = static_cast<Uint32>(static_cast<Float>((tangentBits >> 8) & 0x7F) / 127.0f * 65535.0f + 0.5f);
		Float sign = (tangentBits >> 15) ? 1.0f : -1.0f;

		Vector3 xyz = DecodeOctahedralNormal(octX16 | (octY16 << 16));
		return Vector4(xyz, sign);
	}

	Vector3 CristerQuantizer::DecodeOctahedralNormal(Uint32 packed)const
	{
		Float ex = static_cast<Float>(packed & 0xFFFF) / 65535.0f * 2.0f - 1.0f;
		Float ey = static_cast<Float>(packed >> 16) / 65535.0f * 2.0f - 1.0f;

		Vector3 n(ex, ey, 1.0f - std::abs(ex) - std::abs(ey));

		if (n.z < 0.0f)
		{
			Float nx = (1.0f - std::abs(n.y)) * (n.x >= 0.0f ? 1.0f : -1.0f);
			Float ny = (1.0f - std::abs(n.x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
			n.x = nx;
			n.y = ny;
		}

		n.Normalize();
		return n;
	}

	void CristerQuantizer::DecodeSkin(const CompressedSkinVertex& compressed, Uvector4& outJoints, Vector4& outWeights)const
	{
		outJoints.x = compressed.jointsXY_ & 0xFFFF;
		outJoints.y = compressed.jointsXY_ >> 16;
		outJoints.z = compressed.jointsZW_ & 0xFFFF;
		outJoints.w = compressed.jointsZW_ >> 16;

		outWeights = Vector4(static_cast<Float>(compressed.weights_ & 0xFF), static_cast<Float>((compressed.weights_ >> 8) & 0xFF), static_cast<Float>((compressed.weights_ >> 16) & 0xFF), static_cast<Float>(compressed.weights_ >> 24)) / 255.0f;

		Float weightSum = outWeights.x + outWeights.y + outWeights.z + outWeights.w;
		outWeights /= Max(weightSum, 1e-6f);
	}

	Uint32 CristerQuantizer::EncodeOctahedralNormal(Vector3 direction)const
	{
		Float sum = std::abs(direction.x) + std::abs(direction.y) + std::abs(direction.z);
		if (sum < 1e-8f)
		{
			direction = Vector3(0, 0, 1);
			sum = 1.0f;
		}

		Float x = direction.x / sum;
		Float y = direction.y / sum;

		if (direction.z < 0.0f)
		{
			Float foldedX = (1.0f - std::abs(y)) * (x >= 0.0f ? 1.0f : -1.0f);
			Float foldedY = (1.0f - std::abs(x)) * (y >= 0.0f ? 1.0f : -1.0f);
			x = foldedX;
			y = foldedY;
		}

		return QuantizeUnorm16(x * 0.5f + 0.5f) | (QuantizeUnorm16(y * 0.5f + 0.5f) << 16);
	}

	Uint32 CristerQuantizer::QuantizeUnorm16(Float value01)const
	{
		Float clamped = value01 < 0.0f ? 0.0f : (value01 > 1.0f ? 1.0f : value01);
		return static_cast<Uint32>(clamped * 65535.0f + 0.5f);
	}

	Uint32 CristerQuantizer::QuantizeUnorm8(Float value01)const
	{
		Float clamped = value01 < 0.0f ? 0.0f : (value01 > 1.0f ? 1.0f : value01);
		return static_cast<Uint32>(clamped * 255.0f + 0.5f);
	}
}