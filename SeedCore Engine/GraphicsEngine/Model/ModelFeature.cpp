#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/ModelStage.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(ModelFeature, UpdateStage, ExpandStage);

	void ModelFeature::Create(RenderDevice& device)
	{
		device_ = &device;
		updatePipeline_.Create(device);
		preparePipeline_.Create(device);
		expandPipeline_.Create(device);
		softbodies_.Initialize(device);
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

		meshes_.End(device_->ResourceHeap());
		skins_.End();
		morphs_.End();
		softbodies_.End();
		units_.Finish(scene, softbodies_);

		updating_ = !units_.Updates().empty() || !meshes_.MeshUpdates().empty() || !meshes_.ClusterUpdates().empty() || !meshes_.PlacementUpdates().empty() || !materials_.Updates().empty();
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
