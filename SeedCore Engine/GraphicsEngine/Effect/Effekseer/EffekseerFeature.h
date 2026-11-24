#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>

namespace SeedCore
{
	class EffekseerFeature :public RenderFeature<EffekseerFeature>
	{
	public:
		struct DrawStage;

	public:
		void Gather(const RenderScene& scene, RenderFrame& frame);

	private:
		Bool active_ = false;
	};
}