#pragma once
#include <FoundationEngine/Log/Assert.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	struct ModelTransform
	{
		Matrix world_;
		Matrix inverseTransposeWorld_;
		Matrix previousWorld_;
	};
	SC_STATIC_ASSERT(ModelTransform, 192, "Model/Model.hlsli");

	struct ModelGeometry
	{
		Uint vertexBufferIndex_;
		Uint meshletBufferIndex_;
		Uint meshletBoundBufferIndex_;
		Uint vertexIndicesBufferIndex_;
		Uint primitiveIndicesBufferIndex_;
		Uint meshletOffset_;
		Uint meshletCount_;
		Uint layer_;
	};
	SC_STATIC_ASSERT(ModelGeometry, 32, "Model/Model.hlsli");

	struct ModelStreaming
	{
		Vector3 positionMin_;
		Float texcoordMinU_;

		Vector3 positionExtent_;
		Float texcoordMinV_;

		Vector2 texcoordExtent_;
		Float lodError_;
		Uint lodLevel_;
	};
	SC_STATIC_ASSERT(ModelStreaming, 48, "Model/Model.hlsli");

	struct ModelShading
	{
		Uint shadingModel_;

		Float furLength_;

		Float furDensity_;

		Uint furShellCount_;

		Uint doubleSided_;

		Uint blend_;

		Uint selected_;

		Uint materialMapping_;

		Float tiling_;

		Float triplanarSharpness_;

		Uint actor_;

		Uint modelShadingPadding0_;
	};
	SC_STATIC_ASSERT(ModelShading, 48, "Model/Model.hlsli");

	struct ModelSkining
	{
		Uint skinIndex_;
		Uint boneOffset_;
		Uint skinVertexBufferIndex_;
		Uint modelSkiningPadding0_;
	};
	SC_STATIC_ASSERT(ModelSkining, 16, "Model/Model.hlsli");

	struct ModelMorphing
	{
		Uint morphDeltaBufferIndex_;
		Uint vertexMorphSourceBufferIndex_;
		Uint morphDeltaOffset_;
		Uint morphVertexOffset_;

		Uint morphVertexCount_;
		Uint morphTargetCount_;
		Uint morphWeightOffset_;
		Uint modelMorphPadding0_;
	};
	SC_STATIC_ASSERT(ModelMorphing, 32, "Model/Model.hlsli");

	struct ModelTexture
	{
		Color baseColor_;
		Float metallic_;
		Float roughness_;
		Float alphaCutoff_;
		Float ior_;

		Vector3 emissive_;
		Float emissiveStrength_;

		Uint baseColorTextureIndex_;
		Uint normalTextureIndex_;
		Uint metallicRoughnessTextureIndex_;
		Uint emissiveTextureIndex_;

		Uint occlusionTextureIndex_;
		Vector3 modelTexturePadding0_;
	};
	SC_STATIC_ASSERT(ModelTexture, 80, "Model/Model.hlsli");

	struct ModelExtension
	{
		Float specularFactor_;
		Vector3 specularColor_;
		Float clearCoatFactor_;
		Float clearCoatRoughness_;
		Float anisotropy_;
		Float anisotropyRotation_;
		Float transmissionFactor_;
		Float volumeThicknessFactor_;
		Float volumeAttenuationDistance_;
		Float unlit_;
		Vector3 volumeAttenuationColor_;
		Float sheenRoughness_;
		Vector3 sheenColor_;
		Float iridescenceFactor_;
		Float iridescenceIor_;
		Float iridescenceThickness_;
		Uint specularTextureIndex_;
		Uint specularColorTextureIndex_;
		Uint clearCoatTextureIndex_;
		Uint clearCoatRoughnessTextureIndex_;
		Uint clearCoatNormalTextureIndex_;
		Uint transmissionTextureIndex_;
		Uint thicknessTextureIndex_;
		Uint sheenColorTextureIndex_;
		Uint sheenRoughnessTextureIndex_;
		Uint iridescenceTextureIndex_;
		Uint iridescenceThicknessTextureIndex_;
		Uint anisotropyTextureIndex_;
		Vector2 modelExtensionPadding0_;
	};
	SC_STATIC_ASSERT(ModelExtension, 144, "Model/Model.hlsli");

	struct ModelStructuredBuffer
	{
		ModelTransform transform_;
		ModelGeometry geometry_;
		ModelStreaming streaming_;
		ModelShading shading_;
		ModelSkining skining_;
		ModelMorphing morph_;
		ModelTexture texture_;
		ModelExtension extension_;
	};
	SC_STATIC_ASSERT(ModelStructuredBuffer, 592, "Model/Model.hlsli");

	struct ModelUnitStructuredBuffer
	{
		Matrix world_;

		Matrix previousWorld_;

		Uint meshOffset_ = 0;

		Uint meshCount_ = 0;

		Uint materialOffset_ = 0;

		Uint flags_ = 0;

		Uint boneOffset_ = SC_INVALID;

		Uint morphWeightOffset_ = SC_INVALID;

		Uint modelEntityStructuredBufferPadding0_ = 0;

		Uint modelEntityStructuredBufferPadding1_ = 0;
	};
	SC_STATIC_ASSERT(ModelUnitStructuredBuffer, 160, "Model/Model.hlsli");

	struct ModelMeshStructuredBuffer
	{
		Vector3 positionMin_;
		Float texcoordMinU_ = 0.0f;

		Vector3 positionExtent_;
		Float texcoordMinV_ = 0.0f;

		Vector2 texcoordExtent_;
		Float texcoordDensity_ = 0.0f;
		Uint surfaceIndex_ = 0;

		Uint clusterOffset_ = 0;
		Uint clusterCount_ = 0;
		Uint placementOffset_ = 0;
		Uint placementCount_ = 0;

		Uint skinIndex_ = SC_INVALID;
		Uint skinBoneOffset_ = 0;
		Uint skinVertexBufferIndex_ = SC_INVALID;
		Uint morphDeltaBufferIndex_ = SC_INVALID;

		Uint vertexMorphSourceBufferIndex_ = SC_INVALID;
		Uint morphDeltaOffset_ = 0;
		Uint vertexOffset_ = 0;
		Uint vertexCount_ = 0;

		Uint morphTargetCount_ = 0;
		Uint morphWeightOffset_ = 0;
		Uint modelMeshStructuredBufferPadding0_ = 0;
		Uint modelMeshStructuredBufferPadding1_ = 0;
	};
	SC_STATIC_ASSERT(ModelMeshStructuredBuffer, 112, "Model/Model.hlsli");

	struct ModelClusterStructuredBuffer
	{
		Uint meshletOffset_ = 0;
		Uint meshletCount_ = 0;
		Float lodError_ = 0.0f;
		Uint flags_ = 0;

		Uint vertexBufferIndex_ = SC_INVALID;
		Uint meshletBufferIndex_ = SC_INVALID;
		Uint meshletBoundBufferIndex_ = SC_INVALID;
		Uint vertexIndicesBufferIndex_ = SC_INVALID;

		Uint primitiveIndicesBufferIndex_ = SC_INVALID;
		Uint modelClusterStructuredBufferPadding0_ = 0;
		Uint modelClusterStructuredBufferPadding1_ = 0;
		Uint modelClusterStructuredBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(ModelClusterStructuredBuffer, 48, "Model/Model.hlsli");

	struct ModelMaterialStructuredBuffer
	{
		ModelShading shading_;

		ModelTexture texture_;

		ModelExtension extension_;
	};
	SC_STATIC_ASSERT(ModelMaterialStructuredBuffer, 272, "Model/Model.hlsli");

	struct ModelUnitUpdate
	{
		Uint index_ = 0;
		Uint modelEntityUpdatePadding0_ = 0;
		Uint modelEntityUpdatePadding1_ = 0;
		Uint modelEntityUpdatePadding2_ = 0;

		ModelUnitStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelUnitUpdate, 176, "Model/Model.hlsli");

	struct ModelMeshUpdate
	{
		Uint index_ = 0;
		Uint modelMeshUpdatePadding0_ = 0;
		Uint modelMeshUpdatePadding1_ = 0;
		Uint modelMeshUpdatePadding2_ = 0;

		ModelMeshStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelMeshUpdate, 128, "Model/Model.hlsli");

	struct ModelClusterUpdate
	{
		Uint index_ = 0;
		Uint modelClusterUpdatePadding0_ = 0;
		Uint modelClusterUpdatePadding1_ = 0;
		Uint modelClusterUpdatePadding2_ = 0;

		ModelClusterStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelClusterUpdate, 64, "Model/Model.hlsli");

	struct ModelPlacementUpdate
	{
		Uint index_ = 0;
		Uint modelPlacementUpdatePadding0_ = 0;
		Uint modelPlacementUpdatePadding1_ = 0;
		Uint modelPlacementUpdatePadding2_ = 0;

		Matrix data_;
	};
	SC_STATIC_ASSERT(ModelPlacementUpdate, 80, "Model/Model.hlsli");

	struct ModelMaterialUpdate
	{
		Uint index_ = 0;
		Uint modelMaterialUpdatePadding0_ = 0;
		Uint modelMaterialUpdatePadding1_ = 0;
		Uint modelMaterialUpdatePadding2_ = 0;

		ModelMaterialStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelMaterialUpdate, 288, "Model/Model.hlsli");

	struct ModelRaytracingStructuredBuffer
	{
		Uint64 bottomLevel_ = 0;
		Uint vertexBufferIndex_ = SC_INVALID;
		Uint indexBufferIndex_ = SC_INVALID;

		Uint surfaceBufferIndex_ = SC_INVALID;
		Uint modelRaytracingStructuredBufferPadding0_ = 0;
		Vector2 texcoordMin_ = { 0.0f, 0.0f };

		Vector2 texcoordExtent_ = { 0.0f, 0.0f };
		Uint modelRaytracingStructuredBufferPadding1_ = 0;
		Uint modelRaytracingStructuredBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(ModelRaytracingStructuredBuffer, 48, "Model/Model.hlsli");

	struct ModelRaytracingUpdate
	{
		Uint index_ = 0;
		Uint modelRaytracingUpdatePadding0_ = 0;
		Uint modelRaytracingUpdatePadding1_ = 0;
		Uint modelRaytracingUpdatePadding2_ = 0;

		ModelRaytracingStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelRaytracingUpdate, 64, "Model/Model.hlsli");

	struct ModelDeformStructuredBuffer
	{
		Uint unitIndex_ = 0;
		Uint subMeshIndex_ = 0;
		Uint vertexOffset_ = 0;
		Uint vertexCount_ = 0;

		Uint outputOffset_ = 0;
		Uint morphDeltaOffset_ = 0;
		Uint morphTargetCount_ = 0;
		Uint positionBufferIndex_ = SC_INVALID;

		Uint skinVertexBufferIndex_ = SC_INVALID;
		Uint morphDeltaBufferIndex_ = SC_INVALID;
		Uint64 bottomLevel_ = 0;
	};
	SC_STATIC_ASSERT(ModelDeformStructuredBuffer, 48, "Model/Model.hlsli");

	struct ModelExpandConstantBuffer
	{
		StaticArray<Vector4, RenderViewCount * 6> planes_;

		StaticArray<Vector4, RenderViewCount> cameras_;

		Uint viewCount_ = 0;
		Uint unitCount_ = 0;
		Uint transparentBase_ = 0;
		Uint capacity_ = 0;

		Uint softbodyOpaqueCount_ = 0;
		Uint softbodyTransparentCount_ = 0;
		Uint modelExpandConstantBufferPadding0_ = 0;
		Uint modelExpandConstantBufferPadding1_ = 0;
	};
	SC_STATIC_ASSERT(ModelExpandConstantBuffer, 368, "Model/Model.hlsli");
}
