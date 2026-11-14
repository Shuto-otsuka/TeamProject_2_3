#include <GraphicsEngine/Screen/Splash/SplashFeature.h>
#include <GraphicsEngine/Screen/Splash/SplashStage.h>
#include <GraphicsEngine/Screen/Splash/SplashRequest.h>
#include <GraphicsEngine/Screen/Splash/SplashTexture.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(SplashFeature, DrawStage);

	void SplashFeature::Create(RenderDevice& device)
	{
		drawPipeline_.Create(device);
	}

	void SplashFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		drawing_ = false;
		const SplashRequest* request = scene.Find<SplashRequest>();
		if (!request || !request->texture_ || scene.views_.empty())
		{
			return;
		}

		const RenderView& view = scene.views_[0];
		constants_ = SplashConstantBuffer{};
		constants_.screenSize_ = Vector2(static_cast<Float>(view.displayWidth_), static_cast<Float>(view.displayHeight_));
		constants_.alpha_ = request->alpha_;
		constants_.aspect_ = request->texture_->Aspect(request->phase_);
		constants_.texture_ = request->texture_->Index(request->phase_);
		drawing_ = constants_.texture_ != SC_INVALID;
	}

	void SplashFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (drawing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const SplashConstantBuffer>(&constants_, 1));
		}
	}
}
