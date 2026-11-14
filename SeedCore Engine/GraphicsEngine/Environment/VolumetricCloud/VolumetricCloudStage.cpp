#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void VolumetricCloudFeature::BuildStage::ShapeStep::Execute(VolumetricCloudFeature& feature, RenderPass& pass)
	{
		feature.shapePipeline_.Dispatch(pass, shapeSize_, shapeSize_, shapeSize_);
	}

	void VolumetricCloudFeature::BuildStage::DetailStep::Execute(VolumetricCloudFeature& feature, RenderPass& pass)
	{
		feature.detailPipeline_.Dispatch(pass, detailSize_, detailSize_, detailSize_);
	}

	void VolumetricCloudFeature::TraceStage::Execute(VolumetricCloudFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
