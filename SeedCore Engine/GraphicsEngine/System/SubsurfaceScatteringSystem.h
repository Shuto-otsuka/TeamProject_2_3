#pragma once
#include <GraphicsEngine/Renderer/SubsurfaceScatteringRenderer.h>

namespace SeedCore
{
	class World;

	class SubsurfaceScatteringSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const SubsurfaceScatteringRayConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		SubsurfaceScatteringRayConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}
