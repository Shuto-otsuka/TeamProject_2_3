#include <GraphicsEngine/PostProcess/Upscale/UpscaleStage.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	void UpscaleFeature::TaauResolveStage::Execute(UpscaleFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.taauResolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}

	void UpscaleFeature::PrepareGuidesStage::Execute(UpscaleFeature& feature, RenderPass& pass)
	{
		feature.prepareGuidesPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void UpscaleFeature::DlssRayReconstructionStage::Execute(UpscaleFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		Uint32 viewportIndex = static_cast<Uint32>(view->kind_);

		DlssBufferTag tags{};
		tags.colorBuffer_ = pass.Resource(RadianceFeature::Color);
		tags.depthBuffer_ = pass.Resource(GeometryFeature::Depth);
		tags.velocityBuffer_ = pass.Resource(MaterialFeature::Velocity);
		tags.normalRoughnessBuffer_ = pass.Resource(NormalRoughness);
		tags.specularAlbedoBuffer_ = pass.Resource(SpecularAlbedo);
		tags.albedoBuffer_ = pass.Resource(DiffuseAlbedo);
		tags.albedoBufferState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		tags.normalRoughnessBufferState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		tags.specularAlbedoBufferState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		tags.width_ = static_cast<Float>(view->renderWidth_);
		tags.height_ = static_cast<Float>(view->renderHeight_);

		DlssManager& dlssManager = Gateway::GetDlssManager();
		ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
		dlssManager.Constants(view->scene_, viewportIndex, false);
		dlssManager.RayReconstructionTag(tags, cmdList, pass.Resource(Color), viewportIndex, view->displayWidth_, view->displayHeight_);
		dlssManager.EvaluateRayReconstruction(cmdList, view->scene_, viewportIndex, view->displayWidth_, view->displayHeight_, feature.mode_);
	}

	void UpscaleFeature::DlssSuperResolutionStage::Execute(UpscaleFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		Uint32 viewportIndex = static_cast<Uint32>(view->kind_);

		DlssBufferTag tags{};
		tags.colorBuffer_ = pass.Resource(RadianceFeature::Color);
		tags.depthBuffer_ = pass.Resource(GeometryFeature::Depth);
		tags.velocityBuffer_ = pass.Resource(MaterialFeature::Velocity);
		tags.width_ = static_cast<Float>(view->renderWidth_);
		tags.height_ = static_cast<Float>(view->renderHeight_);

		DlssManager& dlssManager = Gateway::GetDlssManager();
		ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
		dlssManager.Constants(view->scene_, viewportIndex, false);
		dlssManager.SuperResolutionTag(tags, cmdList, pass.Resource(Color), viewportIndex, view->displayWidth_, view->displayHeight_);
		dlssManager.EvaluateSuperResolution(cmdList, viewportIndex, view->displayWidth_, view->displayHeight_, feature.mode_);
	}
}
