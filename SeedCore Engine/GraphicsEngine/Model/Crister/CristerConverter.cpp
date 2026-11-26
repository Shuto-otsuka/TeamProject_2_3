#include <GraphicsEngine/Model/Crister/CristerConverter.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	Bool CristerConverter::ApplyAxisConversion(Crister& crister, const Matrix& deltaBasis, Bool flipWinding, const std::filesystem::path& cristerPath)const
	{
		if (crister.geometry_.compressedVertices_.empty())
		{
			return false;
		}

		Float determinant = deltaBasis.Determinant();
		Bool isMirror = determinant < 0.0f;
		Float tangentSign = isMirror ? -1.0f : 1.0f;

		CristerQuantizer quantizer;

		DynamicArray<Vertex> transformedVertices(crister.geometry_.compressedVertices_.size());
		for (Size vertexIndex = 0; vertexIndex < crister.geometry_.compressedVertices_.size(); vertexIndex++)
		{
			const CompressedVertex& compressed = crister.geometry_.compressedVertices_[vertexIndex];
			Vertex& vertex = transformedVertices[vertexIndex];

			Vertex decoded = quantizer.DecodeVertex(crister, compressed);
			vertex.position_ = Vector3::TransformPosition(decoded.position_, deltaBasis);
			vertex.texcoord_ = decoded.texcoord_;
			vertex.normal_ = Vector3::TransformPosition(decoded.normal_, deltaBasis);

			Vector3 tangentXyz = Vector3::TransformPosition(decoded.tangent_.xyz, deltaBasis);
			vertex.tangent_ = Vector4(tangentXyz, decoded.tangent_.w * tangentSign);

			if (!crister.geometry_.compressedSkinVertices_.empty())
			{
				quantizer.DecodeSkin(crister.geometry_.compressedSkinVertices_[vertexIndex], vertex.joints_, vertex.weights_);
			}
		}

		for (Node& node : crister.scene_.nodes_)
		{
			ConvertRotationByBasis(node.rotation_, deltaBasis);
			ConvertPositionByBasis(node.translation_, deltaBasis);
		}
		crister.CumulateTransforms();

		for (Skin& skin : crister.scene_.skins_)
		{
			for (Matrix& inverseBindMatrix : skin.inverseBindMatrices_)
			{
				ConvertMatrixByBasis(inverseBindMatrix, deltaBasis);
			}
		}

		for (PunctualLight& light : crister.scene_.lights_)
		{
			ConvertPositionByBasis(light.position_, deltaBasis);
			ConvertPositionByBasis(light.direction_, deltaBasis);
			light.direction_.Normalize();
		}

		for (MeshletBound& bound : crister.geometry_.meshletBounds_)
		{
			ConvertPositionByBasis(bound.center_, deltaBasis);
			ConvertPositionByBasis(bound.coneAxis_, deltaBasis);
		}

		if (flipWinding)
		{
			Size triangleCount = crister.geometry_.primitiveIndices_.size() / 3;
			for (Size triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
			{
				std::swap(crister.geometry_.primitiveIndices_[triangleIndex * 3 + 1], crister.geometry_.primitiveIndices_[triangleIndex * 3 + 2]);
			}
		}

		crister.geometry_.vertices_ = std::move(transformedVertices);
		quantizer.BakeMesh(crister);

		BinaryOutputArchive archive;
		crister.Serialize(archive);
		if (!archive.Write(String(cristerPath.string())))
		{
			return false;
		}

		return true;
	}

	Bool CristerConverter::ApplyTransformConversion(Crister& crister, Vector3 position, Vector3 rotation, Vector3 scale, Vector3 pivot, const std::filesystem::path& cristerPath)const
	{
		if (crister.geometry_.compressedVertices_.empty())
		{
			return false;
		}

		Vector3 clampedScale(
			std::abs(scale.x) < 0.0001f ? std::copysign(0.0001f, scale.x) : scale.x,
			std::abs(scale.y) < 0.0001f ? std::copysign(0.0001f, scale.y) : scale.y,
			std::abs(scale.z) < 0.0001f ? std::copysign(0.0001f, scale.z) : scale.z);

		Matrix linearBasis = Matrix::CreateScale(clampedScale.x, clampedScale.y, clampedScale.z) * Matrix::CreateFromYawPitchRoll(ToRadians(rotation.y), ToRadians(rotation.x), ToRadians(rotation.z));
		Matrix fullTransform = Matrix::CreateTranslation(-pivot) * linearBasis * Matrix::CreateTranslation(pivot + position);

		if (crister.scene_.defaultStage_ >= 0 && static_cast<Size>(crister.scene_.defaultStage_) < crister.scene_.stages_.size())
		{
			for (Int rootNodeIndex : crister.scene_.stages_[crister.scene_.defaultStage_].nodes_)
			{
				if (rootNodeIndex < 0 || static_cast<Size>(rootNodeIndex) >= crister.scene_.nodes_.size())
				{
					continue;
				}

				Node& node = crister.scene_.nodes_[rootNodeIndex];
				Matrix localScale = Matrix::CreateScale(node.scale_.x, node.scale_.y, node.scale_.z);
				Matrix localRotation = Matrix::CreateFromQuaternion(node.rotation_);
				Matrix localTranslation = Matrix::CreateTranslation(node.translation_.x, node.translation_.y, node.translation_.z);

				Matrix newLocal = localScale * localRotation * localTranslation * fullTransform;

				newLocal.Decompose(node.translation_, node.rotation_, node.scale_);
			}
		}

		crister.CumulateTransforms();

		for (PunctualLight& light : crister.scene_.lights_)
		{
			light.position_ = Vector3::TransformPosition(light.position_, fullTransform);
			light.direction_ = Vector3::TransformDirection(light.direction_, linearBasis);
			light.direction_.Normalize();
		}

		BinaryOutputArchive archive;
		crister.Serialize(archive);
		if (!archive.Write(String(cristerPath.string())))
		{
			return false;
		}

		return true;
	}

	void CristerConverter::ConvertAxisConvention(Crister& crister, const AxisConvention& axisConvention)const
	{
		ResolvedAxisConvention resolved = ResolvedAxisConvention::Resolve(axisConvention);
		if (!resolved.valid_)
		{
			resolved = ResolvedAxisConvention::Resolve(AxisConvention{});
		}

		for (Node& node : crister.scene_.nodes_)
		{
			ConvertRotationByBasis(node.rotation_, resolved.basis_);
			ConvertPositionByBasis(node.translation_, resolved.basis_);
		}
		crister.CumulateTransforms();

		Float tangentSign = resolved.isMirror_ ? -1.0f : 1.0f;
		for (Vertex& vertex : crister.geometry_.vertices_)
		{
			ConvertPositionByBasis(vertex.position_, resolved.basis_);
			ConvertPositionByBasis(vertex.normal_, resolved.basis_);
			ConvertPositionByBasis(vertex.tangent_, resolved.basis_);
			vertex.tangent_.w *= tangentSign;
		}

		for (Skin& skin : crister.scene_.skins_)
		{
			for (Matrix& inverseBindMatrix : skin.inverseBindMatrices_)
			{
				ConvertMatrixByBasis(inverseBindMatrix, resolved.basis_);
			}
		}

		if (resolved.flipWinding_)
		{
			FlipTriangleWinding(crister);
		}
	}

	void CristerConverter::ConvertPositionByBasis(Vector3& vector, const Matrix& basis)const
	{
		vector = Vector3::TransformPosition(vector, basis);
	}

	void CristerConverter::ConvertPositionByBasis(Vector4& vector, const Matrix& basis)const
	{
		vector.xyz = Vector3::TransformPosition(vector.xyz, basis);
	}

	void CristerConverter::ConvertRotationByBasis(Quaternion& quaternion, const Matrix& basis)const
	{
		Matrix rotation = Matrix::CreateFromQuaternion(quaternion);
		Matrix converted = basis.Transpose() * rotation * basis;
		quaternion = Quaternion::CreateFromRotationMatrix(converted);
	}

	void CristerConverter::ConvertMatrixByBasis(Matrix& matrix, const Matrix& basis)const
	{
		matrix = basis.Transpose() * matrix * basis;
	}

	void CristerConverter::FlipTriangleWinding(Crister& crister)const
	{
		Size triangleCount = crister.geometry_.vertexIndices_.size() / 3;
		for (Size triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
		{
			std::swap(crister.geometry_.vertexIndices_[triangleIndex * 3 + 1], crister.geometry_.vertexIndices_[triangleIndex * 3 + 2]);
		}
	}
}