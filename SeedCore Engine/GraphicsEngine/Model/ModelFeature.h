#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Model/ModelDrivenShader.h>
#include <GraphicsEngine/Model/ModelUnit.h>
#include <GraphicsEngine/Model/ModelRaytracing.h>
#include <GraphicsEngine/Model/Material/ModelMaterial.h>
#include <GraphicsEngine/Model/ModelMesh.h>
#include <GraphicsEngine/Model/Morph/ModelMorph.h>
#include <GraphicsEngine/Model/Skin/ModelSkin.h>
#include <GraphicsEngine/Model/Softbody/ModelSoftbody.h>
#include <GraphicsEngine/D3D12/Buffer/StagingBuffer.h>
#include <GraphicsEngine/D3D12/FrameRing.h>

namespace SeedCore
{
	class ModelFeature :public RenderFeature<ModelFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Units, RenderResourceDesc::FixedBuffer(sizeof(ModelUnitStructuredBuffer), ModelUnit::capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Meshes, RenderResourceDesc::FixedBuffer(sizeof(ModelMeshStructuredBuffer), ModelMesh::meshCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Clusters, RenderResourceDesc::FixedBuffer(sizeof(ModelClusterStructuredBuffer), ModelMesh::clusterCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Placements, RenderResourceDesc::FixedBuffer(sizeof(Matrix), ModelMesh::placementCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Materials, RenderResourceDesc::FixedBuffer(sizeof(ModelMaterialStructuredBuffer), ModelMaterial::capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Bones, RenderResourceDesc::FixedBuffer(sizeof(Matrix), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_CONTRACT(Morphs, RenderResourceDesc::FixedBuffer(sizeof(Float), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(UnitUpdates, RenderResourceDesc::FixedBuffer(sizeof(ModelUnitUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(MeshUpdates, RenderResourceDesc::FixedBuffer(sizeof(ModelMeshUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(ClusterUpdates, RenderResourceDesc::FixedBuffer(sizeof(ModelClusterUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(PlacementUpdates, RenderResourceDesc::FixedBuffer(sizeof(ModelPlacementUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(MaterialUpdates, RenderResourceDesc::FixedBuffer(sizeof(ModelMaterialUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Traces, RenderResourceDesc::FixedBuffer(sizeof(ModelRaytracingStructuredBuffer), ModelUnit::capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(TraceUpdates, RenderResourceDesc::FixedBuffer(sizeof(ModelRaytracingUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Deforms, RenderResourceDesc::FixedBuffer(sizeof(ModelDeformStructuredBuffer), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Softbodies, RenderResourceDesc::FixedBuffer(sizeof(ModelStructuredBuffer), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(ModelExpandConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Streaming, RenderResourceDesc::FixedBuffer(sizeof(Uint32), ModelMesh::meshCapacity_ * 2), RenderResourceScope::Frame, RenderResourceLifetime::Transient)
		);

	public:
		struct UpdateStage;

		struct DeformStage;

		struct SubmitStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<ModelUpdateDescription> updatePipeline_;

		ComputePipeline<ModelClearDescription> clearPipeline_;

		ComputePipeline<ModelPrepareDescription> preparePipeline_;

		ComputePipeline<ModelExpandDescription> expandPipeline_;

		ComputePipeline<ModelDeformDescription> deformPipeline_;

		ComputePipeline<ModelRaytracingDescription> raytracingPipeline_;

		RenderDevice* device_ = nullptr;

		Uint64 frame_ = 0;

		Bool updating_ = false;

		Bool deforming_ = false;

		Bool submitting_ = false;

		StaticArray<ResourcePtr<StagingBuffer>, FrameRing::frameCount> readbacks_;

		StaticArray<Uint32, FrameRing::frameCount> readbackCounts_ = {};

		ModelMesh meshes_;

		ModelMaterial materials_;

		ModelSkin skins_;

		ModelMorph morphs_;

		ModelSoftbody softbodies_;

		ModelUnit units_;

		ModelRaytracing raytracing_;
	};
}
