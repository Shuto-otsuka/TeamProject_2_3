#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/ModelStage.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Raytracing/TopLevelShader.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(ModelFeature, UpdateStage, DeformStage, SubmitStage);

	void ModelFeature::Create(RenderDevice& device)
	{
		device_ = &device;
		updatePipeline_.Create(device);
		clearPipeline_.Create(device);
		preparePipeline_.Create(device);
		expandPipeline_.Create(device);
		deformPipeline_.Create(device);
		raytracingPipeline_.Create(device);
		softbodies_.Initialize(device);
		for (ResourcePtr<StagingBuffer>& readback : readbacks_)
		{
			readback = MakePtr<StagingBuffer>(device.Context().GetDevice(), sizeof(Uint32) * ModelMesh::meshCapacity_ * 2, D3D12_HEAP_TYPE_READBACK);
		}
	}

	void ModelFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		++frame_;
		meshes_.Begin(frame_);
		materials_.Begin(frame_);
		skins_.Begin();
		morphs_.Begin();
		softbodies_.Begin();

		units_.Gather(scene, meshes_, materials_, skins_, morphs_, softbodies_);

		Uint32 slot = FrameRing::Index();
		std::span<const std::byte> streaming = readbacks_[slot]->Read(0, static_cast<Uint64>(readbackCounts_[slot]) * 2 * sizeof(Uint32));
		meshes_.Stream(std::span<const Uint32>(reinterpret_cast<const Uint32*>(streaming.data()), static_cast<Size>(readbackCounts_[slot]) * 2), device_->Context().GetDevice(), *device_->Context().GetFrameTransferLane(), device_->ResourceHeap());
		meshes_.End(device_->ResourceHeap());
		skins_.End();
		morphs_.End();
		softbodies_.End();
		units_.Finish(scene, softbodies_);
		raytracing_.Gather(scene, *device_, frame);

		TopLevelRequest request;
		request.count_ = raytracing_.SubmissionCount();
		frame.Append(request);

		updating_ = !units_.Updates().empty() || !meshes_.MeshUpdates().empty() || !meshes_.ClusterUpdates().empty() || !meshes_.PlacementUpdates().empty() || !materials_.Updates().empty() || !raytracing_.Updates().empty();
		deforming_ = !raytracing_.Deforms().empty();
		submitting_ = raytracing_.SubmissionCount() != 0;
	}

	void ModelFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (!units_.Updates().empty())
		{
			upload.Write(UnitUpdates, RenderViewKind::Game, units_.Updates());
		}
		if (!meshes_.MeshUpdates().empty())
		{
			upload.Write(MeshUpdates, RenderViewKind::Game, meshes_.MeshUpdates());
		}
		if (!meshes_.ClusterUpdates().empty())
		{
			upload.Write(ClusterUpdates, RenderViewKind::Game, meshes_.ClusterUpdates());
		}
		if (!meshes_.PlacementUpdates().empty())
		{
			upload.Write(PlacementUpdates, RenderViewKind::Game, meshes_.PlacementUpdates());
		}
		if (!materials_.Updates().empty())
		{
			upload.Write(MaterialUpdates, RenderViewKind::Game, materials_.Updates());
		}
		if (!raytracing_.Updates().empty())
		{
			upload.Write(RaytracingUnitUpdates, RenderViewKind::Game, raytracing_.Updates());
		}
		if (!raytracing_.Deforms().empty())
		{
			upload.Write(Deforms, RenderViewKind::Game, raytracing_.Deforms());
		}
		if (!skins_.Bones().empty())
		{
			upload.Write(Bones, RenderViewKind::Game, skins_.Bones());
		}
		if (!morphs_.Weights().empty())
		{
			upload.Write(MorphWeights, RenderViewKind::Game, morphs_.Weights());
		}
		if (!softbodies_.Instances().empty())
		{
			upload.Write(SoftbodyInstances, RenderViewKind::Game, softbodies_.Instances());
		}
		upload.Write(Constants, RenderViewKind::Game, std::span<const ModelExpandConstantBuffer>(&units_.Constants(), 1));
	}
}
