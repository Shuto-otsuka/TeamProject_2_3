#include <GraphicsEngine/Screen/Boot/BootFeature.h>
#include <GraphicsEngine/Screen/Boot/BootStage.h>
#include <GraphicsEngine/Screen/Boot/BootRequest.h>
#include <GraphicsEngine/Screen/Boot/BootTexture.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(BootFeature, DrawStage);

	void BootFeature::Create(RenderDevice& device)
	{
		drawPipeline_.Create(device);
	}

	void BootFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		drawing_ = false;
		const BootRequest* request = scene.Find<BootRequest>();
		if (!request || !request->config_ || !request->texture_ || request->texture_->BackgroundIndex() == SC_INVALID || scene.views_.empty())
		{
			return;
		}

		const BootConfig& config = *request->config_;
		const BootTexture& texture = *request->texture_;
		const RenderView& view = scene.views_[0];
		Float width = static_cast<Float>(view.displayWidth_);
		Float height = static_cast<Float>(view.displayHeight_);

		constants_ = BootConstantBuffer{};
		constants_.screenSize_ = Vector2(width, height);
		constants_.alpha_ = request->alpha_;
		constants_.progress_ = request->progress_;
		constants_.time_ = request->time_;
		constants_.backgroundAspect_ = texture.BackgroundAspect();
		constants_.fillMethod_ = static_cast<Uint>(config.fillMethod_);
		constants_.fillOrigin_ = static_cast<Uint>(Max(config.fillOrigin_, 0));
		constants_.clockwise_ = config.clockwise_ ? 1 : 0;
		constants_.wave_ = config.wave_ ? 1 : 0;
		constants_.useBackgroundImage_ = config.useBackgroundImage_ ? 1 : 0;
		constants_.useFrame_ = config.useFrame_ ? 1 : 0;
		constants_.barRect_ = texture.BarRect(config, width, height);
		constants_.backgroundColor_ = Vector4(config.backgroundColor_.x, config.backgroundColor_.y, config.backgroundColor_.z, config.backgroundColor_.w);
		constants_.backgroundTint_ = Vector4(config.backgroundTint_.x, config.backgroundTint_.y, config.backgroundTint_.z, config.backgroundTint_.w);
		constants_.barTint_ = Vector4(config.barTint_.x, config.barTint_.y, config.barTint_.z, config.barTint_.w);
		constants_.frameTint_ = Vector4(config.frameTint_.x, config.frameTint_.y, config.frameTint_.z, config.frameTint_.w);
		constants_.backgroundTexture_ = texture.BackgroundIndex();
		constants_.barTexture_ = texture.BarIndex();
		constants_.frameTexture_ = texture.FrameIndex();
		drawing_ = true;
	}

	void BootFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (drawing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const BootConstantBuffer>(&constants_, 1));
		}
	}
}
