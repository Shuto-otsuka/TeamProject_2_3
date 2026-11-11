#include <GraphicsEngine/PostProcess/Upscale/UpscaleFeature.h>
#include <GraphicsEngine/PostProcess/Upscale/UpscaleStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(UpscaleFeature, TaauResolveStage, PrepareGuidesStage, DlssRayReconstructionStage, DlssSuperResolutionStage);

	void UpscaleFeature::Create(RenderDevice& device)
	{
		taauResolvePipeline_.Create(device);
		prepareGuidesPipeline_.Create(device);
	}

	void UpscaleFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		const GraphicsUpscale* upscale = scene.Find<GraphicsUpscale>();
		UpscaleMethod method = upscale ? upscale->method_ : UpscaleMethod::Taau;
		mode_ = upscale ? upscale->mode_ : UpscaleMode::MaxQuality;

		DlssManager& dlssManager = Gateway::GetDlssManager();
		dlssManager.RayReconstructionEnable(method == UpscaleMethod::DlssRayReconstruction);
		dlssManager.SuperResolutionEnable(method == UpscaleMethod::DlssSuperResolution);

		dlssRayReconstruction_ = dlssManager.RayReconstructionEnable();
		dlssSuperResolution_ = dlssManager.SuperResolutionEnable();
		taau_ = !dlssRayReconstruction_ && !dlssSuperResolution_;
	}
}
