#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>

namespace SeedCore
{
	class Crister;

	class SEEDCORE_API CristerQuantizer
	{
	public:
		void BakeMesh(Crister& crister)const;

		void Reconstruct(Crister& crister)const;

		NODISCARD CompressedVertex EncodeVertex(const Vertex& vertex, const Vector3& positionMin, const Vector3& positionExtent, const Vector2& texcoordMin, const Vector2& texcoordExtent)const;

		NODISCARD Vertex DecodeVertex(const Crister& crister, const CompressedVertex& compressed)const;

		NODISCARD Vector3 DecodePosition(const Crister& crister, const CompressedVertex& compressed)const;

		NODISCARD Vector2 DecodeTexcoord(const Crister& crister, const CompressedVertex& compressed)const;

		NODISCARD Vector4 DecodeTangent(const CompressedVertex& compressed)const;

		NODISCARD Vector3 DecodeOctahedralNormal(Uint32 packed)const;

		void DecodeSkin(const CompressedSkinVertex& compressed, Uvector4& outJoints, Vector4& outWeights)const;

	private:
		NODISCARD Uint32 EncodeOctahedralNormal(Vector3 direction)const;

		NODISCARD Uint32 QuantizeUnorm16(Float value01)const;

		NODISCARD Uint32 QuantizeUnorm8(Float value01)const;
	};
}
