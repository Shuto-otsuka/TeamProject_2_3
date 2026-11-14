#include <GraphicsEngine/Shape/HUD/HUDFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/Resource/Scene/Scene.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(HUDFeature, UpdateStage, ComposeStage);

	void HUDFeature::Create(RenderDevice& device)
	{
		composePipeline_.Create(device);
	}

	void HUDFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		Float fade = Scene::FadeAlpha();
		for (const RenderView& view : scene.views_)
		{
			HUDComposeConstantBuffer& constants = constants_[static_cast<Size>(view.kind_)];
			constants.outputRect_ = view.outputRect_;
			constants.fade_ = view.kind_ == RenderViewKind::Game ? fade : 0.0f;
			constants.background_ = view.kind_ == RenderViewKind::Canvas ? Vector3(0.3f, 0.3f, 0.3f) : Vector3(0.0f, 0.0f, 0.0f);
		}
	}

	void HUDFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		for (Uint32 view = 0;view < RenderViewCount;++view)
		{
			upload.Write(Constants, static_cast<RenderViewKind>(view), std::span<const HUDComposeConstantBuffer>(&constants_[view], 1));
		}
	}
}
